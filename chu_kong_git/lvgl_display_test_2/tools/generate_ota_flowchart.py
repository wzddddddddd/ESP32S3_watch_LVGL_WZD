# -*- coding: utf-8 -*-
from PIL import Image, ImageDraw, ImageFont
import os
import math


ROOT = r"C:\Users\86177\Desktop\ESP32_chukong\chu_kong_git\lvgl_display_test_2"
OUT_DIR = os.path.join(ROOT, "docs")
OUT_PATH = os.path.join(OUT_DIR, "section_4_3_3_ota_flowchart_bw.png")


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


W, H = 2500, 3000
img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)

F_TITLE = font(70, True)
F_SUB = font(34)
F_BOX = font(38)
F_BOX_B = font(40, True)
F_SMALL = font(34)
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
    skew = 42
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
d.text((W / 2, 85), "4.3.3 OTA远程升级与固件维护模块设计流程图", fill=BLACK, font=F_TITLE, anchor="mm")
d.text((W / 2, 150), "A/B分区升级、云端OTA、本地固件升级、启动确认与回滚保护", fill=GRAY, font=F_SUB, anchor="mm")
d.line((140, 195, W - 140, 195), fill=BLACK, width=3)

# 公共入口
cx = W / 2
terminator(930, 245, 640, 92, "进入OTA升级模块")
box(810, 390, 880, 116, "读取当前运行分区与备用升级分区", MID, F_BOX)
box(810, 550, 880, 116, "用户在界面中选择升级方式", WHITE, F_BOX)
decision(cx, 800, 560, 150, "升级方式？")

arrow(cx, 337, cx, 390)
arrow(cx, 506, cx, 550)
arrow(cx, 666, cx, 725)

# 三列标题
left_x, mid_x, right_x = 115, 910, 1705
col_w = 680
col_top = 1010
box(left_x, col_top, col_w, 92, "云端OTA升级路径", MID, F_BOX_B)
box(mid_x, col_top, col_w, 92, "本地SD卡升级路径", MID, F_BOX_B)
box(right_x, col_top, col_w, 92, "启动确认与回滚保护", MID, F_BOX_B)

elbow([(cx - 160, 800), (left_x + col_w / 2, 940), (left_x + col_w / 2, col_top)], "云端", (left_x + col_w / 2, 920))
arrow(cx, 875, mid_x + col_w / 2, col_top, "本地", label_dx=35, label_dy=-18)
elbow([(cx + 160, 800), (right_x + col_w / 2, 940), (right_x + col_w / 2, col_top)], "重启后", (right_x + col_w / 2, 920))

# 云端路径
y = col_top + 145
last = None
cloud_steps = [
    ("box", "连接云平台并接收升级任务"),
    ("box", "上报当前固件版本\n检查是否存在新版本"),
    ("decision", "是否存在新固件？"),
    ("box", "建立固件下载通道\n下载镜像到非活动分区"),
    ("decision", "镜像校验是否通过？"),
    ("data", "向界面提示下载结果\n等待用户确认跳转"),
    ("box", "设置备用分区为下次启动分区\n清理外设并重启"),
]
for kind, text in cloud_steps:
    if kind == "decision":
        cy = y + 74
        decision(left_x + col_w / 2, cy, 430, 142, text)
        if last is not None:
            arrow(left_x + col_w / 2, last, left_x + col_w / 2, cy - 71)
        last = cy + 71
        y = cy + 135
    else:
        h = 112 if kind != "data" else 126
        if kind == "data":
            data_box(left_x, y, col_w, h, text)
        else:
            box(left_x, y, col_w, h, text, WHITE, F_SMALL)
        if last is not None:
            arrow(left_x + col_w / 2, last, left_x + col_w / 2, y)
        last = y + h
        y += h + 58

# 本地路径
y = col_top + 145
last = None
local_steps = [
    ("box", "扫描SD卡中的本地固件文件"),
    ("data", "用户选择.bin升级包"),
    ("box", "打开固件文件并计算文件大小"),
    ("box", "擦除非活动升级分区\n准备写入新镜像"),
    ("box", "按数据块读取固件\n顺序写入备用分区"),
    ("box", "持续上报升级进度到界面"),
    ("decision", "写入与校验是否通过？"),
    ("box", "提示升级成功\n等待用户确认跳转"),
    ("box", "切换启动分区并重启"),
]
for kind, text in local_steps:
    if kind == "decision":
        cy = y + 74
        decision(mid_x + col_w / 2, cy, 430, 142, text)
        if last is not None:
            arrow(mid_x + col_w / 2, last, mid_x + col_w / 2, cy - 71)
        last = cy + 71
        y = cy + 135
    else:
        h = 112 if kind != "data" else 120
        if kind == "data":
            data_box(mid_x, y, col_w, h, text)
        else:
            box(mid_x, y, col_w, h, text, WHITE, F_SMALL)
        if last is not None:
            arrow(mid_x + col_w / 2, last, mid_x + col_w / 2, y)
        last = y + h
        y += h + 55

# 启动确认/回滚路径
y = col_top + 145
last = None
boot_steps = [
    ("box", "设备重启进入新固件分区"),
    ("decision", "新固件是否正常运行？"),
    ("box", "正常：确认当前固件有效\n异常：回滚到上一可用分区"),
    ("box", "系统继续从有效分区启动\n降低升级失败风险"),
    ("end", "启动保护流程结束"),
]
for idx, (kind, text) in enumerate(boot_steps):
    if kind == "decision":
        cy = y + 74
        decision(right_x + col_w / 2, cy, 450, 142, text)
        if last is not None:
            arrow(right_x + col_w / 2, last, right_x + col_w / 2, cy - 71)
        last = cy + 71
        y = cy + 135
    else:
        h = 118
        if kind == "end":
            terminator(right_x, y, col_w, h, text)
        else:
            box(right_x, y, col_w, h, text, WHITE, F_SMALL)
        if last is not None:
            arrow(right_x + col_w / 2, last, right_x + col_w / 2, y)
        last = y + h
        y += h + 72

# 左、中路径结束框
terminator(left_x, 2620, col_w, 110, "云端升级路径结束")
arrow(left_x + col_w / 2, 2535, left_x + col_w / 2, 2620)

terminator(mid_x, 2770, col_w, 110, "本地升级路径结束")
arrow(mid_x + col_w / 2, 2700, mid_x + col_w / 2, 2770)

os.makedirs(OUT_DIR, exist_ok=True)
img.save(OUT_PATH, quality=95)
print(OUT_PATH)
