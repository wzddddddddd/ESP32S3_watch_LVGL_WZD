# -*- coding: utf-8 -*-
from PIL import Image, ImageDraw, ImageFont
import os
import math


ROOT = r"C:\Users\86177\Desktop\ESP32_chukong\chu_kong_git\lvgl_display_test_2"
OUT_DIR = os.path.join(ROOT, "docs")
OUT_PATH = os.path.join(OUT_DIR, "section_4_6_1_interaction_quick_panel_flowchart_bw.png")


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


W, H = 2600, 2380
img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)

BLACK = "#111111"
GRAY = "#444444"
MID = "#EFEFEF"
WHITE = "#FFFFFF"

F_TITLE = font(60, True)
F_SUB = font(29)
F_BOX = font(30)
F_BOX_B = font(32, True)
F_SMALL = font(25)
F_TINY = font(22)


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


def center_text(x, y, text, fnt, fill=BLACK, spacing=7):
    d.multiline_text((x, y), text, font=fnt, fill=fill, anchor="mm", align="center", spacing=spacing)


def box(x, y, w, h, text, fill=WHITE, fnt=F_SMALL, width=4, rounded=False):
    if rounded:
        d.rounded_rectangle((x, y, x + w, y + h), radius=h // 2, fill=fill, outline=BLACK, width=width)
    else:
        d.rectangle((x, y, x + w, y + h), fill=fill, outline=BLACK, width=width)
    center_text(x + w / 2, y + h / 2, wrap_by_width(text, fnt, w - 48), fnt)


def terminator(x, y, w, h, text):
    box(x, y, w, h, text, WHITE, F_BOX_B, 4, True)


def decision(cx, cy, w, h, text):
    pts = [(cx, cy - h / 2), (cx + w / 2, cy), (cx, cy + h / 2), (cx - w / 2, cy)]
    d.polygon(pts, fill=WHITE, outline=BLACK)
    d.line(pts + [pts[0]], fill=BLACK, width=4)
    center_text(cx, cy, wrap_by_width(text, F_SMALL, int(w * 0.58)), F_SMALL, spacing=5)


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
        d.rectangle((lx - tw / 2 - 10, ly - 18, lx + tw / 2 + 10, ly + 18), fill=WHITE)
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
        d.rectangle((lx - tw / 2 - 10, ly - 18, lx + tw / 2 + 10, ly + 18), fill=WHITE)
        d.text((lx, ly), label, font=F_TINY, fill=BLACK, anchor="mm")


def vertical_flow(x, y, w, steps):
    last_bottom = None
    for text, h, fill in steps:
        box(x, y, w, h, text, fill=fill, fnt=F_SMALL)
        if last_bottom is not None:
            arrow(x + w / 2, last_bottom, x + w / 2, y)
        last_bottom = y + h
        y += h + 54
    return last_bottom


# Title
d.text((W / 2, 85), "4.6.1 视图路由与全局快捷面板机制流程图", fill=BLACK, font=F_TITLE, anchor="mm")
d.text((W / 2, 150), "表盘入口、菜单切换、快捷控制与状态反馈的交互闭环", fill=GRAY, font=F_SUB, anchor="mm")
d.line((150, 195, W - 150, 195), fill=BLACK, width=3)

cx = W / 2
terminator(940, 245, 720, 90, "进入时钟主界面")
box(790, 385, 1020, 115, "LVGL事件系统接收触摸、滑动和按键输入\n根据当前页面判断交互目标", fill=MID, fnt=F_SMALL)
decision(cx, 650, 610, 170, "用户输入类型")

arrow(cx, 335, cx, 385)
arrow(cx, 500, cx, 565)

col_y = 900
col_w = 700
gap = 110
left_x = 145
mid_x = left_x + col_w + gap
right_x = mid_x + col_w + gap

box(left_x, col_y, col_w, 92, "表盘手势与即时反馈", fill=MID, fnt=F_BOX_B)
box(mid_x, col_y, col_w, 92, "主菜单页面路由", fill=MID, fnt=F_BOX_B)
box(right_x, col_y, col_w, 92, "快捷面板功能联动", fill=MID, fnt=F_BOX_B)

elbow([(cx - 185, 650), (left_x + col_w / 2, 790), (left_x + col_w / 2, col_y)], "表盘操作", (left_x + col_w / 2, 765))
elbow([(cx, 735), (mid_x + col_w / 2, 790), (mid_x + col_w / 2, col_y)], "菜单操作", (mid_x + col_w / 2, 765))
elbow([(cx + 185, 650), (right_x + col_w / 2, 790), (right_x + col_w / 2, col_y)], "快捷控制", (right_x + col_w / 2, 765))

left_steps = [
    ("上滑进入主菜单\n下滑打开快捷控制面板", 120, WHITE),
    ("右滑展开弧形菜单\n左滑收起弧形菜单", 120, WHITE),
    ("点击时间区域\n循环切换预设主题颜色", 120, WHITE),
    ("界面立即刷新\n形成触摸到视觉反馈的闭环", 130, WHITE),
]
mid_steps = [
    ("横向滑动主菜单\n小说、图片、视频、设置、游戏居中排列", 130, WHITE),
    ("滚动过程中中心项放大\n非中心项降低透明度", 120, WHITE),
    ("点击目标图标\n进入对应功能页面", 110, WHITE),
    ("在菜单页下滑\n返回时钟主界面", 110, WHITE),
]
right_steps = [
    ("下滑呼出快捷面板\n集中显示高频控制项", 120, WHITE),
    ("拖动亮度滑块\n调节背光并同步百分比", 120, WHITE),
    ("短按WiFi、校时、天气\n执行开关、同步或刷新请求", 130, WHITE),
    ("长按对应按钮\n跳转WiFi、时间或天气详情页", 120, WHITE),
    ("上滑关闭面板\n返回时钟主界面", 110, WHITE),
]

left_end = vertical_flow(left_x, col_y + 150, col_w, left_steps)
mid_end = vertical_flow(mid_x, col_y + 150, col_w, mid_steps)
right_end = vertical_flow(right_x, col_y + 150, col_w, right_steps)

merge_y = 2110
merge_w = 820
merge_x = (W - merge_w) / 2
terminator(merge_x, merge_y, merge_w, 105, "完成页面切换与状态反馈")

elbow([(left_x + col_w / 2, left_end), (left_x + col_w / 2, merge_y - 90), (merge_x + 110, merge_y)])
elbow([(mid_x + col_w / 2, mid_end), (mid_x + col_w / 2, merge_y)])
elbow([(right_x + col_w / 2, right_end), (right_x + col_w / 2, merge_y - 90), (merge_x + merge_w - 110, merge_y)])

os.makedirs(OUT_DIR, exist_ok=True)
img.save(OUT_PATH, quality=95)
print(OUT_PATH)
