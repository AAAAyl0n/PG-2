/**
 * @file app_bangboo.cpp
 * @brief USB MIDI Guitar App - joystick chord selection + 6 touch strings
 * @version 2.0
 * @date 2025-06-11
 */
#include "app_bangboo.h"
#include "../../hal/hal.h"
#include <cmath>

#include "esp32-hal-tinyusb.h"
#include "tusb.h"
#include "class/midi/midi_device.h"
#include "USB.h"

using namespace MOONCAKE::APPS;

// ---- USB MIDI descriptor registration ----
static bool _midi_interface_registered = false;

extern "C" uint16_t tusb_midi_load_descriptor(uint8_t* dst, uint8_t* itf)
{
    uint8_t str_index = tinyusb_add_string_descriptor("PocketGuitar MIDI");
    uint8_t ep_out = tinyusb_get_free_out_endpoint();
    if (ep_out == 0) return 0;
    uint8_t ep_in = tinyusb_get_free_in_endpoint();
    if (ep_in == 0) return 0;

    uint8_t descriptor[TUD_MIDI_DESC_LEN] = {
        TUD_MIDI_DESCRIPTOR(*itf, str_index, ep_out, (uint8_t)(0x80 | ep_in), 64)
    };
    *itf += 2;
    memcpy(dst, descriptor, TUD_MIDI_DESC_LEN);
    return TUD_MIDI_DESC_LEN;
    
}

static void _register_midi_interface()
{
    if (_midi_interface_registered) return;
    tinyusb_enable_interface(USB_INTERFACE_MIDI, TUD_MIDI_DESC_LEN, tusb_midi_load_descriptor);
    _midi_interface_registered = true;
}

// ---- USB host detection ----
// App always starts in LOCAL mode. Only LOCAL → USB transition is allowed.
// To return to LOCAL, restart the app (BOOT key exit + re-enter).
static volatile bool _usb_connected = false;

static void _usbEventHandler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    if (base != ARDUINO_USB_EVENTS) return;
    if (id == ARDUINO_USB_STARTED_EVENT || id == ARDUINO_USB_RESUME_EVENT) {
        _usb_connected = true; // one-way switch to USB
    }
}

static bool isUsbConnected()
{
    return _usb_connected;
}

// ---- MIDI helpers (only when USB host is connected) ----
static void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel)
{
    if (!isUsbConnected()) return;
    uint8_t msg[3] = {(uint8_t)(0x90 | (channel & 0x0F)), note, velocity};
    tud_midi_stream_write(0, msg, 3);
}

static void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel)
{
    if (!isUsbConnected()) return;
    uint8_t msg[3] = {(uint8_t)(0x80 | (channel & 0x0F)), note, velocity};
    tud_midi_stream_write(0, msg, 3);
}

static void sendAftertouch(uint8_t note, uint8_t pressure, uint8_t channel)
{
    if (!isUsbConnected()) return;
    uint8_t msg[3] = {(uint8_t)(0xA0 | (channel & 0x0F)), note, pressure};
    tud_midi_stream_write(0, msg, 3);
}

