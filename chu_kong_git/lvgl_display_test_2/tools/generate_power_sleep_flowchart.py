# -*- coding: utf-8 -*-
from PIL import Image, ImageDraw, ImageFont
import os
import math


ROOT = r"C:\Users\86177\Desktop\ESP32_chukong\chu_kong_git\lvgl_display_test_2"
OUT_DIR = os.path.join(ROOT, "docs")
OUT_PATH = os.path.join(OUT_DIR, "section_4_5_1_power_sleep_flowchart_bw.png")


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


W, H = 2500, 3600
img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)

F_TITLE = font(76, True)
F_SUB = font(38)
F_BOX = font(42)
F_BOX_B = font(44, True)
F_SMALL = font(36)
F_TINY = font(34)

BLACK = "#111111"
GRAY = "#444444"
MID = "#EDEDED"
WHITE = "#FFFFFF"


def text_width(txt, fnt):
    if not txt:
        return 0
    return d.textbbox((0, 0), txt, font=fnt)[2]


def wrap_by_width(text, fnt, max_width):
    lines = []
    for raw in text.split("\n"):
        line = ""
        for ch in raw:
            test = line + ch
            if text_width(test, fnt) <= max_width:
                line = test
            else:
                if line:
                    lines.append(line)
                line = ch
        if line:
            lines.append(line)
    return "\n".join(lines)


def text_size(text, fnt, spacing=8):
    bb = d.multiline_textbbox((0, 0), text, font=fnt, spacing=spacing, align="center")
    return bb[2] - bb[0], bb[3] - bb[1]


