"""Generate res/gamerTCP.ico: a dark rounded square with the ZGT wordmark."""

import io
import struct

from PIL import Image, ImageDraw, ImageFilter, ImageFont

FONT_PATH = r"C:\Windows\Fonts\seguibl.ttf"
ICON_PATH = r"res\gamerTCP.ico"
PREVIEW_PATH = r"build\icon-preview.png"
SIZES = [16, 24, 32, 48, 64, 128, 256]
SS = 4  # supersample factor for every size

ACCENT_TOP = (128, 232, 255)
ACCENT_BOTTOM = (186, 158, 255)
BG_TOP = (22, 38, 68)
BG_BOTTOM = (6, 10, 19)


def vertical_gradient(size, top, bottom, horizontal=False):
    strip = Image.new("RGB", (1, size))
    px = strip.load()
    for y in range(size):
        t = y / (size - 1) if size > 1 else 0.0
        px[0, y] = tuple(round(top[i] + (bottom[i] - top[i]) * t) for i in range(3))
    gradient = strip.resize((size, size), Image.NEAREST)
    return gradient.transpose(Image.ROTATE_90) if horizontal else gradient


def fit_font(size):
    target_width = size * 0.76
    best = ImageFont.truetype(FONT_PATH, 8)
    low, high = 8, size
    while low <= high:
        mid = (low + high) // 2
        font = ImageFont.truetype(FONT_PATH, mid)
        width = font.getlength("ZGT")
        if width <= target_width and font.getbbox("ZGT")[3] <= size * 0.64:
            best = font
            low = mid + 1
        else:
            high = mid - 1
    return best


def render(size):
    s = size * SS
    radius = int(s * 0.22)

    mask = Image.new("L", (s, s), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, s - 1, s - 1], radius=radius, fill=255)

    canvas = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    canvas.paste(vertical_gradient(s, BG_TOP, BG_BOTTOM), (0, 0), mask)

    border = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    width = max(1, s // 110)
    ImageDraw.Draw(border).rounded_rectangle(
        [width / 2, width / 2, s - 1 - width / 2, s - 1 - width / 2],
        radius=radius,
        outline=(90, 209, 255, 105),
        width=width,
    )
    canvas = Image.alpha_composite(canvas, border)

    font = fit_font(s)
    text_mask = Image.new("L", (s, s), 0)
    text_draw = ImageDraw.Draw(text_mask)
    origin = (s / 2, s * 0.53)
    text_draw.text(origin, "ZGT", font=font, fill=255, anchor="mm")

    text_layer = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    if size <= 32:
        # The gradient and shadow turn muddy at taskbar/tray sizes; a bright
        # flat fill keeps the letters legible instead.
        text_layer.paste((150, 232, 255, 255), (0, 0), text_mask)
    else:
        shadow = text_mask.filter(ImageFilter.GaussianBlur(s * 0.012))
        shadow_layer = Image.new("RGBA", (s, s), (0, 0, 0, 0))
        shadow_layer.putalpha(shadow.point(lambda value: int(value * 0.55)))
        shadow_layer = Image.composite(shadow_layer, Image.new("RGBA", (s, s), (0, 0, 0, 0)), mask)
        canvas.paste(shadow_layer, (0, int(s * 0.02)), shadow_layer)
        text_layer.paste(vertical_gradient(s, ACCENT_TOP, ACCENT_BOTTOM).convert("RGBA"),
                         (0, 0), text_mask)
    canvas = Image.alpha_composite(canvas, text_layer)

    return canvas.resize((size, size), Image.LANCZOS)


def write_ico(frames):
    entries = b""
    payloads = b""
    offset = 6 + 16 * len(frames)
    for size, frame in frames:
        buffer = io.BytesIO()
        frame.save(buffer, format="PNG", optimize=True)
        png = buffer.getvalue()
        dimension = 0 if size >= 256 else size
        entries += struct.pack(
            "<BBBBHHII", dimension, dimension, 0, 0, 1, 32, len(png), offset + len(payloads)
        )
        payloads += png
    return struct.pack("<HHH", 0, 1, len(frames)) + entries + payloads


def main():
    frames = [(size, render(size)) for size in SIZES]
    with open(ICON_PATH, "wb") as handle:
        handle.write(write_ico(frames))

    # Contact sheet: 256 at 1x, 32 at 4x, 16 at 8x, on a light and a dark row.
    tiles = [(frames[-1][1], 1), (frames[2][1], 4), (frames[0][1], 8)]
    gap = 24
    sheet_width = gap + sum(frame.width * factor + gap for frame, factor in tiles)
    sheet_height = gap * 3 + 256 + 128
    sheet = Image.new("RGBA", (sheet_width, sheet_height), (255, 255, 255, 255))
    dark = Image.new("RGBA", (sheet_width, 128), (10, 16, 28, 255))
    sheet.paste(dark, (0, gap * 2 + 256))
    for row in range(2):
        x = gap
        y = gap + (0 if row == 0 else 256 + gap)
        for frame, factor in tiles:
            scaled = frame.resize((frame.width * factor, frame.height * factor), Image.NEAREST)
            sheet.alpha_composite(scaled, (x, y))
            x += frame.width * factor + gap
    sheet.save(PREVIEW_PATH)


if __name__ == "__main__":
    main()
