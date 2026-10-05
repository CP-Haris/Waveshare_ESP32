"""Generate the app icon, adaptive icon, splash image and favicon.

Carbon Blue mark: a 270° gauge (graphite track + Clayton-blue fill) with the
"C" from the Clayton Power wordmark in the centre. The splash shows the full
wordmark. Source logo: ClaytonDisplay/docs/SplashLogo.webp (white on
transparent).

Usage (from ClaytonPowerApp/):  python tools/gen_app_icons.py
"""
from collections import deque
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
LOGO = ROOT.parent / "ClaytonDisplay" / "docs" / "SplashLogo.webp"
ASSETS = ROOT / "assets"

BG = (0x0B, 0x0C, 0x0E, 255)
TRACK = (0x2A, 0x2D, 0x32, 255)  # a step above the UI track so it reads at icon size
BLUE = (0x4E, 0x9E, 0xEB, 255)
INK = (0xEF, 0xED, 0xE8, 255)

SIZE = 1024
SS = 4            # supersampling for smooth arcs
FILL = 0.76       # gauge fill, as in the design spec's reference frame


def letter_c_mask():
    """Alpha mask of the leftmost connected shape (the "C") of the wordmark."""
    alpha = Image.open(LOGO).convert("RGBA").getchannel("A")
    a = np.array(alpha)
    solid = a > 60
    h, w = solid.shape
    seed = next((y, x) for x in range(w) for y in range(h) if solid[y, x])
    seen = np.zeros_like(solid)
    seen[seed] = True
    q = deque([seed])
    while q:
        y, x = q.popleft()
        for ny, nx in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)):
            if 0 <= ny < h and 0 <= nx < w and solid[ny, nx] and not seen[ny, nx]:
                seen[ny, nx] = True
                q.append((ny, nx))
    ys, xs = np.nonzero(seen)
    x0, x1, y0, y1 = xs.min() - 3, xs.max() + 4, ys.min() - 3, ys.max() + 4
    # Keep anti-aliased edge pixels, but only around this letter
    region = np.zeros_like(a)
    grown = seen.copy()
    for _ in range(3):
        g = grown.copy()
        g[1:] |= grown[:-1]; g[:-1] |= grown[1:]; g[:, 1:] |= grown[:, :-1]; g[:, :-1] |= grown[:, 1:]
        grown = g
    region[grown] = a[grown]
    return Image.fromarray(region).crop((x0, y0, x1, y1))


def draw_mark(scale, transparent):
    """Gauge + C on a SIZE canvas; scale shrinks the mark (adaptive safe zone)."""
    s = SIZE * SS
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0) if transparent else BG)
    d = ImageDraw.Draw(img)

    r = 330 * SS * scale                 # arc centre-line radius
    stroke = round(r / 24 * 6)           # same ratio as the 64-unit dial
    c = s / 2
    box = [c - r - stroke / 2, c - r - stroke / 2, c + r + stroke / 2, c + r + stroke / 2]
    # PIL angles run clockwise from 3 o'clock: the gauge starts bottom-left (135°)
    d.arc(box, 135, 135 + 270, fill=TRACK, width=stroke)
    d.arc(box, 135, 135 + 270 * FILL, fill=BLUE, width=stroke)

    c_mask = letter_c_mask()
    cw = round(330 * SS * scale)
    ch = round(c_mask.height * cw / c_mask.width)
    c_mask = c_mask.resize((cw, ch), Image.LANCZOS)
    ink = Image.new("RGBA", (cw, ch), INK)
    # Optical centring: the italic C sits slightly right of its box centre
    img.paste(ink, (round(c - cw / 2 - cw * 0.03), round(c - ch / 2)), c_mask)

    return img.resize((SIZE, SIZE), Image.LANCZOS)


def splash():
    alpha = Image.open(LOGO).convert("RGBA")
    logo = alpha.crop(alpha.getchannel("A").getbbox())
    w = 1000
    logo = logo.resize((w, round(logo.height * w / logo.width)), Image.LANCZOS)
    canvas = Image.new("RGBA", (1200, 1200), (0, 0, 0, 0))
    canvas.paste(logo, ((1200 - logo.width) // 2, (1200 - logo.height) // 2), logo)
    return canvas


def notification_icon():
    """Android status-bar icon: the same mark as a white silhouette on
    transparent (Android only uses the alpha channel)."""
    mark = draw_mark(0.95, transparent=True)
    alpha = mark.getchannel("A")
    white = Image.new("RGBA", mark.size, (255, 255, 255, 0))
    white.putalpha(alpha)
    return white.resize((96, 96), Image.LANCZOS)


def main():
    icon = draw_mark(1.0, transparent=False)
    icon.convert("RGB").save(ASSETS / "icon.png")
    # Android adaptive icons crop to a circle/squircle of ~66 % — keep the
    # whole gauge inside it.
    draw_mark(0.72, transparent=True).save(ASSETS / "adaptive-icon.png")
    splash().save(ASSETS / "splash-icon.png")
    icon.resize((48, 48), Image.LANCZOS).convert("RGB").save(ASSETS / "favicon.png")
    notification_icon().save(ASSETS / "notification-icon.png")
    print("Wrote icon.png, adaptive-icon.png, splash-icon.png, favicon.png, notification-icon.png")


if __name__ == "__main__":
    main()
