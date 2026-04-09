/**
 * @file app_bangboo.cpp
 * @brief USB MIDI Guitar App - joystick chord selection + 6 touch strings
 * @version 2.0
 * @date 2025-06-11
 */
#include "app_bangboo.h"
#include "../../hal/hal.h"
#include "assets/chord96_vlw.hpp"
#include "../assets/theme/theme.h"
#include <cmath>
#include <cstring>

#include "esp32-hal-tinyusb.h"
#include "tusb.h"
#include "class/midi/midi_device.h"
#include "USB.h"

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

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

// PG-2 SysEx receiver is defined further below — needs CW_* and GROUPS visible.
static void _pollSysEx();
static volatile int8_t  _sysExPendingGroup   = -1;
static volatile int8_t  _sysExPendingSustain = -1;

// ============================================================================
// MIDI Mixer — software polyphonic sample playback for LOCAL mode
// ----------------------------------------------------------------------------
// 6 voice slots, each holds an open WAV file streaming from LittleFS.
// A dedicated FreeRTOS task pulls 256 stereo frames from each active voice,
// sums them with clipping, and writes the result to I2S in one shot.
// i2s_write blocks until DMA buffer drains → automatic pacing, no timer needed.
// When idle (no voices), the task writes silence to keep DMA running.
// ============================================================================

