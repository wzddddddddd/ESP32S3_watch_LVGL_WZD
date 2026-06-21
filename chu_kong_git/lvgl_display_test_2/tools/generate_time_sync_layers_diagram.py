# -*- coding: utf-8 -*-
from PIL import Image, ImageDraw, ImageFont
import os


ROOT = r"C:\Users\86177\Desktop\ESP32_chukong\chu_kong_git\lvgl_display_test_2"
OUT_DIR = os.path.join(ROOT, "docs")
OUT_PATH = os.path.join(OUT_DIR, "time_sync_layers_largefont.png")


def find_font():
    for path in [
        r"C:\Windows\Fonts\msyh.ttc",
        r"C:\Windows\Fonts\simhei.ttf",
        r"C:\Windows\Fonts\simsun.ttc",
    ]:
        if os.path.exists(path):
            return path
    return r"C:\Windows\Fonts\arial.ttf"


FONT_PATH = find_font()


def font(size, bold=False):
    if bold:
        for path in [r"C:\Windows\Fonts\msyhbd.ttc", r"C:\Windows\Fonts\simhei.ttf"]:
            if os.path.exists(path):
                return ImageFont.truetype(path, size)
    return ImageFont.truetype(FONT_PATH, size)


W, H = 1700, 1320
img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)

BLACK = "#0F1720"
LINE = "#5A6B7F"
LAYER = "#F2F6FA"
BOX = "#CAD7E6"


def text_size(text, fnt, spacing=6):
    bb = d.multiline_textbbox((0, 0), text, font=fnt, spacing=spacing, align="center")
    return bb[2] - bb[0], bb[3] - bb[1]


def wrap(text, fnt, max_w):
    lines = []
    for raw in text.split("\n"):
        line = ""
        for ch in raw:
            test = line + ch
            if text_size(test, fnt)[0] <= max_w:
                line = test
            else:
                if line:
                    lines.append(line)
                line = ch
        if line:
            lines.append(line)
    return "\n".join(lines)


def fit(text, max_w, max_h, bold=False, max_size=42, min_size=18):
    for size in range(max_size, min_size - 1, -1):
        fnt = font(size, bold)
        spacing = max(5, size // 5)
        wrapped = wrap(text, fnt, max_w)
        tw, th = text_size(wrapped, fnt, spacing)
        if tw <= max_w and th <= max_h:
            return wrapped, fnt, spacing
    fnt = font(min_size, bold)
    return wrap(text, fnt, max_w), fnt, 5


def draw_text_center(x, y, w, h, title, body=None, max_title=38, max_body=30):
    d.rounded_rectangle((x, y, x + w, y + h), radius=13, fill=BOX, outline=LINE, width=3)
    if body:
        d.text((x + w / 2, y + 42), title, font=font(max_title, True), fill=BLACK, anchor="mm")
        wrapped, fnt, spacing = fit(body, w - 54, h - 78, False, max_body, 18)
        d.multiline_text((x + w / 2, y + 92), wrapped, font=fnt, fill=BLACK,
                         anchor="ma", align="center", spacing=spacing)
    else:
        wrapped, fnt, spacing = fit(title, w - 44, h - 24, True, max_title, 20)
        d.multiline_text((x + w / 2, y + h / 2), wrapped, font=fnt, fill=BLACK,
                         anchor="mm", align="center", spacing=spacing)


def layer(y, h, title):
    d.rounded_rectangle((18, y, W - 18, y + h), radius=18, fill=LAYER, outline="#B8C4D1", width=3)
    d.text((45, y + 38), title, font=font(38, True), fill=BLACK, anchor="lm")


def arrow(x1, y1, x2, y2, label=None, label_pos=None):
    d.line((x1, y1, x2, y2), fill=LINE, width=4)
    if y2 < y1:
        pts = [(x2, y2), (x2 - 13, y2 + 28), (x2 + 13, y2 + 28)]
    else:
        pts = [(x2, y2), (x2 - 13, y2 - 28), (x2 + 13, y2 - 28)]
    d.polygon(pts, fill=LINE)
    if label:
        fnt = font(24, True)
        lx, ly = label_pos if label_pos else ((x1 + x2) / 2, (y1 + y2) / 2)
        tw, th = text_size(label, fnt)
        d.rectangle((lx - tw / 2 - 8, ly - th / 2 - 4, lx + tw / 2 + 8, ly + th / 2 + 4), fill=LAYER)
        d.text((lx, ly), label, font=fnt, fill=BLACK, anchor="mm")


layer(18, 275, "显示系统时间")
layer(330, 345, "时钟模块")
layer(720, 545, "时间输入")

top_x, top_y, top_w, top_h = 260, 105, 1180, 155
rtc_x, rtc_y, rtc_w, rtc_h = 390, 455, 920, 165
left_x, input_y, input_w, input_h = 140, 900, 500, 260
right_x = 1060

draw_text_center(top_x, top_y, top_w, top_h,
                 "显示系统时间",
                 "用于时间、日期等状态显示",
                 max_title=46, max_body=34)

draw_text_center(rtc_x, rtc_y, rtc_w, rtc_h,
                 "SD3078实时时钟（RTC）",
                 "统一保存网络校时与手动设置结果，周期校正系统时间",
                 max_title=44, max_body=34)

draw_text_center(left_x, input_y, input_w, input_h,
                 "WiFi网络校时",
                 "连接上一次热点，定时同步标准时间，写入RTC并刷新显示时间",
                 max_title=42, max_body=34)

draw_text_center(right_x, input_y, input_w, input_h,
                 "手动时间设置",
                 "用户在界面中修改时间，写入RTC并同步显示系统时间",
                 max_title=42, max_body=34)

# 两类时间输入写入RTC。
arrow(440, input_y, 440, rtc_y + rtc_h, "写入RTC", (360, 750))
arrow(1260, input_y, 1260, rtc_y + rtc_h, "写入RTC", (1180, 750))

# RTC周期校正显示时间。
arrow(850, rtc_y, 850, top_y + top_h, "周期校正", (940, 345))

# 两类输入也同步刷新显示层，箭头直接指向上方显示方框底边。
arrow(300, input_y, 300, top_y + top_h, "同步显示", (210, 565))
arrow(1400, input_y, 1400, top_y + top_h, "同步显示", (1490, 565))

os.makedirs(OUT_DIR, exist_ok=True)
img.save(OUT_PATH, quality=95)
print(OUT_PATH)