// ---- Local sample playback (when USB is unplugged) ----
// MIDI note → piano sample filename
// MIDI 36 = C2, 71 = B4 (covered range)
static const char* NOTE_NAMES[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

static void playLocalNote(uint8_t midiNote)
{
    if (midiNote < 36 || midiNote > 71) return;
    int octave = midiNote / 12 - 1;
    int idx = midiNote % 12;
    char path[32];
    snprintf(path, sizeof(path), "/piano/%s%d.wav", NOTE_NAMES[idx], octave);
    HAL::PlayWavFile(path);
}

// ---- Chord table ----
// 0 = muted string (no sound)
// Index: 0=Open, 1=C, 2=Em, 3=G, 4=Dm, 5=Am, 6=G7, 7=F, 8=E7
//                        6弦  5弦  4弦  3弦  2弦  1弦
static const uint8_t CHORD_NOTES[9][6] = {
    {40, 45, 50, 55, 59, 64},  // Open  E  A  D  G  B  E
    { 0, 48, 52, 55, 60, 64},  // C     x  C  E  G  C  E
    {40, 47, 52, 55, 59, 64},  // Em    E  B  E  G  B  E
    {43, 47, 50, 55, 59, 67},  // G     G  B  D  G  B  G
    { 0,  0, 50, 57, 62, 65},  // Dm    x  x  D  A  D  F
    { 0, 45, 52, 57, 60, 64},  // Am    x  A  E  A  C  E
    {43, 47, 50, 55, 59, 65},  // G7    G  B  D  G  B  F
    {41, 48, 53, 57, 60, 65},  // F     F  C  F  A  C  F
    {40, 47, 50, 56, 59, 64},  // E7    E  B  D  G# B  E
};

static const char* CHORD_NAMES[9] = {
    "Open", "C", "Em", "G", "Dm", "Am", "G7", "F", "E7"
};

// ---- Constants ----
static const int TOUCH_THRESHOLD = 25;
static const int pad_to_led[6] = {10, 8, 6, 4, 2, 0};
static const int NUM_LEDS = 11;
static const uint32_t SUSTAIN_OPTIONS[] = {0, 500, 1000, 1500, 2000};
static const int SUSTAIN_COUNT = 5;
static const int JOYSTICK_DEADZONE = 400;

// ---- Joystick zone detection ----
// Returns 0=Center, 1=Up(C), 2=NE(Em), 3=Right(G), 4=SE(Dm),
//         5=Down(Am), 6=SW(G7), 7=Left(F), 8=NW(E7)
int AppBangboo::_getJoystickZone()
{
    int dx = HAL::GetJoystickX() - _data.centerX;
    int dy = _data.centerY - HAL::GetJoystickY(); // invert: up = positive

    if (dx * dx + dy * dy < JOYSTICK_DEADZONE * JOYSTICK_DEADZONE)
        return 0; // Center = Open

    float angle = atan2f((float)dx, (float)dy) * 180.0f / M_PI; // 0°=up, CW positive
    if (angle < 0) angle += 360.0f;

    int sector = (int)((angle + 22.5f) / 45.0f) % 8;
    return sector + 1; // 1-8
}

// ---- Handle chord change ----
void AppBangboo::_onChordChange(uint8_t newChord)
{
    for (int i = 0; i < 6; i++) {
        if (_data.activeNote[i] != 0 && _data.releaseTime[i] != 0) {
            // Sustaining — let it keep ringing, don't touch it
            continue;
        }

        if (_data.activeNote[i] != 0 && _data.stringTouched[i]) {
            // String currently held — switch to new chord's note
            sendNoteOff(_data.activeNote[i], 0, 0);
            uint8_t newNote = CHORD_NOTES[newChord][i];
            if (newNote != 0) {
                sendNoteOn(newNote, 127, 0);
                _data.activeNote[i] = newNote;
            } else {
                _data.activeNote[i] = 0;
            }
        }
    }
    _data.currentChord = newChord;
}

// ---- App lifecycle ----
void AppBangboo::onCreate()
{
    _register_midi_interface();
    _usb_connected = false; // always start in LOCAL mode
    USB.onEvent(_usbEventHandler);
    USB.begin();

    for (int i = 0; i < 6; i++) {
        _data.stringTouched[i] = false;
        _data.activeNote[i] = 0;
        _data.releaseTime[i] = 0;
    }
}

void AppBangboo::onResume()
{
    HAL::GetCanvas()->setFont(&fonts::Font0);
    HAL::GetCanvas()->fillScreen(TFT_BLACK);
    _data.centerX = HAL::GetJoystickX();
    _data.centerY = HAL::GetJoystickY();
}

void AppBangboo::onRunning()
{
    while (true)
    {
        uint32_t now = HAL::Millis();

        // BOOT exits
        if (HAL::GetButton(GAMEPAD::BTN_BACK)) {
            for (int i = 0; i < 6; i++) {
                if (_data.activeNote[i] != 0) {
                    sendNoteOff(_data.activeNote[i], 0, 0);
                    _data.activeNote[i] = 0;
                }
            }
            destroyApp();
            return;
        }

        // Joystick press: cycle sustain
        bool joyPressed = HAL::GetButton(GAMEPAD::BTN_JOYSTICK);
        if (joyPressed && !_data.joystickPressed) {
            _data.sustainLevel = (_data.sustainLevel + 1) % SUSTAIN_COUNT;
        }
        _data.joystickPressed = joyPressed;

        uint32_t sustainMs = SUSTAIN_OPTIONS[_data.sustainLevel];

        // Joystick direction: chord selection
        uint8_t zone = _getJoystickZone();
        if (zone != _data.currentChord) {
            _onChordChange(zone);
        }

        uint8_t led_brightness[NUM_LEDS] = {0};

        for (int i = 0; i < 6; i++) {
            uint8_t chordNote = CHORD_NOTES[_data.currentChord][i];

            uint16_t filt = HAL::GetTouchFiltered(i);
            uint16_t base = HAL::GetTouchBaseline(i);
            bool touched = (base > TOUCH_THRESHOLD) && (filt < base - TOUCH_THRESHOLD);

            if (touched) {
                if (!_data.stringTouched[i]) {
                    // New touch
                    if (chordNote != 0) {
                        if (_data.activeNote[i] != 0)
                            sendNoteOff(_data.activeNote[i], 0, 0);
                        sendNoteOn(chordNote, 127, 0);
                        if (!isUsbConnected()) playLocalNote(chordNote);
                        _data.activeNote[i] = chordNote;
                    }
                    _data.stringTouched[i] = true;
                    _data.releaseTime[i] = 0;
                } else if (_data.releaseTime[i] != 0) {
                    // Re-touched during sustain
                    if (_data.activeNote[i] != 0)
                        sendNoteOff(_data.activeNote[i], 0, 0);
                    if (chordNote != 0) {
                        sendNoteOn(chordNote, 127, 0);
                        if (!isUsbConnected()) playLocalNote(chordNote);
                        _data.activeNote[i] = chordNote;
                    }
                    _data.releaseTime[i] = 0;
                }
            } else if (_data.stringTouched[i] && _data.releaseTime[i] == 0) {
                // Just released
                if (sustainMs > 0 && _data.activeNote[i] != 0) {
                    _data.releaseTime[i] = now;
                    sendAftertouch(_data.activeNote[i], 127, 0);
                } else {
                    if (_data.activeNote[i] != 0)
                        sendNoteOff(_data.activeNote[i], 0, 0);
                    _data.activeNote[i] = 0;
                    _data.stringTouched[i] = false;
                }
            }

            // Sustain decay
            if (_data.releaseTime[i] != 0 && sustainMs > 0) {
                uint32_t elapsed = now - _data.releaseTime[i];
                if (elapsed >= sustainMs) {
                    if (_data.activeNote[i] != 0) {
                        sendAftertouch(_data.activeNote[i], 0, 0);
                        sendNoteOff(_data.activeNote[i], 0, 0);
                    }
                    _data.activeNote[i] = 0;
                    _data.stringTouched[i] = false;
                    _data.releaseTime[i] = 0;
                } else {
                    uint8_t pressure = (uint8_t)(127 - (127 * elapsed / sustainMs));
                    sendAftertouch(_data.activeNote[i], pressure, 0);
                }
            }

            // LED
            if (_data.stringTouched[i]) {
                int center = pad_to_led[i];
                uint8_t bright = 255;
                if (_data.releaseTime[i] != 0 && sustainMs > 0) {
                    uint32_t elapsed = now - _data.releaseTime[i];
                    bright = (uint8_t)(255 - (255 * elapsed / sustainMs));
                }
                if (led_brightness[center] < bright)
                    led_brightness[center] = bright;
                uint8_t side = bright / 3;
                if (center - 1 >= 0 && led_brightness[center - 1] < side)
                    led_brightness[center - 1] = side;
                if (center + 1 < NUM_LEDS && led_brightness[center + 1] < side)
                    led_brightness[center + 1] = side;
            }
        }

        // LED color: sustain active = warm yellow, else cyan
        for (int i = 0; i < NUM_LEDS; i++) {
            uint8_t b = led_brightness[i];
            if (_data.sustainLevel > 0)
                HAL::SetLedColor(i, b, b / 2, 0);
            else
                HAL::SetLedColor(i, 0, b, b);
        }
        HAL::LedShow();

        _render();
        HAL::CanvasUpdate();

        //delay(10); // ~100fps, prevent WS2812 timing issues
    }
}

void AppBangboo::_render()
{
    auto* canvas = HAL::GetCanvas();
    canvas->fillScreen(TFT_BLACK);

    // Top-left: sustain duration (number only)
    canvas->setCursor(4, 4);
    if (_data.sustainLevel > 0) {
        canvas->setTextColor(TFT_YELLOW);
        canvas->printf("%d", SUSTAIN_OPTIONS[_data.sustainLevel]);
    } else {
        canvas->setTextColor(TFT_DARKGREY);
        canvas->print("0");
    }

    // Top-right: USB / Local mode indicator
    canvas->setCursor(180, 4);
    if (isUsbConnected()) {
        canvas->setTextColor(TFT_GREEN);
        canvas->print("USB");
    } else {
        canvas->setTextColor(TFT_CYAN);
        canvas->print("LOCAL");
    }

    // Center: chord name
    canvas->setTextColor(TFT_WHITE);
    const char* name = CHORD_NAMES[_data.currentChord];
    int len = strlen(name);
    canvas->setTextSize(3);
    canvas->setCursor(120 - len * 9, 110);
    canvas->print(name);
    canvas->setTextSize(1);
}

void AppBangboo::onDestroy()
{
    for (int i = 0; i < 6; i++) {
        if (_data.activeNote[i] != 0) {
            sendNoteOff(_data.activeNote[i], 0, 0);
            _data.activeNote[i] = 0;
        }
        _data.stringTouched[i] = false;
        _data.releaseTime[i] = 0;
    }
    for (int i = 0; i < NUM_LEDS; i++)
        HAL::SetLedColor(i, 0, 0, 0);
    HAL::LedShow();
}