static const char* NOTE_NAMES[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

namespace MidiMixer
{
    constexpr int VOICE_COUNT = 6;
    constexpr int MIX_FRAMES  = 256;          // stereo frames per chunk (~5ms @ 48kHz)
    constexpr int FRAME_BYTES = 4;            // 2ch × 16bit

    // Sustain model:
    // - sustainMs == 0  → play sample once, no loop, voice ends at natural EOF.
    // - sustainMs >  0  → keep voice alive for max(fileLen, sustainMs); when the
    //                     file reaches EOF, seek back to loopStartByte and keep
    //                     reading. A linear release envelope fades the last 25%
    //                     of the total duration to silence. Each loop pass also
    //                     gets a tiny linear fade-in to soften the splice click.
    constexpr uint32_t SAMPLE_RATE_HZ  = 48000;
    constexpr int32_t  GAIN_UNITY      = 32767;
    constexpr int      LOOP_FADE_FR    = 96;        // ~2ms loop-splice fade-in
    constexpr uint32_t MIN_RELEASE_FR  = SAMPLE_RATE_HZ / 20; // 50ms

    struct Voice
    {
        File     file;
        bool     active;
        bool     inLoopPass;                  // false during the first natural pass
        uint8_t  midiNote;
        uint32_t bytesRemaining;
        uint32_t loopStartByte;               // file offset where the sustain loop begins
        uint32_t loopLenBytes;                // byte length of the sustain loop region
        uint32_t framesPlayed;                // total frames emitted so far
        uint32_t totalFrames;                 // target total length (0 = play to EOF)
        uint32_t releaseStartFrame;           // frame where the fade-out begins
        uint32_t startTick;                   // for voice stealing (oldest first)
    };

    static Voice              voices[VOICE_COUNT];
    static SemaphoreHandle_t  mutex      = nullptr;
    static TaskHandle_t       taskHandle = nullptr;
    static volatile bool      running    = false;
    static uint32_t           tickCounter = 0;

    // Reusable buffers (kept on heap-style static, not stack — too big for task stack)
    static int32_t mixAccum[MIX_FRAMES * 2];  // 32-bit accumulator to avoid overflow during sum
    static int16_t mixOut[MIX_FRAMES * 2];    // final clipped 16-bit stereo output
    static uint8_t scratch[MIX_FRAMES * FRAME_BYTES]; // shared per-voice read buffer

    // Find "data" chunk in WAV file, return offset of audio bytes (0 = error)
    static uint32_t parseWavData(File& f)
    {
        char header[12];
        f.seek(0);
        if (f.read((uint8_t*)header, 12) != 12) return 0;
        if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) return 0;

        char     chunkId[4];
        uint32_t chunkSize;
        while (f.available() >= 8) {
            if (f.read((uint8_t*)chunkId, 4) != 4) return 0;
            if (f.read((uint8_t*)&chunkSize, 4) != 4) return 0;
            if (memcmp(chunkId, "data", 4) == 0) {
                return f.position();
            }
            f.seek(f.position() + chunkSize);
        }
        return 0;
    }

    static void mixerTask(void*)
    {
        while (running) {
            // 1. Clear accumulator
            memset(mixAccum, 0, sizeof(mixAccum));

            // 2. Mix every active voice (count them too, for normalization)
            int activeCount = 0;
            xSemaphoreTake(mutex, portMAX_DELAY);
            for (int v = 0; v < VOICE_COUNT; v++) {
                Voice& voice = voices[v];
                if (!voice.active) continue;
                activeCount++;

                size_t toRead = MIX_FRAMES * FRAME_BYTES;
                if (toRead > voice.bytesRemaining) toRead = voice.bytesRemaining;

                // Frames already emitted within the *current* loop pass — used for
                // the splice fade-in. (Only meaningful while inLoopPass.)
                uint32_t framesIntoPass = (voice.loopLenBytes - voice.bytesRemaining) / FRAME_BYTES;

                size_t actualRead = voice.file.read(scratch, toRead);
                voice.bytesRemaining -= actualRead;

                int16_t* src = (int16_t*)scratch;
                int frames  = actualRead / FRAME_BYTES;

                // --- Compute base release-envelope gain (Q15) per frame ---
                // For speed, evaluate at chunk start and end and linearly interpolate.
                auto envAt = [&](uint32_t f) -> int32_t {
                    if (voice.totalFrames == 0) return GAIN_UNITY;        // no time limit
                    if (f >= voice.totalFrames) return 0;
                    if (f <= voice.releaseStartFrame) return GAIN_UNITY;
                    uint32_t into = f - voice.releaseStartFrame;
                    uint32_t span = voice.totalFrames - voice.releaseStartFrame;
                    return (int32_t)(GAIN_UNITY - (int32_t)((uint64_t)GAIN_UNITY * into / span));
                };

                int32_t gStart = envAt(voice.framesPlayed);
                int32_t gEnd   = envAt(voice.framesPlayed + frames);

                for (int i = 0; i < frames; i++) {
                    int32_t g = gStart + ((gEnd - gStart) * i) / (frames > 1 ? (frames - 1) : 1);

                    // Loop-splice fade-in: smooth the first LOOP_FADE_FR frames of each loop pass.
                    if (voice.inLoopPass) {
                        uint32_t fr = framesIntoPass + i;
                        if (fr < (uint32_t)LOOP_FADE_FR) {
                            g = (int32_t)((int64_t)g * fr / LOOP_FADE_FR);
                        }
                    }

                    mixAccum[i * 2 + 0] += ((int32_t)src[i * 2 + 0] * g) >> 15;
                    mixAccum[i * 2 + 1] += ((int32_t)src[i * 2 + 1] * g) >> 15;
                }

                voice.framesPlayed += frames;

                // Reached target duration → done.
                if (voice.totalFrames > 0 && voice.framesPlayed >= voice.totalFrames) {
                    voice.file.close();
                    voice.active = false;
                }
                // Reached end of current pass.
                else if (voice.bytesRemaining == 0 || actualRead < toRead) {
                    if (voice.totalFrames == 0 || voice.loopLenBytes == 0) {
                        // No sustain target, or no loop region — stop.
                        voice.file.close();
                        voice.active = false;
                    } else {
                        // Loop the tail.
                        voice.file.seek(voice.loopStartByte);
                        voice.bytesRemaining = voice.loopLenBytes;
                        voice.inLoopPass = true;
                    }
                }
            }
            xSemaphoreGive(mutex);

            // 3. Normalize by max voice count (constant) — guarantees no clipping
            // and keeps each voice's amplitude stable regardless of polyphony.
            // Trade -15dB digital headroom for predictable per-voice loudness.
            (void)activeCount;
            for (int i = 0; i < MIX_FRAMES * 2; i++) {
                int32_t s = mixAccum[i] / VOICE_COUNT;
                if (s > 32767)  s = 32767;
                if (s < -32768) s = -32768;
                mixOut[i] = (int16_t)s;
            }

            // 4. Push to I2S (blocks → natural pacing)
            size_t written = 0;
            i2s_write(I2S_NUM_0, mixOut, sizeof(mixOut), &written, portMAX_DELAY);
        }

        // Clean exit
        i2s_zero_dma_buffer(I2S_NUM_0);
        taskHandle = nullptr;
        vTaskDelete(NULL);
    }

    void init()
    {
        if (running) return;
        if (!mutex) mutex = xSemaphoreCreateMutex();
        for (int i = 0; i < VOICE_COUNT; i++) voices[i].active = false;
        tickCounter = 0;
        running = true;
        xTaskCreatePinnedToCore(mixerTask, "MidiMixer", 6144, nullptr,
                                configMAX_PRIORITIES - 4, &taskHandle, 0);
    }

    void shutdown()
    {
        if (!running) return;
        running = false;
        // Wait for task to exit
        while (taskHandle) vTaskDelay(pdMS_TO_TICKS(2));

        xSemaphoreTake(mutex, portMAX_DELAY);
        for (int i = 0; i < VOICE_COUNT; i++) {
            if (voices[i].active) {
                voices[i].file.close();
                voices[i].active = false;
            }
        }
        xSemaphoreGive(mutex);
    }

    // Play a MIDI note: opens WAV, allocates a voice slot, steals oldest if full.
    // sustainMs == 0 → play once and stop at natural EOF.
    // sustainMs >  0 → keep ringing for at least sustainMs (looping the tail).
    void playNote(uint8_t midiNote, uint32_t sustainMs)
    {
        if (!running) return;
        if (midiNote < 36 || midiNote > 71) return;

        char path[40];
        snprintf(path, sizeof(path), "/guitar/%s%d.wav",
                 NOTE_NAMES[midiNote % 12], midiNote / 12 - 1);

        File f = LittleFS.open(path, "r");
        if (!f) return;

        uint32_t dataOffset = parseWavData(f);
        if (dataOffset == 0) { f.close(); return; }

        uint32_t dataSize = f.size() - dataOffset;
        f.seek(dataOffset);

        xSemaphoreTake(mutex, portMAX_DELAY);

        // Find an empty slot, else steal the oldest voice
        int slot = -1;
        for (int i = 0; i < VOICE_COUNT; i++) {
            if (!voices[i].active) { slot = i; break; }
        }
        if (slot < 0) {
            uint32_t oldestTick = UINT32_MAX;
            for (int i = 0; i < VOICE_COUNT; i++) {
                if (voices[i].startTick < oldestTick) {
                    oldestTick = voices[i].startTick;
                    slot = i;
                }
            }
            voices[slot].file.close();
        }

        // Sustain loop region: latter ~50% of the sample (past the pluck transient).
        // Aligned down to FRAME_BYTES so we never split a stereo frame.
        uint32_t loopStartRel = (dataSize / 2) & ~(uint32_t)(FRAME_BYTES - 1);
        uint32_t loopLen      = dataSize - loopStartRel;

        // Compute target frame count and release-envelope start.
        uint32_t fileFrames   = dataSize / FRAME_BYTES;
        uint32_t totalFrames  = 0;            // 0 = play to natural EOF
        uint32_t releaseStart = 0;
        if (sustainMs > 0) {
            uint32_t sustFrames = sustainMs * SAMPLE_RATE_HZ / 1000;
            totalFrames = sustFrames > fileFrames ? sustFrames : fileFrames;
            // Release = last 25% of total, clamped to >= 50ms.
            uint32_t relFrames = totalFrames / 4;
            if (relFrames < MIN_RELEASE_FR) relFrames = MIN_RELEASE_FR;
            if (relFrames > totalFrames)    relFrames = totalFrames;
            releaseStart = totalFrames - relFrames;
        }

        voices[slot].file              = f;
        voices[slot].active            = true;
        voices[slot].inLoopPass        = false;
        voices[slot].midiNote          = midiNote;
        voices[slot].bytesRemaining    = dataSize;
        voices[slot].loopStartByte     = dataOffset + loopStartRel;
        voices[slot].loopLenBytes      = loopLen;
        voices[slot].framesPlayed      = 0;
        voices[slot].totalFrames       = totalFrames;
        voices[slot].releaseStartFrame = releaseStart;
        voices[slot].startTick         = ++tickCounter;

        xSemaphoreGive(mutex);
    }
} // namespace MidiMixer

