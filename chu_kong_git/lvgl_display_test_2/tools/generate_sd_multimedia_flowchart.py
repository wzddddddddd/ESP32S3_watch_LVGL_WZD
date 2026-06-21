# -*- coding: utf-8 -*-
from PIL import Image, ImageDraw, ImageFont
import os
import math


ROOT = r"C:\Users\86177\Desktop\ESP32_chukong\chu_kong_git\lvgl_display_test_2"
OUT_DIR = os.path.join(ROOT, "docs")
OUT_PATH = os.path.join(OUT_DIR, "section_4_3_1_sd_multimedia_flowchart_bw_fixed.png")


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


W, H = 2600, 3900
img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)

F_TITLE = font(70, True)
F_SUB = font(34)
F_BOX = font(38)
F_BOX_B = font(40, True)
F_SMALL = font(34)
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


def fit_wrapped_text(text, max_width, max_height, bold=False, max_size=52, min_size=24):
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
        wrapped, use_font, spacing = fit_wrapped_text(text, w - 40, h - 24, True, 54, 26)
    elif fnt is F_TINY:
        wrapped, use_font, spacing = fit_wrapped_text(text, w - 34, h - 20, False, 42, 22)
    elif fnt is F_SMALL:
        wrapped, use_font, spacing = fit_wrapped_text(text, w - 38, h - 22, False, 48, 24)
    else:
        wrapped, use_font, spacing = fit_wrapped_text(text, w - 40, h - 24, False, 50, 24)
    center_text(x + w / 2, y + h / 2, wrapped, use_font, spacing=spacing)


def terminator(x, y, w, h, text):
    box(x, y, w, h, text, WHITE, F_BOX_B, 4, True)


def decision(cx, cy, w, h, text):
    pts = [(cx, cy - h / 2), (cx + w / 2, cy), (cx, cy + h / 2), (cx - w / 2, cy)]
    d.polygon(pts, fill=WHITE, outline=BLACK)
    d.line(pts + [pts[0]], fill=BLACK, width=4)
    wrapped, use_font, spacing = fit_wrapped_text(text, int(w * 0.64), int(h * 0.56), False, 42, 22)
    center_text(cx, cy, wrapped, use_font, spacing=spacing)


def data_box(x, y, w, h, text):
    skew = 44
    pts = [(x + skew, y), (x + w, y), (x + w - skew, y + h), (x, y + h)]
    d.polygon(pts, fill=WHITE, outline=BLACK)
    d.line(pts + [pts[0]], fill=BLACK, width=4)
    wrapped, use_font, spacing = fit_wrapped_text(text, w - 88, h - 22, False, 48, 24)
    center_text(x + w / 2, y + h / 2, wrapped, use_font, spacing=spacing)


def storage_box(x, y, w, h, text):
    box(x, y, w, h, text, WHITE, F_SMALL)


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
d.text((W / 2, 85), "4.3.1 SD卡综合应用与多媒体模块设计流程图", fill=BLACK, font=F_TITLE, anchor="mm")
d.text((W / 2, 150), "SD/TF卡挂载、图片流式显示、MJPEG视频播放、TXT分页阅读与NVS断点记忆", fill=GRAY, font=F_SUB, anchor="mm")
d.line((150, 195, W - 150, 195), fill=BLACK, width=3)

# 顶部公共流程
terminator(960, 245, 680, 92, "开始")
box(860, 385, 880, 118, "系统启动后初始化 SD 卡接口与 FatFS 文件系统", MID, F_BOX)
decision(W / 2, 620, 560, 150, "SD卡挂载是否成功？")
box(330, 540, 430, 155, "挂载失败\n提示异常并退出模块", WHITE, F_SMALL)
box(860, 760, 880, 116, "扫描 /sdcard 目录资源\n建立小说、图片、视频、固件等文件列表缓存", MID, F_BOX)
box(860, 1015, 880, 116, "用户在 LVGL 页面中选择资源类型或具体文件", WHITE, F_BOX)
box(1020, 1195, 560, 95, "根据文件类型进入对应业务流程", MID, F_SMALL)

