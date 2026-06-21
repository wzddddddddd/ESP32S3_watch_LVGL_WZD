# -*- coding: utf-8 -*-
from PIL import Image, ImageDraw, ImageFont
import math
import os


ROOT = r"C:\Users\86177\Desktop\ESP32_chukong\chu_kong_git\lvgl_display_test_2"
OUT_DIR = os.path.join(ROOT, "docs")
OUT_PATH = os.path.join(OUT_DIR, "software_architecture_layers_largefont.png")


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


W, H = 2100, 970
img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)

BLACK = "#0F1720"
LINE = "#5A6B7F"
LAYER = "#F2F6FA"
BOX = "#CAD7E6"
QUEUE = "#E7EEF6"

LAYER_TITLE_SIZE = 36
LAYER_DESC_SIZE = 25
CARD_TITLE_SIZE = 31
CARD_BODY_SIZE = 26
QUEUE_TITLE_SIZE = 32
QUEUE_BODY_SIZE = 26


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


def fit(text, max_w, max_h, bold=False, max_size=34, min_size=13):
    for size in range(max_size, min_size - 1, -1):
        fnt = font(size, bold)
        spacing = max(4, size // 5)
        wrapped = wrap(text, fnt, max_w)
        tw, th = text_size(wrapped, fnt, spacing)
        if tw <= max_w and th <= max_h:
            return wrapped, fnt, spacing
    fnt = font(min_size, bold)
    return wrap(text, fnt, max_w), fnt, 4


def layer(y, h, title, desc):
    d.rounded_rectangle((14, y, W - 14, y + h), radius=14, fill=LAYER, outline="#B8C4D1", width=3)
    d.text((62, y + 48), title, font=font(LAYER_TITLE_SIZE, True), fill=BLACK, anchor="lm")


def card(x, y, w, h, title, body, title_size=24, body_size=17, fill=BOX):
    d.rounded_rectangle((x, y, x + w, y + h), radius=10, fill=fill, outline=LINE, width=2)
    title_wrapped, title_font, title_spacing = fit(title, w - 16, 64, True, title_size, 18)
    d.multiline_text((x + w / 2, y + 38), title_wrapped, font=title_font, fill=BLACK,
                     anchor="mm", align="center", spacing=title_spacing)
    wrapped, fnt, spacing = fit(body, w - 16, h - 94, False, body_size, 17)
    d.multiline_text((x + w / 2, y + 88), wrapped, font=fnt, fill=BLACK,
                     anchor="ma", align="center", spacing=spacing)


def label_box(x, y, text, max_size=20):
    wrapped, fnt, spacing = fit(text, 420, 34, True, max_size, 13)
    tw, th = text_size(wrapped, fnt, spacing)
    d.rounded_rectangle((x - tw / 2 - 12, y - th / 2 - 6,
                         x + tw / 2 + 12, y + th / 2 + 6),
                        radius=5, fill=LAYER, outline=None)
    d.multiline_text((x, y), wrapped, font=fnt, fill=BLACK, anchor="mm",
                     align="center", spacing=spacing)


def arrow(x1, y1, x2, y2, dashed=False, width=4, bidirectional=False):
    def arrowhead(x, y, ang):
        ah, aw = 22, 11
        p1 = (x - ah * math.cos(ang) + aw * math.sin(ang),
              y - ah * math.sin(ang) - aw * math.cos(ang))
        p2 = (x - ah * math.cos(ang) - aw * math.sin(ang),
              y - ah * math.sin(ang) + aw * math.cos(ang))
        d.polygon([(x, y), p1, p2], fill=LINE)

    dist = math.hypot(x2 - x1, y2 - y1)
    if not dist:
        return

    ux, uy = (x2 - x1) / dist, (y2 - y1) / dist
    start_gap = 24 if bidirectional else 0
    end_gap = 24

    if dashed:
        dash, gap = 12, 9
        p = start_gap
        while p < dist - end_gap:
            sx, sy = x1 + ux * p, y1 + uy * p
            ex = x1 + ux * min(dist - end_gap, p + dash)
            ey = y1 + uy * min(dist - end_gap, p + dash)
            d.line((sx, sy, ex, ey), fill=LINE, width=width)
            p += dash + gap
    else:
        d.line((x1 + ux * start_gap, y1 + uy * start_gap,
                x2 - ux * end_gap, y2 - uy * end_gap), fill=LINE, width=width)

    ang = math.atan2(y2 - y1, x2 - x1)
    arrowhead(x2, y2, ang)
    if bidirectional:
        arrowhead(x1, y1, ang + math.pi)


# 原图三层结构与逻辑保持不变，仅替换视觉风格并去掉图标。
layer(15, 205, "显示层",
      "系统前端视觉与交互入口，基于LVGL在独立显示线程运行。")
layer(380, 300, "应用层",
      "系统后端，负责业务逻辑、多媒体处理、系统服务和任务协作。")
layer(720, 230, "硬件层",
      "系统物理基础，控制底层外设，向上提供解耦、标准的驱动接口。")

# 显示层只负责接收应用层状态并统一刷新界面。
display_y = 42
display_x, display_w, display_h = 450, 1200, 130
card(display_x, display_y, display_w, display_h,
     "LVGL统一显示刷新线程",
     "读取异步消息队列中的业务状态，统一完成页面刷新、控件重绘与屏幕输出",
     CARD_TITLE_SIZE, CARD_BODY_SIZE)

# 应用层与显示层之间的消息队列枢纽。
queue_x, queue_y, queue_w, queue_h = 610, 250, 880, 105
d.rounded_rectangle((queue_x, queue_y, queue_x + queue_w, queue_y + queue_h),
                    radius=10, fill=QUEUE, outline=LINE, width=2)
d.text((queue_x + queue_w / 2, queue_y + 36), "异步消息队列枢纽",
       font=font(QUEUE_TITLE_SIZE, True), fill=BLACK, anchor="mm")
wrapped, fnt, spacing = fit("应用任务间通信、业务状态同步、UI刷新请求",
                            queue_w - 42, 38, False, QUEUE_BODY_SIZE, 18)
d.multiline_text((queue_x + queue_w / 2, queue_y + 76), wrapped,
                 font=fnt, fill=BLACK, anchor="mm", align="center", spacing=spacing)

# 应用层模块：数量与原图保持一致。
app_y = 485
app_w, app_h = 248, 190
app_xs = [310, 562, 814, 1066, 1318, 1570, 1822]
app_cards = [
    ("系统服务", "时间同步、天气获取\nWiFi状态管理"),
    ("OTA升级与功耗控制", "本地/云端OTA\n亮灭屏、休眠唤醒"),
    ("多媒体与娱乐", "小说显示、图片解码\nMJPEG视频解码\n游戏逻辑"),
    ("存储工作线程", "SD卡文件扫描\n电子书读取\n视频帧读取"),
    ("网络线程", "后台管理WiFi\n连接与通信状态"),
    ("解码/业务任务", "音视频解码\n状态处理\n业务协作"),
    ("FreeRTOS同步机制", "队列、信号量\n任务间安全通信"),
]
for x, (title, body) in zip(app_xs, app_cards):
    card(x, app_y, app_w, app_h, title, body, CARD_TITLE_SIZE, CARD_BODY_SIZE)

# 硬件层模块
hw_y = 785
hw_w, hw_h = 285, 160
hw_xs = [305, 602, 899, 1196, 1493, 1790]
hw_cards = [
    ("触摸屏幕相关硬件", "LCD显示芯片\n触控芯片、背光"),
    ("存储硬件", "SD卡\n外置Flash芯片"),
    ("总线与接口", "SPI、I2C\nGPIO底层外设"),
    ("时钟与时序硬件", "RTC实时时钟\n系统定时器"),
    ("网络相关硬件", "WiFi无线\n射频链路"),
    ("电源及唤醒硬件", "供电\n休眠唤醒"),
]
for x, (title, body) in zip(hw_xs, hw_cards):
    card(x, hw_y, hw_w, hw_h, title, body, CARD_TITLE_SIZE, CARD_BODY_SIZE)

# 原图的层间主逻辑箭头。
center_x = W // 2
arrow(center_x, 720, center_x, 660)
label_box(center_x, 738, "标准驱动接口与底层外设支撑", 20)
arrow(center_x, queue_y, center_x, display_y + display_h)

# 应用模块到消息队列的原有关系。
for sx, tx in zip([x + app_w / 2 for x in app_xs],
                  [680, 805, 930, 1050, 1170, 1295, 1420]):
    arrow(sx, app_y, tx, queue_y + queue_h, dashed=True, width=3, bidirectional=True)

os.makedirs(OUT_DIR, exist_ok=True)
img.save(OUT_PATH, quality=95)
print(OUT_PATH)