static void playLocalNote(uint8_t midiNote, uint32_t sustainMs)
{
    MidiMixer::playNote(midiNote, sustainMs);
}

// ---- Chord tables (per group) ----
// 0 = muted string (no sound). Index 0 = center (Open), 1-8 = joystick directions:
// 1=Up, 2=NE, 3=Right, 4=SE, 5=Down, 6=SW, 7=Left, 8=NW
constexpr int CHORD_COUNT = 9;

// G1 / G2 — standard guitar open chords
//                                    6弦  5弦  4弦  3弦  2弦  1弦
static const uint8_t CHORDS_STD[CHORD_COUNT][6] = {
    {40, 45, 50, 55, 59, 64},  // 0 Open    E  A  D  G  B  E
    { 0, 48, 52, 55, 60, 64},  // 1 C       x  C  E  G  C  E
    {40, 47, 52, 55, 59, 64},  // 2 Em      E  B  E  G  B  E
    {43, 47, 50, 55, 59, 67},  // 3 G       G  B  D  G  B  G
    { 0,  0, 50, 57, 62, 65},  // 4 Dm      x  x  D  A  D  F
    { 0, 45, 52, 57, 60, 64},  // 5 Am      x  A  E  A  C  E
    {43, 47, 50, 55, 59, 65},  // 6 G7      G  B  D  G  B  F
    {41, 48, 53, 57, 60, 65},  // 7 F       F  C  F  A  C  F
    {40, 47, 50, 56, 59, 64},  // 8 E7      E  B  D  G# B  E
};
static const char* NAMES_STD[CHORD_COUNT] = {
    "Daizy", "C", "Em", "G", "Dm", "Am", "G7", "F", "E7"
};

