# -*- coding: utf-8 -*-
from PIL import Image, ImageDraw, ImageFont
import os


ROOT = r"C:\Users\86177\Desktop\ESP32_chukong\chu_kong_git\lvgl_display_test_2"
OUT_DIR = os.path.join(ROOT, "docs")
OUT_PATH = os.path.join(OUT_DIR, "ota_layers_largefont.png")


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


W, H = 1600, 1200
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
    text = text.replace("\n", "，")
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


def fit(text, max_w, max_h, bold=False, max_size=42, min_size=20):
    for size in range(max_size, min_size - 1, -1):
        fnt = font(size, bold)
        spacing = max(5, size // 5)
        wrapped = wrap(text, fnt, max_w)
        tw, th = text_size(wrapped, fnt, spacing)
        if tw <= max_w and th <= max_h:
            return wrapped, fnt, spacing
    fnt = font(min_size, bold)
    return wrap(text, fnt, max_w), fnt, 5


def text_box(x, y, w, h, title, body=None, max_title=42, max_body=32):
    d.rounded_rectangle((x, y, x + w, y + h), radius=13, fill=BOX, outline=LINE, width=3)
    if body:
        title_y = y + 46
        body_top = y + 78
        body_bottom = y + h - 18
        d.text((x + w / 2, title_y), title, font=font(max_title, True), fill=BLACK, anchor="mm")
        wrapped, fnt, spacing = fit(body, w - 60, body_bottom - body_top, False, max_body, 22)
        d.multiline_text((x + w / 2, (body_top + body_bottom) / 2), wrapped, font=fnt, fill=BLACK,
                         anchor="mm", align="center", spacing=spacing)
    else:
        wrapped, fnt, spacing = fit(title, w - 50, h - 28, True, max_title, 20)
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
        d.rectangle((lx - tw / 2 - 8, ly - th / 2 - 5, lx + tw / 2 + 8, ly + th / 2 + 5), fill=LAYER)
        d.text((lx, ly), label, font=fnt, fill=BLACK, anchor="mm")


layer(18, 245, "升级结果与运行保护")
layer(330, 300, "OTA执行与分区切换")
layer(700, 430, "升级输入路径")

text_box(470, 92, 660, 128, "新固件确认与回滚",
         "重启进入新固件后确认有效；异常时回滚上一可用分区",
         max_title=38, max_body=30)
text_box(380, 430, 840, 170, "备用分区写入与启动切换",
         "写入备份分区，完成校验后设置下次启动分区，重启或点击跳转按钮",
         max_title=38, max_body=32)
text_box(190, 835, 430, 240, "云端OTA",
         "接收OneNet升级任务，上报版本与状态，下载新固件镜像",
         max_title=38, max_body=30)
text_box(980, 835, 430, 240, "本地OTA（SD卡）",
         "选择SD卡.bin固件包，按数据块读取，等待写入备用分区",
         max_title=38, max_body=30)

arrow(585, 835, 585, 600, "写入备份分区", (670, 705))
arrow(1015, 835, 1015, 600, "写入备份分区", (1105, 705))
arrow(800, 430, 800, 220, "重启或点击跳转按钮", (930, 315))

os.makedirs(OUT_DIR, exist_ok=True)
img.save(OUT_PATH, quality=95)
print(OUT_PATH)

