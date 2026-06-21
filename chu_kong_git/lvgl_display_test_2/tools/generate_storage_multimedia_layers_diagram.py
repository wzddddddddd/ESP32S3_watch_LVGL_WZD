# -*- coding: utf-8 -*-
from PIL import Image, ImageDraw, ImageFont
import os


ROOT = r"C:\Users\86177\Desktop\ESP32_chukong\chu_kong_git\lvgl_display_test_2"
OUT_DIR = os.path.join(ROOT, "docs")
OUT_PATH = os.path.join(OUT_DIR, "storage_multimedia_layers_largefont.png")


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


W, H = 1700, 900
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


def fit(text, max_w, max_h, bold=False, max_size=38, min_size=18):
    for size in range(max_size, min_size - 1, -1):
        fnt = font(size, bold)
        spacing = max(5, size // 5)
        wrapped = wrap(text, fnt, max_w)
        tw, th = text_size(wrapped, fnt, spacing)
        if tw <= max_w and th <= max_h:
            return wrapped, fnt, spacing
    fnt = font(min_size, bold)
    return wrap(text, fnt, max_w), fnt, 5


def layer(y, h, title, desc):
    d.rounded_rectangle((18, y, W - 18, y + h), radius=18, fill=LAYER, outline="#B8C4D1", width=3)
    d.text((45, y + 46), title, font=font(34, True), fill=BLACK, anchor="lm")
    wrapped, fnt, spacing = fit(desc, 330, h - 90, False, 24, 18)
    d.multiline_text((45, y + 88), wrapped, font=fnt, fill=BLACK, anchor="la",
                     spacing=spacing, align="left")


def card(x, y, w, h, title, body, title_size=30, body_size=24):
    d.rounded_rectangle((x, y, x + w, y + h), radius=13, fill=BOX, outline=LINE, width=3)
    d.text((x + w / 2, y + 40), title, font=font(title_size, True), fill=BLACK, anchor="mm")
    body_top = y + 72
    body_bottom = y + h - 18
    wrapped, fnt, spacing = fit(body, w - 44, body_bottom - body_top, False, body_size, 18)
    d.multiline_text((x + w / 2, (body_top + body_bottom) / 2), wrapped, font=fnt, fill=BLACK,
                     anchor="mm", align="center", spacing=spacing)


def arrow(x1, y1, x2, y2, label=None, label_pos=None, dashed=False):
    if dashed:
        dash, gap = 18, 12
        dx, dy = x2 - x1, y2 - y1
        dist = (dx * dx + dy * dy) ** 0.5
        if dist:
            ux, uy = dx / dist, dy / dist
            p = 0
            while p < dist - 28:
                sx, sy = x1 + ux * p, y1 + uy * p
                ex = x1 + ux * min(dist - 28, p + dash)
                ey = y1 + uy * min(dist - 28, p + dash)
                d.line((sx, sy, ex, ey), fill=LINE, width=4)
                p += dash + gap
    else:
        d.line((x1, y1, x2, y2), fill=LINE, width=4)

    if abs(y2 - y1) >= abs(x2 - x1):
        if y2 < y1:
            pts = [(x2, y2), (x2 - 13, y2 + 28), (x2 + 13, y2 + 28)]
        else:
            pts = [(x2, y2), (x2 - 13, y2 - 28), (x2 + 13, y2 - 28)]
    else:
        if x2 < x1:
            pts = [(x2, y2), (x2 + 28, y2 - 13), (x2 + 28, y2 + 13)]
        else:
            pts = [(x2, y2), (x2 - 28, y2 - 13), (x2 - 28, y2 + 13)]
    d.polygon(pts, fill=LINE)

    if label:
        fnt = font(22, True)
        lx, ly = label_pos if label_pos else ((x1 + x2) / 2, (y1 + y2) / 2)
        tw, th = text_size(label, fnt)
        d.rectangle((lx - tw / 2 - 8, ly - th / 2 - 4,
                     lx + tw / 2 + 8, ly + th / 2 + 4), fill=LAYER)
        d.text((lx, ly), label, font=fnt, fill=BLACK, anchor="mm")


layer(18, 245, "功能显示与使用效果",
      "面向用户呈现图片、视频和小说资源的最终使用效果。")
layer(318, 245, "应用模块与交互",
      "负责资源解析、页面刷新和交互状态维护，将底层数据转换为可显示内容。")
layer(618, 245, "存储基础能力",
      "提供文件来源、目录管理和连续读取能力，为多媒体应用提供统一数据输入。")

xs = [430, 850, 1270]
w, h = 350, 150

top_y = 74
mid_y = 371
bot_y = 674

top_cards = [
    ("图片/壁纸显示", "表盘壁纸切换、图片浏览与静态图像呈现"),
    ("视频播放显示", "连续帧画面刷新，提升视频播放体验"),
    ("小说阅读显示", "分页文本展示，支持断点续读恢复"),
]
for x, (title, body) in zip(xs, top_cards):
    card(x, top_y, w, h, title, body, 31, 24)

mid_cards = [
    ("图片与壁纸模块", "分块读取图片，完成解码、缓存和控件映射"),
    ("视频显示模块", "读取视频帧并解码，通知显示任务刷新画面"),
    ("小说显示模块", "按页读取文本，维护路径、偏移量和进度"),
]
for x, (title, body) in zip(xs, mid_cards):
    card(x, mid_y, w, h, title, body, 30, 23)

bottom_cards = [
    ("SD/TF外置存储", "保存图片、视频、小说等大容量资源"),
    ("FatFS文件系统", "统一目录扫描、文件访问和资源列表管理"),
    ("文件读取与索引", "支持流式读取、路径检索和偏移定位"),
]
for x, (title, body) in zip(xs, bottom_cards):
    card(x, bot_y, w, h, title, body, 30, 23)

# 功能显示由对应应用模块直接驱动。
for x, label in zip([605, 1025, 1445], ["图片/壁纸显示", "视频画面显示", "小说页面显示"]):
    arrow(x, mid_y, x, top_y + h, label, (x, 300))

# 底层存储能力按文件系统链路逐级提供数据。
arrow(780, bot_y + h / 2, 850, bot_y + h / 2)
arrow(1200, bot_y + h / 2, 1270, bot_y + h / 2)

# 文件读取与索引分别支撑上层三个应用模块。
arrow(1445, bot_y, 605, mid_y + h, dashed=True)
arrow(1445, bot_y, 1025, mid_y + h, dashed=True)
arrow(1445, bot_y, 1445, mid_y + h, dashed=True)

os.makedirs(OUT_DIR, exist_ok=True)
img.save(OUT_PATH, quality=95)
print(OUT_PATH)