// GH — jazz / passing chords (C major family). Muted strings filled in
// where it doesn't break the chord's character; slash chords keep their
// bass note as the lowest sounding pitch.
//                                    6弦  5弦  4弦  3弦  2弦  1弦
static const uint8_t CHORDS_GH[CHORD_COUNT][6] = {
    {40, 45, 50, 55, 59, 64},  // 0 Daizy     E  A  D  G  B  E
    {41, 45, 53, 57, 60, 64},  // 1 Fmaj7     F  A  F  A  C  E   (low F + open A)
    {40, 47, 50, 55, 59, 64},  // 2 Em7       E  B  D  G  B  E
    {38, 45, 50, 57, 60, 64},  // 3 Am/D      D  A  D  A  C  E   (D bass)
    { 0,  0, 51, 55, 59, 64},  // 4 Em7/D#    xx D# G  B  E      (slash chord)
    { 0, 48, 52, 57, 59, 64},  // 5 Asus2/C   x  C  E  A  B  E   (slash chord)
    {40, 48, 52, 55, 59, 64},  // 6 Cmaj7     E  C  E  G  B  E   (low E = 3rd)
    {40, 45, 52, 57, 60, 67},  // 7 Am/E      0  0 2 2 1 3 (1弦=2弦+2格)
    {38, 45, 50, 57, 60, 65},  // 8 Dm7       D  A  D  A  C  F   (replaces C#)
};
static const char* NAMES_GH[CHORD_COUNT] = {
    "Daizy", "Fmaj7", "Em7", "Am/D", "Em7/D#", "Asus2/C", "Cmaj7", "Am/E", "Dm7"
};