arrow(1300, 337, 1300, 385)
arrow(1300, 503, 1300, 545)
arrow(1020, 620, 760, 620, "否", label_dy=-22)
arrow(1300, 695, 1300, 760, "是", label_dx=28)
arrow(1300, 876, 1300, 1015)
arrow(1300, 1131, 1300, 1195)

# 三列分支
col_w = 680
left_x, mid_x, right_x = 120, 960, 1800
col_top = 1410

box(left_x, col_top, col_w, 92, "静态图像解析与壁纸映射", MID, F_BOX_B)
box(mid_x, col_top, col_w, 92, "连续流媒体视频异步解码", MID, F_BOX_B)
box(right_x, col_top, col_w, 92, "TXT长文本分页与断点记忆", MID, F_BOX_B)

elbow([(1300, 1290), (1300, 1340), (left_x + col_w / 2, 1340), (left_x + col_w / 2, col_top)], "图片文件", (left_x + col_w / 2, 1316))
elbow([(1300, 1290), (1300, col_top)], "视频文件", (1370, 1350))
elbow([(1300, 1290), (1300, 1340), (right_x + col_w / 2, 1340), (right_x + col_w / 2, col_top)], "TXT文件", (right_x + col_w / 2, 1316))

# 图片流程
image_steps = [
    ("data", "读取用户选择的 PNG/JPEG 图片路径"),
    ("box", "通过 LVGL 文件系统接口打开 SD 卡图片文件"),
    ("box", "按固定字节块流式读取图像数据"),
    ("box", "PNG/JPEG 解码为屏幕可显示像素块"),
    ("box", "将像素块映射到 LVGL 图片控件或表盘背景"),
    ("store", "显示缓存/图片缓存\n减少重复解码开销"),
    ("end", "完成静态图像显示或壁纸更新"),
]
y = col_top + 150
last = None
for kind, text in image_steps:
    h = 108 if kind != "store" else 128
    if kind == "data":
        data_box(left_x, y, col_w, h, text)
    elif kind == "store":
        storage_box(left_x + 75, y, col_w - 150, h, text)
    elif kind == "end":
        terminator(left_x, y, col_w, h, text)
    else:
        box(left_x, y, col_w, h, text, WHITE, F_SMALL)
    if last is not None:
        arrow(left_x + col_w / 2, last, left_x + col_w / 2, y)
    last = y + h
    y += h + 70

# 视频流程
y = col_top + 150
last = None
for text in [
    "读取用户选择的视频文件名",
    "解析视频完整路径并识别文件格式",
    "切换到视频播放页面\n启动播放流程",
    "创建播放任务并分配多帧缓冲区",
]:
    if last is None:
        data_box(mid_x, y, col_w, 108, text)
    else:
        box(mid_x, y, col_w, 108, text, WHITE, F_SMALL)
        arrow(mid_x + col_w / 2, last, mid_x + col_w / 2, y)
    last = y + 108
    y += 170

