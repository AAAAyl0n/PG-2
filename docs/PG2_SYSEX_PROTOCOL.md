# PocketGuitar 2 — SysEx Protocol (v1)

This document is for **web-side developers** building tools that talk to a
PocketGuitar 2 device over USB MIDI. It describes the SysEx wire format the
device understands and includes copy-pasteable JavaScript helpers.

> Audience: web devs writing browser/Electron tools.
> No firmware knowledge required.

---

## 1. Connecting from the browser

PG-2 enumerates as a standard USB MIDI class device. The endpoint name is
`PocketGuitar MIDI`. To send SysEx you **must** request MIDI access with
`{ sysex: true }` — the user will see a one-time browser permission prompt.

```js
const access = await navigator.requestMIDIAccess({ sysex: true });

const port = [...access.outputs.values()]
              .find(o => o.name.includes("PocketGuitar"));

if (!port) throw new Error("PG-2 not connected");
```

Browser support: Chrome / Edge / Opera / Brave on desktop. Safari and
Firefox do not currently expose Web MIDI.

---

## 2. Frame format

Every command is wrapped in a SysEx frame:

```
F0  7D  50 47 32  <cmd>  <args...>  F7
└┬┘ └┬┘ └──┬───┘  └─┬─┘  └────┬───┘  └┬┘
 │   │     │       │         │       └ SysEx end
 │   │     │       │         └ command-specific arguments
 │   │     │       └ command byte (see §3)
 │   │     └ ASCII "PG2" — product tag
 │   └ Manufacturer ID 0x7D (non-commercial / educational range)
 └ SysEx start
```

### Hard rules

- **Every byte between `F0` and `F7` must have its high bit clear**
  (i.e. `0x00`–`0x7F`). This is a MIDI spec limitation, not ours. The
  helper functions below mask `& 0x7F` automatically.
- MIDI note range is `0..127`, which already fits in 7 bits — no encoding
  trick needed for note values.
- Signed values (like capo offset) are encoded as `value + 64`, range
  `0..127` → `-64..63`.
- Strings are plain ASCII bytes. Non-printable characters (`< 0x20`) are
  replaced by `?` on the device.
- Maximum total frame size is **128 bytes** including `F0`/`F7`.

---

## 3. Command reference

### `0x01` — SET_GROUP

Switch the active chord group. The device will pop a small dropdown bubble
showing the new group's name.

```
F0 7D 50 47 32 01 <idx> F7
```

| Field | Range | Meaning |
|---|---|---|
| `idx` | 0..3 | 0=G1, 1=G2, 2=GH, 3=CW (user-configurable) |

Out-of-range values are silently dropped.

---

### `0x02` — SET_SUSTAIN

Change the sustain (release-tail) duration.

```
F0 7D 50 47 32 02 <level> F7
```

| `level` | Sustain |
|---|---|
| 0 | off |
| 1 | 500 ms |
| 2 | 1000 ms |
| 3 | 1500 ms |
| 4 | 2000 ms |

---

### `0x10` — CW_SET_CHORD_NOTES

Overwrite the 6 string notes for one chord slot in the **CW** group only.
G1/G2/GH cannot be modified.

```
F0 7D 50 47 32 10 <chord> n0 n1 n2 n3 n4 n5 F7
```

| Field | Range | Meaning |
|---|---|---|
| `chord` | 0..8 | Chord slot index (see §4 for joystick mapping) |
| `n0..n5` | 0..127 | MIDI note for strings 6..1 (low E to high E). `0` = muted (no sound when that string is touched). |

The CW group has its own capo offset (see `0x12`); the offset is added to
non-zero notes at playback time, so write the *unshifted* note here.

---

### `0x11` — CW_SET_CHORD_NAME

Set the on-screen name for one chord in the CW group.

```
F0 7D 50 47 32 11 <chord> <len> <ascii bytes...> F7
```

| Field | Range | Meaning |
|---|---|---|
| `chord` | 0..8 | Chord slot |
| `len` | 0..11 | Number of bytes that follow |
| `ascii` | 0x20..0x7F | Display name (ASCII) |

Names longer than 11 characters are truncated. The bottom UI strip uses a
60 px-wide cell per chord, so practical names are ≤ 7 characters wide.

---

### `0x12` — CW_SET_OFFSET

Set the CW group's capo offset (semitones added to every non-muted note at
playback time). Useful for transposing the whole CW group without rewriting
each chord.

```
F0 7D 50 47 32 12 <offset+64> F7
```

| Field | Encoded | Decoded |
|---|---|---|
| `offset+64` | 0..127 | -64..63 semitones |

Common values:

| Encoded byte | Semitones |
|---|---|
| `64` | 0 (no transpose) |
| `68` | +4 (default, like GH group "capo 4") |
| `76` | +12 (one octave up) |
| `52` | -12 (one octave down) |

---

## 4. Chord slot ↔ joystick mapping

The 9 chord slots map to joystick directions:

```
slot 8 (NW)   slot 1 (Up)   slot 2 (NE)
slot 7 (Lt)   slot 0 (Cn)   slot 3 (Rt)
slot 6 (SW)   slot 5 (Dn)   slot 4 (SE)
```

`slot 0` is the "neutral / open" chord — what plays when the joystick is
centered. The other 8 are the directional chords.

The bottom-of-screen UI strip shows them in this 2×4 layout (skipping
slot 0):

```
NW  Up  NE  Right
Left SW Down SE
```

So if you want chord X to appear in the top-left of the strip, write it to
slot **8** (NW).

---

## 5. JavaScript helpers

A self-contained client. Drop into a `<script type="module">` block.

```js
const PG2_HEADER = [0xF0, 0x7D, 0x50, 0x47, 0x32];

let pg2 = null;

export async function connectPG2() {
  const access = await navigator.requestMIDIAccess({ sysex: true });
  pg2 = [...access.outputs.values()]
          .find(o => o.name.includes("PocketGuitar"));
  if (!pg2) throw new Error("PG-2 not found");
  return pg2;
}

function send(cmd, ...args) {
  if (!pg2) throw new Error("call connectPG2() first");
  const body = args.flat().map(b => b & 0x7F);
  pg2.send([...PG2_HEADER, cmd, ...body, 0xF7]);
}

// ---- High-level API ----
export const setGroup    = (idx)        => send(0x01, idx);
export const setSustain  = (level)      => send(0x02, level);

export function cwSetChord(chordIdx, notes /* length 6 */) {
  if (notes.length !== 6) throw new Error("need 6 notes");
  send(0x10, chordIdx, notes);
}

export function cwSetName(chordIdx, name) {
  const ascii = [...name].map(c => c.charCodeAt(0)).slice(0, 11);
  send(0x11, chordIdx, ascii.length, ascii);
}

export function cwSetOffset(semitones) {
  send(0x12, (semitones + 64) & 0x7F);
}

// ---- Convenience: upload a whole CW group at once ----
//
// chords = [
//   { name: "Daizy", notes: [40,45,50,55,59,64] },  // slot 0 (center)
//   { name: "C",     notes: [40,48,52,55,60,64] },  // slot 1 (Up)
//   ...                                              // 9 entries total
// ]
export function uploadCW(chords, offset = 0) {
  if (chords.length !== 9) throw new Error("need 9 chord slots");
  cwSetOffset(offset);
  chords.forEach((c, i) => {
    cwSetChord(i, c.notes);
    cwSetName(i, c.name);
  });
}

// ---- Activate ----
export const activateCW = () => setGroup(3);
```

### Usage

```js
import { connectPG2, uploadCW, activateCW } from "./pg2.js";

await connectPG2();

uploadCW([
  { name: "Daizy", notes: [40, 45, 50, 55, 59, 64] }, // 0 center
  { name: "C",     notes: [40, 48, 52, 55, 60, 64] }, // 1 Up
  { name: "G",     notes: [43, 47, 50, 55, 59, 67] }, // 2 NE
  { name: "Am",    notes: [40, 45, 52, 57, 60, 64] }, // 3 Right
  { name: "F",     notes: [41, 45, 53, 57, 60, 65] }, // 4 SE
  { name: "Em",    notes: [40, 47, 52, 55, 59, 64] }, // 5 Down
  { name: "Dm",    notes: [38, 45, 50, 57, 60, 65] }, // 6 SW
  { name: "G7",    notes: [43, 47, 50, 55, 59, 65] }, // 7 Left
  { name: "C/E",   notes: [40, 48, 52, 55, 60, 64] }, // 8 NW
], 0);

activateCW();
```

---

## 6. Behavioural notes

- **CW writes are not persisted.** A power cycle resets CW to its built-in
  default (a copy of the GH jazz set). Re-upload after each reconnect.
- **The device always starts in LOCAL audio mode** when first powered on.
  After USB MIDI is enumerated and SysEx is received, it switches to USB
  mode (one-way; restart the app to return to LOCAL).
- **No ack or response.** v1 is fire-and-forget; the device does not echo
  anything back. If you need verification, send `setGroup(3)` and visually
  confirm the popup. A query/report command may be added in v2.
- **Latency.** SysEx is parsed in the main render loop, so commands take
  effect within ~10 ms (one frame).
- **Burst sending is fine.** The 128-byte rx buffer is more than enough
  for the largest single command (~19 bytes), and the parser drains the
  USB rx queue completely each frame.

---

## 7. Cheat sheet

| What you want | Bytes (hex) |
|---|---|
| Switch to CW group | `F0 7D 50 47 32 01 03 F7` |
| Set sustain to 1500 ms | `F0 7D 50 47 32 02 03 F7` |
| CW slot 1 = E A D G B E | `F0 7D 50 47 32 10 01 28 2D 32 37 3B 40 F7` |
| Name CW slot 1 "Open" | `F0 7D 50 47 32 11 01 04 4F 70 65 6E F7` |
| CW capo +4 | `F0 7D 50 47 32 12 44 F7` |
| CW capo 0 | `F0 7D 50 47 32 12 40 F7` |