// ---- Group definitions ----
struct Group
{
    const char*         name;
    const uint8_t     (*chords)[6];     // pointer to CHORD_COUNT × 6 table
    const char* const*  chordNames;
    int8_t              offset;          // semitone shift applied to non-muted notes
};

// ---- "CW" — runtime user group, mutable via SysEx ---------------------------
// Defaults are a copy of GH so the slot is musically usable on first boot.
// Names are stored in fixed-size buffers to allow web overrides without alloc.
constexpr int CW_NAME_MAX = 12;
static uint8_t CW_CHORDS[CHORD_COUNT][6] = {
    {40, 45, 50, 55, 59, 64},
    {41, 45, 53, 57, 60, 64},
    {40, 47, 50, 55, 59, 64},
    {38, 45, 50, 57, 60, 64},
    { 0,  0, 51, 55, 59, 64},
    { 0, 48, 52, 57, 59, 64},
    {40, 48, 52, 55, 59, 64},
    {40, 45, 52, 57, 60, 67},
    {38, 45, 50, 57, 60, 65},
};
static char  CW_NAMES_BUF[CHORD_COUNT][CW_NAME_MAX] = {
    "Daizy", "Fmaj7", "Em7", "Am/D", "Em7/D#", "Asus2/C", "Cmaj7", "Am/E", "Dm7"
};
static const char* CW_NAMES_PTR[CHORD_COUNT] = {
    CW_NAMES_BUF[0], CW_NAMES_BUF[1], CW_NAMES_BUF[2],
    CW_NAMES_BUF[3], CW_NAMES_BUF[4], CW_NAMES_BUF[5],
    CW_NAMES_BUF[6], CW_NAMES_BUF[7], CW_NAMES_BUF[8],
};
static int8_t CW_OFFSET = 4;

// Note: GROUPS is non-const so that the CW slot's offset can be updated at
// runtime. Built-in groups (G1/G2/GH) still point at const tables, so their
// chord notes/names remain immutable.
static Group GROUPS[] = {
    {"G1", CHORDS_STD, NAMES_STD,  0},
    {"G2", CHORDS_STD, NAMES_STD, 12},
    {"GH", CHORDS_GH,  NAMES_GH,   4},  // capo 4品
    {"CW", CW_CHORDS,  CW_NAMES_PTR, 4},  // 可由 SysEx 实时下发
};
static const int GROUP_COUNT = sizeof(GROUPS) / sizeof(GROUPS[0]);

// CW group's slot index in GROUPS[]; the only group SysEx is allowed to mutate.
static constexpr int CW_GROUP_INDEX = 3;

