#!/usr/bin/env python3
"""
Generate a TFT_eSPI / LovyanGFX .vlw smooth font file from a TTF.
Only the glyphs needed by the chord-name renderer are included so the
output is just a few KB.
"""
import freetype
import struct
import sys
from pathlib import Path

TTF        = Path(__file__).parent / "PowerGrotesk-Regular.ttf"
OUT        = Path(__file__).parent / "chord96.vlw"
FONT_PIXEL = 64  # render size in pixels
# Characters needed for chord names. Add anything you might display.
# Only the glyphs that appear in NAMES_STD / NAMES_GH chord tables.
# Open, C, Em, G, Dm, Am, G7, F, E7,
# Fmaj7, Em7, Am/D, Em7/D#, Asus2/C, Cmaj7, Am/E, C#
CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789#b/+-"

def be_u32(v):
    # VLW uses big-endian, signed ints (Java DataOutputStream.writeInt)
    return struct.pack(">i", int(v))

def main():
    face = freetype.Face(str(TTF))
    face.set_pixel_sizes(0, FONT_PIXEL)

    ascent  = face.size.ascender  >> 6
    descent = -(face.size.descender >> 6)  # store as positive

    # Deduplicate while keeping order, drop chars the font lacks
    seen = []
    for ch in CHARS:
        if ch in seen:
            continue
        if face.get_char_index(ch) == 0 and ch != " ":
            print(f"  warn: glyph missing for {ch!r}", file=sys.stderr)
            continue
        seen.append(ch)
    # LovyanGFX uses std::lower_bound on the unicode table → must be sorted.
    seen.sort(key=ord)

    glyphs = []  # (unicode, w, h, xadv, dY, dX, bitmap_bytes)
    for ch in seen:
        face.load_char(ch, freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_NORMAL)
        g  = face.glyph
        bm = g.bitmap
        w, h = bm.width, bm.rows

        # Repack bitmap row-major, dropping the row pitch padding.
        if w == 0 or h == 0:
            data = b""
        else:
            data = bytearray(w * h)
            buf  = bm.buffer
            for row in range(h):
                src = row * bm.pitch
                data[row * w : row * w + w] = bytes(buf[src : src + w])
            data = bytes(data)

        glyphs.append((
            ord(ch),
            h,
            w,
            g.advance.x >> 6,
            g.bitmap_top,   # dY: pixels from baseline up to top of bitmap
            g.bitmap_left,  # dX: left side bearing
            data,
        ))

    out = bytearray()
    # File header (6 × int32 BE)
    out += be_u32(len(glyphs))
    out += be_u32(11)            # format version
    out += be_u32(FONT_PIXEL)
    out += be_u32(0)             # padding (mboxY, unused)
    out += be_u32(ascent)
    out += be_u32(descent)

    # Per-glyph metric table (7 × int32 BE per glyph — 28 bytes)
    for cp, h, w, xadv, dy, dx, _ in glyphs:
        out += be_u32(cp)
        out += be_u32(h)
        out += be_u32(w)
        out += be_u32(xadv)
        out += be_u32(dy)
        out += be_u32(dx)
        out += be_u32(0)   # reserved / padding (LovyanGFX discards)

    # Bitmap blob, in metric order
    for *_, data in glyphs:
        out += data

    OUT.write_bytes(out)
    print(f"Wrote {OUT}  ({len(out)} bytes, {len(glyphs)} glyphs, "
          f"ascent={ascent}, descent={descent})")

if __name__ == "__main__":
    main()
