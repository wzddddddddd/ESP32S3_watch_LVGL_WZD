# -*- coding: utf-8 -*-
from PIL import Image, ImageDraw, ImageFont
import os
import math


ROOT = r"C:\Users\86177\Desktop\ESP32_chukong\chu_kong_git\lvgl_display_test_2"
OUT_DIR = os.path.join(ROOT, "docs")
OUT_PATH = os.path.join(OUT_DIR, "section_4_3_2_clock_time_flowchart_bw.png")


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


W, H = 2400, 3350
img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)

F_TITLE = font(68, True)
F_SUB = font(34)
F_BOX = font(36)
F_BOX_B = font(38, True)
F_SMALL = font(31)
F_TINY = font(32)

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


def fit_wrapped_text(text, max_width, max_height, bold=False, max_size=50, min_size=22):
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
        wrapped, use_font, spacing = fit_wrapped_text(text, w - 38, h - 22, True, 52, 24)
    elif fnt is F_SMALL:
        wrapped, use_font, spacing = fit_wrapped_text(text, w - 38, h - 22, False, 46, 22)
    else:
        wrapped, use_font, spacing = fit_wrapped_text(text, w - 40, h - 22, False, 48, 22)
    center_text(x + w / 2, y + h / 2, wrapped, use_font, spacing=spacing)


def terminator(x, y, w, h, text):
    box(x, y, w, h, text, WHITE, F_BOX_B, 4, True)


def decision(cx, cy, w, h, text):
    pts = [(cx, cy - h / 2), (cx + w / 2, cy), (cx, cy + h / 2), (cx - w / 2, cy)]
    d.polygon(pts, fill=WHITE, outline=BLACK)
    d.line(pts + [pts[0]], fill=BLACK, width=4)
    wrapped, use_font, spacing = fit_wrapped_text(text, int(w * 0.64), int(h * 0.56), False, 40, 20)
    center_text(cx, cy, wrapped, use_font, spacing=spacing)


def data_box(x, y, w, h, text):
    skew = 44
    pts = [(x + skew, y), (x + w, y), (x + w - skew, y + h), (x, y + h)]
    d.polygon(pts, fill=WHITE, outline=BLACK)
    d.line(pts + [pts[0]], fill=BLACK, width=4)
    wrapped, use_font, spacing = fit_wrapped_text(text, w - 82, h - 22, False, 44, 22)
    center_text(x + w / 2, y + h / 2, wrapped, use_font, spacing=spacing)


def arrow(x1, y1, x2, y2, label=None, label_dx=0, label_dy=0):
    d.line((x1, y1, x2, y2), fill=BLACK, width=4)
    ang = math.atan2(y2 - y1, x2 - x1)
    ah, aw = 20, 11
    p1 = (x2 - ah * math.cos(ang) + aw * math.sin(ang), y2 - ah * math.sin(ang) - aw * math.cos(ang))
    p2 = (x2 - ah * math.cos(ang) - aw * math.sin(ang), y2 - ah * math.sin(ang) + aw * math.cos(ang))
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
d.text((W / 2, 85), "4.3.2 时钟管理与时间协同模块设计流程图", fill=BLACK, font=F_TITLE, anchor="mm")
d.text((W / 2, 150), "系统时间缓存、网络校时、硬件RTC守时与界面显示刷新协同", fill=GRAY, font=F_SUB, anchor="mm")
d.line((140, 195, W - 140, 195), fill=BLACK, width=3)

# 主流程
cx = W / 2
terminator(880, 250, 640, 92, "系统启动")
box(760, 390, 880, 116, "显示任务启动后进入时间初始化流程", MID, F_BOX)
box(760, 550, 880, 122, "初始化外置SD3078硬件RTC\n并建立I2C通信通道", WHITE, F_SMALL)
decision(cx, 800, 520, 150, "RTC时间是否有效？")
box(300, 742, 360, 116, "无效：写入默认时间\n2026-01-01", WHITE, F_SMALL)
box(760, 930, 880, 122, "从RTC读取当前时间\n并同步到ESP32系统时钟", WHITE, F_SMALL)
box(760, 1095, 880, 116, "周期性刷新系统时间缓存\n供界面显示使用", MID, F_SMALL)
box(760, 1255, 880, 116, "等待WiFi连接或用户手动校时触发", MID, F_SMALL)