// ============================================================================
// PG-2 SysEx receiver
// ----------------------------------------------------------------------------
// Frame: F0 7D 50 47 32 <cmd> <args...> F7
//   7D       = non-commercial mfr id
//   50 47 32 = "PG2" product tag
//
// Commands (v1):
//   0x01 SET_GROUP            <idx 0..3>           切换和弦组
//   0x02 SET_SUSTAIN          <level 0..4>         切换 sustain 档位
//   0x10 CW_SET_CHORD_NOTES   <chord 0..8> n0..n5  改 CW 单和弦 6 弦音符
//   0x11 CW_SET_CHORD_NAME    <chord 0..8> <len> <ascii...>
//   0x12 CW_SET_OFFSET        <offset+64>          capo offset (-64..63)
//
// _sysExPendingGroup / _sysExPendingSustain are forward-declared above the
// MidiMixer namespace; they're applied in the main render loop. CW table
// writes are applied directly here since they only touch dedicated CW_*
// buffers, not the renderer's hot path.
// ============================================================================
static void _handleSysEx(const uint8_t* d, size_t n)
{
    if (n < 5) return;
    if (d[0] != 0x7D) return;
    if (d[1] != 'P' || d[2] != 'G' || d[3] != '2') return;

    uint8_t cmd = d[4];
    switch (cmd) {
        case 0x01:
            if (n >= 6) _sysExPendingGroup = (int8_t)d[5];
            break;
        case 0x02:
            if (n >= 6) _sysExPendingSustain = (int8_t)d[5];
            break;

        case 0x10: {
            if (n < 12) break;
            uint8_t chord = d[5];
            if (chord >= CHORD_COUNT) break;
            for (int i = 0; i < 6; i++) {
                CW_CHORDS[chord][i] = d[6 + i] & 0x7F;
            }
            break;
        }

        case 0x11: {
            if (n < 7) break;
            uint8_t chord   = d[5];
            uint8_t nameLen = d[6];
            if (chord >= CHORD_COUNT) break;
            if (n < (size_t)(7 + nameLen)) break;
            size_t copy = nameLen < (CW_NAME_MAX - 1) ? nameLen : (CW_NAME_MAX - 1);
            for (size_t i = 0; i < copy; i++) {
                uint8_t c = d[7 + i] & 0x7F;
                CW_NAMES_BUF[chord][i] = (c >= 0x20) ? (char)c : '?';
            }
            CW_NAMES_BUF[chord][copy] = '\0';
            break;
        }

        case 0x12: {
            if (n < 6) break;
            int8_t off = (int8_t)d[5] - 64;
            GROUPS[CW_GROUP_INDEX].offset = off;
            break;
        }

        default:
            break;
    }
}