def fit_wrapped_text(text, max_width, max_height, bold=False, max_size=54, min_size=24):
    for size in range(max_size, min_size - 1, -1):
        fnt = font(size, bold)
        spacing = max(4, size // 5)
        wrapped = wrap_by_width(text, fnt, max_width)
        tw, th = text_size(wrapped, fnt, spacing)
        if tw <= max_width and th <= max_height:
            return wrapped, fnt, spacing
    fnt = font(min_size, bold)
    spacing = max(4, min_size // 5)
    return wrap_by_width(text, fnt, max_width), fnt, spacing


def center_text(x, y, text, fnt, fill=BLACK, spacing=10):
    d.multiline_text((x, y), text, font=fnt, fill=fill, anchor="mm", align="center", spacing=spacing)


def box(x, y, w, h, text, fill=WHITE, fnt=F_BOX, width=4, rounded=False):
    if rounded:
        d.rounded_rectangle((x, y, x + w, y + h), radius=h // 2, fill=fill, outline=BLACK, width=width)
    else:
        d.rectangle((x, y, x + w, y + h), fill=fill, outline=BLACK, width=width)
    if fnt is F_BOX_B:
        wrapped, use_font, spacing = fit_wrapped_text(text, w - 38, h - 22, True, 56, 26)
    elif fnt is F_SMALL:
        wrapped, use_font, spacing = fit_wrapped_text(text, w - 38, h - 22, False, 50, 24)
    else:
        wrapped, use_font, spacing = fit_wrapped_text(text, w - 40, h - 22, False, 52, 24)
    center_text(x + w / 2, y + h / 2, wrapped, use_font, spacing=spacing)


def terminator(x, y, w, h, text):
    box(x, y, w, h, text, WHITE, F_BOX_B, 4, True)


def decision(cx, cy, w, h, text):
    pts = [(cx, cy - h / 2), (cx + w / 2, cy), (cx, cy + h / 2), (cx - w / 2, cy)]
    d.polygon(pts, fill=WHITE, outline=BLACK)
    d.line(pts + [pts[0]], fill=BLACK, width=4)
    wrapped, use_font, spacing = fit_wrapped_text(text, int(w * 0.64), int(h * 0.56), False, 42, 22)
    center_text(cx, cy, wrapped, use_font, spacing=spacing)


def arrow(x1, y1, x2, y2, label=None, label_dx=0, label_dy=0):
    d.line((x1, y1, x2, y2), fill=BLACK, width=4)
    ang = math.atan2(y2 - y1, x2 - x1)
    ah, aw = 20, 11
    p1 = (x2 - ah * math.cos(ang) + aw * math.sin(ang), y2 - ah * math.sin(ang) - aw * math.cos(ang))
    p2 = (x2 - ah * math.cos(ang) - aw * math.sin(ang) + 0, y2 - ah * math.sin(ang) + aw * math.cos(ang))
    d.polygon([(x2, y2), p1, p2], fill=BLACK)
    if label:
        lx = (x1 + x2) / 2 + label_dx
        ly = (y1 + y2) / 2 + label_dy
        tw = text_width(label, F_TINY)
        d.rectangle((lx - tw / 2 - 8, ly - 17, lx + tw / 2 + 8, ly + 17), fill=WHITE)
        d.text((lx, ly), label, font=F_TINY, fill=BLACK, anchor="mm")


def elbow(points, label=None, label_at=None):
    for i in range(len(points) - 1):
        x1, y1 = points[i]
        x2, y2 = points[i + 1]
        if i == len(points) - 2:
            arrow(x1, y1, x2, y2)
        else:
            d.line((x1, y1, x2, y2), fill=BLACK, width=4)
    if label and label_at:
        lx, ly = label_at
        tw = text_width(label, F_TINY)
        d.rectangle((lx - tw / 2 - 8, ly - 17, lx + tw / 2 + 8, ly + 17), fill=WHITE)
        d.text((lx, ly), label, font=F_TINY, fill=BLACK, anchor="mm")


# 标题
d.text((W / 2, 85), "4.5.1 多级休眠与动态唤醒控制逻辑流程图", fill=BLACK, font=F_TITLE, anchor="mm")
d.text((W / 2, 150), "自动轻休眠、按键深休眠、外设关断、唤醒恢复与系统重启", fill=GRAY, font=F_SUB, anchor="mm")
d.line((140, 195, W - 140, 195), fill=BLACK, width=3)

cx = W / 2
terminator(850, 245, 700, 92, "系统正常运行")
box(730, 390, 940, 116, "初始化低功耗管理任务\n记录最近一次用户活动时间", MID, F_SMALL)
box(730, 550, 940, 116, "周期检测空闲时间\n并监听确认键长按状态", WHITE, F_SMALL)
decision(cx, 790, 560, 150, "触发条件？")

arrow(cx, 337, cx, 390)
arrow(cx, 506, cx, 550)
arrow(cx, 666, cx, 715)

left_x, right_x = 210, 1390
col_w = 800
col_top = 1010
box(left_x, col_top, col_w, 92, "自动轻度休眠路径", MID, F_BOX_B)
box(right_x, col_top, col_w, 92, "强制深度休眠路径", MID, F_BOX_B)

elbow([(cx - 120, 790), (left_x + col_w / 2, 930), (left_x + col_w / 2, col_top)], "约2分钟无操作", (left_x + col_w / 2, 905))
elbow([(cx + 120, 790), (right_x + col_w / 2, 930), (right_x + col_w / 2, col_top)], "确认键长按约3秒", (right_x + col_w / 2, 905))

# Light sleep path
y = col_top + 150
last = None
light_steps = [
    ("box", "通知显示任务暂停刷新\n等待界面进入可休眠状态"),
    ("box", "若视频正在播放\n先停止播放并记录返回位置"),
    ("box", "关闭屏幕背光和LCD显示"),
    ("box", "记录WiFi状态并临时关闭无线连接"),
    ("box", "准备RTC等外设进入低功耗状态"),
    ("box", "配置按键和触摸作为唤醒源\n进入LightSleep"),
    ("decision", "检测到唤醒事件？"),
    ("box", "重新初始化触摸、LCD和背光"),
    ("box", "按需恢复WiFi连接\n恢复LVGL界面运行"),
    ("end", "快速恢复到交互状态"),
]
for kind, text in light_steps:
    if kind == "decision":
        cy = y + 74
        decision(left_x + col_w / 2, cy, 470, 142, text)
        if last is not None:
            arrow(left_x + col_w / 2, last, left_x + col_w / 2, cy - 71)
        last = cy + 71
        y = cy + 135
    else:
        h = 112 if kind != "end" else 118
        if kind == "end":
            terminator(left_x, y, col_w, h, text)
        else:
            box(left_x, y, col_w, h, text, WHITE, F_SMALL)
        if last is not None:
            arrow(left_x + col_w / 2, last, left_x + col_w / 2, y)
        last = y + h
        y += h + 54

# Deep sleep path
y = col_top + 150
last = None
deep_steps = [
    ("box", "确认键长按后\n进入深度休眠准备流程"),
    ("box", "通知显示任务暂停\n停止视频等高功耗业务"),
    ("box", "保存文件阅读进度\n关闭SD卡相关操作"),
    ("box", "关闭屏幕、WiFi、I2C和相关外设"),
    ("box", "等待确认键释放\n避免立即重复唤醒"),
    ("box", "配置GPIO7为RTC唤醒引脚\n保持必要IO电平"),
    ("box", "进入DeepSleep深度休眠"),
    ("decision", "再次按下确认键？"),
    ("box", "硬件唤醒并重新启动系统"),
    ("end", "从启动流程重新运行"),
]
for kind, text in deep_steps:
    if kind == "decision":
        cy = y + 74
        decision(right_x + col_w / 2, cy, 470, 142, text)
        if last is not None:
            arrow(right_x + col_w / 2, last, right_x + col_w / 2, cy - 71)
        last = cy + 71
        y = cy + 135
    else:
        h = 112 if kind != "end" else 118
        if kind == "end":
            terminator(right_x, y, col_w, h, text)
        else:
            box(right_x, y, col_w, h, text, WHITE, F_SMALL)
        if last is not None:
            arrow(right_x + col_w / 2, last, right_x + col_w / 2, y)
        last = y + h
        y += h + 54

os.makedirs(OUT_DIR, exist_ok=True)
img.save(OUT_PATH, quality=95)
print(OUT_PATH)
