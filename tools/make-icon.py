"""Generates src/pingy.ico.

The icon is a latency trace on the app's own background, drawn with the same
Catmull-Rom interpolation graph.cpp uses, in the same red as Theme::PrimaryRed.

Each size is rendered independently at 4x and downsampled, rather than scaling
one large image. A stroke width that reads at 256px disappears at 16px, and a
width that reads at 16px is a slab at 256px.

    python tools/make-icon.py
"""

import io
import struct
from pathlib import Path

from PIL import Image, ImageDraw

SIZES = [16, 24, 32, 48, 64, 128, 256]
OUT = Path(__file__).resolve().parent.parent / "src" / "pingy.ico"

# Theme::Background, Theme::Border and Theme::PrimaryRed, converted from the
# float triples in src/theme.h.
BG = (10, 10, 10, 255)
BORDER = (51, 51, 51, 255)
RED = (255, 23, 68, 255)
HALO_ALPHA = 60

# A plausible latency trace: mostly low, one spike, settling. Normalized, with
# y running down the way the screen does.
POINTS = [
    (0.09, 0.63),
    (0.27, 0.47),
    (0.41, 0.71),
    (0.57, 0.28),
    (0.74, 0.54),
    (0.91, 0.40),
]


def catmull_rom(points, steps=24):
    """Same spline the graph draws, with the ends duplicated as phantom points."""
    p = [points[0]] + list(points) + [points[-1]]
    out = []
    for i in range(len(p) - 3):
        p0, p1, p2, p3 = p[i], p[i + 1], p[i + 2], p[i + 3]
        for s in range(steps):
            t = s / steps
            t2, t3 = t * t, t * t * t
            out.append(
                tuple(
                    0.5
                    * (
                        2 * p1[c]
                        + (-p0[c] + p2[c]) * t
                        + (2 * p0[c] - 5 * p1[c] + 4 * p2[c] - p3[c]) * t2
                        + (-p0[c] + 3 * p1[c] - 3 * p2[c] + p3[c]) * t3
                    )
                    for c in (0, 1)
                )
            )
    out.append(points[-1])
    return out


def render(size):
    scale = 4
    s = size * scale
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    draw.rounded_rectangle(
        [0, 0, s - 1, s - 1],
        radius=s * 0.18,
        fill=BG,
        outline=BORDER,
        width=max(scale, int(s * 0.014)),
    )

    curve = [(x * s, y * s) for x, y in catmull_rom(POINTS)]
    width = max(scale, int(s * 0.045))

    # The halo goes on its own layer at full alpha and is faded once, rather
    # than being drawn translucent directly. A wide translucent line is stamped
    # as overlapping segments, and every overlap composites twice, which stripes
    # the result.
    halo = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    ImageDraw.Draw(halo).line(curve, fill=RED, width=int(width * 2.1), joint="curve")
    halo.putalpha(halo.getchannel("A").point(lambda a: a * HALO_ALPHA // 255))
    img.alpha_composite(halo)

    draw.line(curve, fill=RED, width=width, joint="curve")

    # The current reading, which is the one number anyone is actually watching.
    ex, ey = curve[-1]
    dot = width * 1.15
    draw.ellipse([ex - dot, ey - dot, ex + dot, ey + dot], fill=(255, 255, 255, 255))

    return img.resize((size, size), Image.LANCZOS)


def main():
    # The container is written directly rather than through Image.save(format=
    # "ICO"), which resizes one base image and drops the per-size renders.
    #
    # Every entry is PNG-compressed, supported for icon entries of any size
    # since Vista. Uncompressed DIB entries would add ~100KB of resource to a
    # 52KB binary, which for this project is not a rounding error.
    # Each entry is encoded both as 32-bit RGBA and as a 256-colour palette, and
    # the smaller wins. The art is about five colours plus antialiasing, so the
    # palette is 5x smaller at 256px and slightly larger at 16px. Entries in one
    # icon are independent PNGs, so mixing the two is fine.
    blobs = []
    for size in SIZES:
        frame = render(size)
        rgba = io.BytesIO()
        frame.save(rgba, format="PNG", optimize=True)
        palette = io.BytesIO()
        frame.quantize(colors=256, method=Image.FASTOCTREE).save(
            palette, format="PNG", optimize=True
        )
        blobs.append(min(rgba.getvalue(), palette.getvalue(), key=len))

    header = struct.pack("<HHH", 0, 1, len(blobs))  # reserved, type=icon, count
    offset = len(header) + 16 * len(blobs)
    entries = b""
    for size, blob in zip(SIZES, blobs):
        entries += struct.pack(
            "<BBBBHHII",
            size if size < 256 else 0,  # 0 means 256
            size if size < 256 else 0,
            0,  # palette size, 0 for truecolour
            0,  # reserved
            1,  # colour planes
            32,  # bits per pixel
            len(blob),
            offset,
        )
        offset += len(blob)

    OUT.write_bytes(header + entries + b"".join(blobs))
    print(f"wrote {OUT} ({OUT.stat().st_size} bytes) with sizes {SIZES}")


if __name__ == "__main__":
    main()