static void _pollSysEx()
{
    if (!tud_midi_mounted()) return;

    static uint8_t  buf[128];
    static size_t   len     = 0;
    static bool     inSysEx = false;

    uint8_t pkt[4];
    while (tud_midi_available()) {
        if (!tud_midi_packet_read(pkt)) break;
        for (int i = 1; i < 4; i++) {
            uint8_t b = pkt[i];
            if (b == 0xF0) { inSysEx = true; len = 0; continue; }
            if (b == 0xF7) {
                if (inSysEx) _handleSysEx(buf, len);
                inSysEx = false;
                len = 0;
                continue;
            }
            if (inSysEx && len < sizeof(buf)) buf[len++] = b;
        }
    }
}

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
            const Group& g = GROUPS[_data.groupIndex];
            uint8_t baseNote = g.chords[newChord][i];
            uint8_t newNote = (baseNote == 0) ? 0 : (uint8_t)(baseNote + g.offset);
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

    MidiMixer::init();

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

        // Encoder rotation: cycle octave group
        bool cw  = HAL::GetButton(GAMEPAD::BTN_RIGHT);
        bool ccw = HAL::GetButton(GAMEPAD::BTN_SELECT);
        if (cw && !_data.encoderCwPressed) {
            _data.groupIndex = (_data.groupIndex + 1) % GROUP_COUNT;
            _showGroupPopup(GROUPS[_data.groupIndex].name);
        }
        if (ccw && !_data.encoderCcwPressed) {
            _data.groupIndex = (_data.groupIndex + GROUP_COUNT - 1) % GROUP_COUNT;
            _showGroupPopup(GROUPS[_data.groupIndex].name);
        }
        _data.encoderCwPressed = cw;
        _data.encoderCcwPressed = ccw;

        // ---- Drain incoming SysEx and apply any pending state changes ----
        _pollSysEx();
        if (_sysExPendingGroup >= 0 && _sysExPendingGroup < GROUP_COUNT) {
            _data.groupIndex = (uint8_t)_sysExPendingGroup;
            _showGroupPopup(GROUPS[_data.groupIndex].name);
            _sysExPendingGroup = -1;
        } else if (_sysExPendingGroup >= 0) {
            _sysExPendingGroup = -1;  // out-of-range, drop
        }
        if (_sysExPendingSustain >= 0 && _sysExPendingSustain < SUSTAIN_COUNT) {
            _data.sustainLevel = (uint8_t)_sysExPendingSustain;
            _sysExPendingSustain = -1;
        } else if (_sysExPendingSustain >= 0) {
            _sysExPendingSustain = -1;
        }

        uint32_t sustainMs = SUSTAIN_OPTIONS[_data.sustainLevel];

        // Joystick direction → sticky chord selection
        // - Push to a direction (zone != 0) → switch chord, refresh timer
        // - Return to center → keep current chord
        // - 10s of no direction input → revert to Open
        uint8_t zone = _getJoystickZone();
        if (zone != 0) {
            if (zone != _data.currentChord) {
                _onChordChange(zone);
            }
            _data.lastChordActivity = now;
        } else if (_data.currentChord != 0 &&
                   now - _data.lastChordActivity >= 10000) {
            _onChordChange(0); // revert to Open
        }

        uint8_t led_brightness[NUM_LEDS] = {0};

        const Group& curGroup = GROUPS[_data.groupIndex];

        for (int i = 0; i < 6; i++) {
            uint8_t baseNote = curGroup.chords[_data.currentChord][i];
            uint8_t chordNote = (baseNote == 0) ? 0 : (uint8_t)(baseNote + curGroup.offset);

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
                        if (!isUsbConnected()) playLocalNote(chordNote, sustainMs);
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
                        if (!isUsbConnected()) playLocalNote(chordNote, sustainMs);
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

    // Reset to default bitmap font for the top status bar.
    // (Previous frame may have left a runtime VLW font active.)
    canvas->setFont(&fonts::Font0);
    canvas->setTextColor(TFT_DARKGREY);

    // Top-left: sustain duration (number only)
    canvas->setCursor(4, 4);
    if (_data.sustainLevel > 0) {
        canvas->printf("%d", SUSTAIN_OPTIONS[_data.sustainLevel]);
    } else {
        canvas->print("0");
    }

    // Top-right: USB / Local mode indicator
    if (isUsbConnected()) {
        canvas->setCursor(222, 4);
        canvas->print("USB");
    } else {
        canvas->setCursor(212, 4);
        canvas->print("LOCAL");
    }

    const Group& g = GROUPS[_data.groupIndex];

    // Layout slot index for each chord (0..8). The chord indices match
    // _getJoystickZone(): 0=Center, 1=Up, 2=NE, 3=Right, 4=SE, 5=Down,
    // 6=SW, 7=Left, 8=NW. The grid is laid out:
    //   NW(8) | Up(1) | NE(2)
    //   Lt(7) | Cn(0) | Rt(3)
    //   SW(6) | Dn(5) | SE(4)
    static const int SLOT_TO_CHORD[9] = {8, 1, 2, 7, 0, 3, 6, 5, 4};

    if (HAL::GetSystemConfig().midi_ui_mode == 1) {
        // ---- BOX mode: 3x3 grid, highlight current chord cell ----
        const int gridSize = 132;          // total grid pixel size
        const int cell     = gridSize / 3; // cell side
        const int gap      = 3;
        const int gx0      = 120 - gridSize / 2;
        const int gy0      = 135 - gridSize / 2;

        for (int s = 0; s < 9; s++) {
            int row = s / 3;
            int col = s % 3;
            int x = gx0 + col * cell;
            int y = gy0 + row * cell;
            int w = cell - gap;
            int h = cell - gap;

            uint8_t chordIdx = SLOT_TO_CHORD[s];
            bool highlighted = (chordIdx == _data.currentChord);

            uint16_t fill = highlighted ? TFT_RED : TFT_LIGHTGREY;
            canvas->fillRect(x, y, w, h, fill);

            // Chord name centered in cell (skip "Open" for the empty look)
            const char* name = g.chordNames[chordIdx];
            if (chordIdx != 0) {
                canvas->setTextColor(highlighted ? TFT_WHITE : TFT_BLACK);
                canvas->setTextSize(1);
                int len = strlen(name);
                int tx = x + w / 2 - len * 3;
                int ty = y + h / 2 - 4;
                canvas->setCursor(tx, ty);
                canvas->print(name);
            }
        }
    } else {
        // ---- Normal mode: large chord name in screen center ----
        // The VLW font is loaded fresh each frame because LovyanGFX's
        // setFont(&Font0) at the top of this function destroys any runtime
        // font. loadFont() from a flash array only reads the small metric
        // table, so this is cheap.
        canvas->loadFont(chord96_vlw);
        canvas->setTextColor(TFT_WHITE);
        canvas->setTextDatum(middle_center);
        const char* name = g.chordNames[_data.currentChord];
        canvas->drawString(name, 120, 118);  // 17px above center
        canvas->setTextDatum(top_left);

        // ---- Bottom strip: 8 chord names in 2×4 grid ----
        // Row 1: NW, Up,   NE,   Right   (chord indices 8, 1, 2, 3)
        // Row 2: Left, SW, Down, SE      (chord indices 7, 6, 5, 4)
        static const uint8_t STRIP_LAYOUT[2][4] = {
            {8, 1, 2, 3},
            {7, 6, 5, 4},
        };
        canvas->setFont(&fonts::Font0);
        canvas->setTextSize(1);
        const int slotW  = 240 / 4;          // 60 px
        const int rowY[2] = {214, 226};
        for (int row = 0; row < 2; row++) {
            for (int col = 0; col < 4; col++) {
                uint8_t idx = STRIP_LAYOUT[row][col];
                const char* nm = g.chordNames[idx];
                int len = strlen(nm);
                int cx  = col * slotW + slotW / 2;
                int tx  = cx - len * 3;
                bool sel = (idx == _data.currentChord);
                canvas->setTextColor(sel ? TFT_LIGHTGREY : TFT_DARKGREY);
                canvas->setCursor(tx, rowY[row]);
                canvas->print(nm);
            }
        }
    }

    // Group-name dropdown bubble (drawn last so it overlays everything)
    _renderPopup();
}