cy = y + 70
decision(mid_x + col_w / 2, cy, 420, 142, "视频格式？")
arrow(mid_x + col_w / 2, last, mid_x + col_w / 2, cy - 71)
b1 = (mid_x + 20, cy + 125, 305, 128)
b2 = (mid_x + 355, cy + 125, 305, 128)
box(*b1, "MJPEG：读取压缩帧\n转换为屏幕像素数据", WHITE, F_TINY)
box(*b2, "RGB565：直接读取\n一帧原始像素", WHITE, F_TINY)
arrow(mid_x + col_w / 2 - 70, cy + 71, b1[0] + b1[2] / 2, b1[1], "MJPEG", label_dy=-12)
arrow(mid_x + col_w / 2 + 70, cy + 71, b2[0] + b2[2] / 2, b2[1], "RGB565", label_dy=-12)
join_y = b1[1] + b1[3] + 70
d.line((b1[0] + b1[2] / 2, b1[1] + b1[3], b1[0] + b1[2] / 2, join_y), fill=BLACK, width=4)
d.line((b2[0] + b2[2] / 2, b2[1] + b2[3], b2[0] + b2[2] / 2, join_y), fill=BLACK, width=4)
d.line((b1[0] + b1[2] / 2, join_y, b2[0] + b2[2] / 2, join_y), fill=BLACK, width=4)
arrow(mid_x + col_w / 2, join_y, mid_x + col_w / 2, join_y + 55)
y = join_y + 55
for text, is_end in [
    ("通知显示任务接收新画面", False),
    ("刷新视频画面\n并记录当前帧状态", False),
    ("停止播放或返回视频列表", True),
]:
    if is_end:
        terminator(mid_x, y, col_w, 112, text)
    else:
        box(mid_x, y, col_w, 112, text, WHITE, F_SMALL)
    if y != join_y + 55:
        arrow(mid_x + col_w / 2, y - 70, mid_x + col_w / 2, y)
    y += 182

# TXT流程
txt_steps = [
    ("data", "读取用户选择的 TXT 小说文件名"),
    ("box", "根据文件缓存解析完整文件路径"),
    ("store", "从非易失性存储中\n读取历史阅读位置"),
    ("box", "打开文本文件\n跳转到历史阅读位置"),
    ("box", "按屏幕容量切片读取当前页文本"),
    ("box", "通知显示任务\n更新阅读界面文本"),
]
y = col_top + 150
last = None
for kind, text in txt_steps:
    h = 108 if kind != "store" else 128
    if kind == "data":
        data_box(right_x, y, col_w, h, text)
    elif kind == "store":
        storage_box(right_x + 70, y, col_w - 140, h, text)
    else:
        box(right_x, y, col_w, h, text, WHITE, F_SMALL)
    if last is not None:
        arrow(right_x + col_w / 2, last, right_x + col_w / 2, y)
    last = y + h
    y += h + 62

dec_y = y + 72
decision(right_x + col_w / 2, dec_y, 480, 150, "用户翻页、退出\n或系统休眠？")
arrow(right_x + col_w / 2, last, right_x + col_w / 2, dec_y - 75)
flip_y = dec_y + 135
save_y = dec_y + 330
box(right_x, flip_y, col_w, 108, "翻页：更新文件偏移\n读取上一页或下一页", WHITE, F_SMALL)
storage_box(right_x + 70, save_y, col_w - 140, 128, "退出/休眠：保存文件路径\n与当前阅读位置")
terminator(right_x, save_y + 205, col_w, 108, "下次打开时从断点继续显示")
arrow(right_x + col_w / 2, dec_y + 75, right_x + col_w / 2, flip_y, "翻页")
elbow(
    [
        (right_x + col_w / 2 + 150, dec_y),
        (right_x + col_w - 30, dec_y),
        (right_x + col_w - 30, save_y + 64),
        (right_x + col_w - 70, save_y + 64),
    ],
    "退出/休眠",
    (right_x + col_w - 95, dec_y + 150),
)
arrow(right_x + col_w / 2, save_y + 128, right_x + col_w / 2, save_y + 205)

# 底部说明
note_y = 3430
d.line((150, note_y - 45, W - 150, note_y - 45), fill=BLACK, width=3)
box(
    180,
    note_y,
    2240,
    96,
    "说明：SD卡资源管理、图片显示、视频播放和小说阅读统一采用文件扫描、路径解析、数据读取和界面刷新的处理方式，减少业务模块之间的直接耦合。",
    WHITE,
    F_TINY,
    width=2,
)

os.makedirs(OUT_DIR, exist_ok=True)
img.save(OUT_PATH, quality=95)
print(OUT_PATH)