arrow(cx, 342, cx, 390)
arrow(cx, 506, cx, 550)
arrow(cx, 672, cx, 725)
arrow(cx - 260, 800, 660, 800, "否", label_dy=-22)
arrow(480, 858, 760, 985)
arrow(cx, 875, cx, 930, "是", label_dx=28)
arrow(cx, 1052, cx, 1095)
arrow(cx, 1211, cx, 1255)

# 分支标题
branch_y = 1450
box(760, branch_y, 880, 100, "时间协同进入三条运行路径", MID, F_BOX_B)
arrow(cx, 1371, cx, branch_y)

left_x, mid_x, right_x = 115, 860, 1605
col_w = 680
col_top = 1690
box(left_x, col_top, col_w, 92, "网络自动校时路径", MID, F_BOX_B)
box(mid_x, col_top, col_w, 92, "本地RTC守时路径", MID, F_BOX_B)
box(right_x, col_top, col_w, 92, "界面显示刷新路径", MID, F_BOX_B)

elbow([(cx, branch_y + 100), (cx, 1605), (left_x + col_w / 2, 1605), (left_x + col_w / 2, col_top)], "WiFi/NTP", (left_x + col_w / 2, 1580))
elbow([(cx, branch_y + 100), (cx, col_top)], "RTC", (cx + 56, 1605))
elbow([(cx, branch_y + 100), (cx, 1605), (right_x + col_w / 2, 1605), (right_x + col_w / 2, col_top)], "LVGL", (right_x + col_w / 2, 1580))

# NTP路径
y = col_top + 145
last = None
for kind, text in [
    ("box", "WiFi连接成功或用户点击网络校时"),
    ("box", "系统发出网络校时请求"),
    ("box", "WiFi可用时通过SNTP协议\n向授时服务器请求标准时间"),
    ("box", "校时成功后刷新系统时间缓存\n失败则保留RTC本地时间"),
    ("box", "将校准后的系统时间\n回写到SD3078硬件RTC"),
    ("data", "向界面发送同步结果\n更新状态提示"),
]:
    if kind == "decision":
        cy = y + 76
        decision(left_x + col_w / 2, cy, 430, 142, text)
        if last is not None:
            arrow(left_x + col_w / 2, last, left_x + col_w / 2, cy - 71)
        last = cy + 71
        y = cy + 135
    else:
        h = 116 if kind != "data" else 126
        if kind == "data":
            data_box(left_x, y, col_w, h, text)
        else:
            box(left_x, y, col_w, h, text, WHITE, F_SMALL)
        if last is not None:
            arrow(left_x + col_w / 2, last, left_x + col_w / 2, y)
        last = y + h
        y += h + 62

# local RTC路径
y = col_top + 145
last = None
for text in [
    "SD3078由独立电源保持走时",
    "系统定时读取RTC时间\n修正本地系统时钟",
    "通过I2C读取RTC寄存器\n转换为系统可识别的时间",
    "将RTC时间写入系统时钟",
    "刷新全局时间缓存",
    "休眠前让RTC进入低功耗保持状态",
]:
    h = 116
    box(mid_x, y, col_w, h, text, WHITE, F_SMALL)
    if last is not None:
        arrow(mid_x + col_w / 2, last, mid_x + col_w / 2, y)
    last = y + h
    y += h + 72

# UI显示路径
y = col_top + 145
last = None
for kind, text in [
    ("box", "显示层创建500ms时间刷新定时器"),
    ("box", "刷新前检查当前是否为时钟页面"),
    ("decision", "时钟页面是否有效且正在显示？"),
    ("box", "读取缓存中的时、分、年、月、日"),
    ("box", "更新时间与日期显示标签"),
    ("box", "用户手动设置时间时\n同步系统时钟与硬件RTC"),
]:
    if kind == "decision":
        cy = y + 76
        decision(right_x + col_w / 2, cy, 470, 142, text)
        if last is not None:
            arrow(right_x + col_w / 2, last, right_x + col_w / 2, cy - 71)
        last = cy + 71
        y = cy + 135
    else:
        h = 116
        box(right_x, y, col_w, h, text, WHITE, F_SMALL)
        if last is not None:
            arrow(right_x + col_w / 2, last, right_x + col_w / 2, y)
        last = y + h
        y += h + 72

os.makedirs(OUT_DIR, exist_ok=True)
img.save(OUT_PATH, quality=95)
print(OUT_PATH)