void AppBangboo::_showGroupPopup(const char* text)
{
    if (text == nullptr) return;
    _data.popupText = text;
    _data.popupVisible = true;
    _data.popupAnimating = true;
    _data.popupShowTime = HAL::Millis();
    _data.popupSlideAnim.setAnim(LVGL::ease_out, 0, 24, 300);
    _data.popupSlideAnim.resetTime(HAL::Millis());
}

void AppBangboo::_renderPopup()
{
    if (_data.popupVisible && !_data.popupAnimating) {
        if (HAL::Millis() - _data.popupShowTime > _data.popupDisplayDuration) {
            _data.popupAnimating = true;
            _data.popupSlideAnim.setAnim(LVGL::ease_out, 24, -10, 300);
            _data.popupSlideAnim.resetTime(HAL::Millis());
        }
    }

    if (!_data.popupVisible && !_data.popupAnimating) return;

    int animValue = _data.popupSlideAnim.getValue(HAL::Millis());

    auto* canvas = HAL::GetCanvas();
    canvas->setFont(&fonts::Font0);  // ensure VLW isn't active

    int rect_width  = 70;
    int rect_height = 24;
    int rect_x = (240 - rect_width) / 2;
    int rect_y = 5 - rect_height + animValue;

    canvas->fillSmoothRoundRect(rect_x, rect_y, rect_width, rect_height, 8, THEME_COLOR_NIGHT);
    canvas->setTextSize(2);
    canvas->setTextColor(TFT_WHITE, THEME_COLOR_NIGHT);
    canvas->drawCenterString(_data.popupText.c_str(), 120, rect_y + 4, &fonts::Font0);
    canvas->setTextSize(1);

    if (_data.popupAnimating && _data.popupSlideAnim.isFinished(HAL::Millis())) {
        _data.popupAnimating = false;
        if (animValue <= 0) {
            _data.popupVisible = false;
        }
    }
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

    HAL::GetCanvas()->unloadFont();

    MidiMixer::shutdown();
}
