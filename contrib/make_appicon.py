#!/usr/bin/env python3
"""Turn a square source image into BestWallet's round app icon set.

Trims any flat border (the black corners left behind by a rounded-square
export), masks the artwork to a circle with an antialiased edge, and writes
every size the app and packaging need.

    python3 contrib/make_appicon.py path/to/icon.png

Add --dry-run to render into a scratch directory instead of overwriting the
committed assets.
"""

import argparse
import os
import sys

try:
    from PIL import Image, ImageDraw
except ImportError:
    sys.exit("Pillow is required: pip install --user Pillow")

# Sizes referenced from assets.qrc / the desktop entry / packaging.
PNG_SIZES = [32, 48, 64, 96, 128, 256, 512]
ICO_SIZES = [16, 24, 32, 48, 64, 128, 256]
# Supersampling factor for the circular mask, so the edge is smooth.
SS = 8


def trim_border(im, tolerance=12):
    """Drop a uniform border (e.g. the black outside a rounded square)."""
    rgb = im.convert("RGB")
    w, h = rgb.size
    corner = rgb.getpixel((0, 0))

    def flat(px):
        return all(abs(a - b) <= tolerance for a, b in zip(px, corner))

    left, right, top, bottom = 0, w - 1, 0, h - 1
    while left < right and all(flat(rgb.getpixel((left, y))) for y in range(h)):
        left += 1
    while right > left and all(flat(rgb.getpixel((right, y))) for y in range(h)):
        right -= 1
    while top < bottom and all(flat(rgb.getpixel((x, top))) for x in range(w)):
        top += 1
    while bottom > top and all(flat(rgb.getpixel((x, bottom))) for x in range(w)):
        bottom -= 1

    if right - left < 8 or bottom - top < 8:
        return im  # nothing sensible to trim
    return im.crop((left, top, right + 1, bottom + 1))


def to_square(im):
    """Centre-crop to a square so the circular mask isn't distorted."""
    w, h = im.size
    if w == h:
        return im
    side = min(w, h)
    return im.crop(((w - side) // 2, (h - side) // 2,
                    (w - side) // 2 + side, (h - side) // 2 + side))


def round_icon(im, size):
    """Render the artwork into a circle of `size` px with a soft edge."""
    big = size * SS
    art = im.convert("RGBA").resize((big, big), Image.LANCZOS)

    mask = Image.new("L", (big, big), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, big - 1, big - 1), fill=255)

    # Keep whatever transparency the source already had.
    alpha = art.getchannel("A").point(lambda v: v)
    mask = Image.composite(mask, Image.new("L", (big, big), 0), alpha)

    out = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    out.paste(art, (0, 0), mask)
    return out.resize((size, size), Image.LANCZOS)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("source")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--out", default=None)
    args = ap.parse_args()

    here = os.path.dirname(os.path.abspath(__file__))
    default_out = os.path.join(here, os.pardir, "src", "assets", "images", "appicons")
    out_dir = args.out or (os.path.join(here, os.pardir, "appicon_preview")
                           if args.dry_run else default_out)
    out_dir = os.path.abspath(out_dir)
    os.makedirs(out_dir, exist_ok=True)

    src = Image.open(args.source)
    print(f"source      : {args.source} {src.size} {src.mode}")
    src = to_square(trim_border(src))
    print(f"after trim  : {src.size}")

    for size in PNG_SIZES:
        path = os.path.join(out_dir, f"{size}x{size}.png")
        round_icon(src, size).save(path)
        print(f"wrote {path}")

    ico = os.path.join(out_dir, "appicon.ico")
    round_icon(src, 256).save(ico, sizes=[(s, s) for s in ICO_SIZES])
    print(f"wrote {ico}")

    print("\nDone. Rebuild so the Qt resources are re-embedded.")


if __name__ == "__main__":
    main()
