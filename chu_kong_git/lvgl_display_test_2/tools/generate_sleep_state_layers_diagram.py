# -*- coding: utf-8 -*-
from PIL import Image, ImageDraw, ImageFont
import os


ROOT = r"C:\Users\86177\Desktop\ESP32_chukong\chu_kong_git\lvgl_display_test_2"
OUT_DIR = os.path.join(ROOT, "docs")
OUT_PATH = os.path.join(OUT_DIR, "sleep_state_layers_largefont.png")


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


def card(x, y, w, h, title, body, title_size=31, body_size=24):
    d.rounded_rectangle((x, y, x + w, y + h), radius=13, fill=BOX, outline=LINE, width=3)
    d.text((x + w / 2, y + 40), title, font=font(title_size, True), fill=BLACK, anchor="mm")
    body_top = y + 72
    body_bottom = y + h - 18
    wrapped, fnt, spacing = fit(body, w - 44, body_bottom - body_top, False, body_size, 18)
    d.multiline_text((x + w / 2, (body_top + body_bottom) / 2), wrapped, font=fnt,
                     fill=BLACK, anchor="mm", align="center", spacing=spacing)


def arrow(x1, y1, x2, y2, label=None, label_pos=None):
    d.line((x1, y1, x2, y2), fill=LINE, width=4)
    if y2 < y1:
        pts = [(x2, y2), (x2 - 13, y2 + 28), (x2 + 13, y2 + 28)]
    else:
        pts = [(x2, y2), (x2 - 13, y2 - 28), (x2 + 13, y2 - 28)]
    d.polygon(pts, fill=LINE)
    if label:
        fnt = font(22, True)
        lx, ly = label_pos if label_pos else ((x1 + x2) / 2, (y1 + y2) / 2)
        tw, th = text_size(label, fnt)
        d.rectangle((lx - tw / 2 - 8, ly - th / 2 - 4,
                     lx + tw / 2 + 8, ly + th / 2 + 4), fill=LAYER)
        d.text((lx, ly), label, font=fnt, fill=BLACK, anchor="mm")


layer(18, 245, "唤醒与恢复结果",
      "根据休眠模式执行快速恢复或重新启动。")
layer(318, 245, "休眠执行状态",
      "执行外设关闭、状态保存和唤醒源配置。")
layer(618, 245, "触发条件",
      "根据用户操作和空闲时间决定进入不同低功耗模式。")

box_w, box_h = 430, 150
left_x, right_x = 455, 1085
top_y, mid_y, bot_y = 74, 371, 674

card(left_x, top_y, box_w, box_h,
     "轻休眠唤醒恢复",
     "快速恢复显示与交互上下文")
card(right_x, top_y, box_w, box_h,
     "深休眠唤醒启动",
     "由唤醒引脚触发后重新启动系统")

card(left_x, mid_y, box_w, box_h,
     "Light Sleep",
     "关闭背光并暂停高功耗任务，唤醒后恢复显示与无线状态")
card(right_x, mid_y, box_w, box_h,
     "Deep Sleep",
     "关闭显示、无线与外设供电路径，配置RTC唤醒引脚等待硬件唤醒")

card(left_x, bot_y, box_w, box_h,
     "自动触发",
     "超过120s无操作，进入轻休眠并保持快速唤醒能力")
card(right_x, bot_y, box_w, box_h,
     "按键触发",
     "确认键长按约3s，进入深休眠控制，唤醒后冷启动")

cx_l = left_x + box_w / 2
cx_r = right_x + box_w / 2

arrow(cx_l, bot_y, cx_l, mid_y + box_h, "进入Light Sleep", (cx_l, 596))
arrow(cx_r, bot_y, cx_r, mid_y + box_h, "进入Deep Sleep", (cx_r, 596))
arrow(cx_l, mid_y, cx_l, top_y + box_h, "触摸/按键唤醒", (cx_l, 300))
arrow(cx_r, mid_y, cx_r, top_y + box_h, "确认键唤醒", (cx_r, 300))

os.makedirs(OUT_DIR, exist_ok=True)
img.save(OUT_PATH, quality=95)
print(OUT_PATH)
