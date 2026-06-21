# -*- coding: utf-8 -*-
from PIL import Image, ImageDraw, ImageFont
import os


ROOT = r"C:\Users\86177\Desktop\ESP32_chukong\chu_kong_git\lvgl_display_test_2"
OUT_DIR = os.path.join(ROOT, "docs")
OUT_PATH = os.path.join(OUT_DIR, "table_5_1_core_function_test.png")


def find_font():
    for path in [
        r"C:\Windows\Fonts\simsun.ttc",
        r"C:\Windows\Fonts\msyh.ttc",
        r"C:\Windows\Fonts\simhei.ttf",
    ]:
        if os.path.exists(path):
            return path
    return r"C:\Windows\Fonts\arial.ttf"


FONT_PATH = find_font()


def font(size, bold=False):
    if bold:
        for path in [r"C:\Windows\Fonts\simhei.ttf", r"C:\Windows\Fonts\msyhbd.ttc"]:
            if os.path.exists(path):
                return ImageFont.truetype(path, size)
    return ImageFont.truetype(FONT_PATH, size)


W, H = 1800, 1040
img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)

BLACK = "#000000"
GRAY = "#222222"

F_TITLE = font(42)
F_HEAD = font(31, True)
F_BODY = font(29)
F_SMALL = font(25)


def text_width(text, fnt):
    if not text:
        return 0
    return d.textbbox((0, 0), text, font=fnt)[2]


def wrap_text(text, fnt, max_width):
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
    return lines or [""]


def draw_centered_lines(xc, yc, lines, fnt, line_gap=8):
    line_h = fnt.size + line_gap
    total_h = line_h * len(lines) - line_gap
    y = yc - total_h / 2
    for line in lines:
        d.text((xc, y), line, fill=BLACK, font=fnt, anchor="ma")
        y += line_h


def draw_left_lines(x, yc, lines, fnt, line_gap=8):
    line_h = fnt.size + line_gap
    total_h = line_h * len(lines) - line_gap
    y = yc - total_h / 2
    for line in lines:
        d.text((x, y), line, fill=BLACK, font=fnt, anchor="la")
        y += line_h


title = "表5.1 核心功能测试结果"
headers = ["测试功能项", "成功率", "平均响应", "备注说明"]
rows = [
    ["主菜单高频滑动", "30/30\n（100%）", "＜50ms", "交互稳定，高频滑动下偶发轻微画面撕裂"],
    ["SD卡壁纸切换", "25/30\n（83.3%）", "约100ms～200ms", "存在短暂解码显示延迟，少数情况下出现无响应"],
    ["MJPEG视频播放", "28/30\n（93.3%）", "约25fps", "高负载下轻微卡顿，偶有掉帧"],
    ["电子书断点续读", "30/30\n（100%）", "＜150ms", "索引加载稳定，阅读位置定位准确"],
    ["WiFi网络连接", "29/30\n（96.7%）", "约4.5s", "极少数情况下受环境干扰导致连接超时，重新开启后可恢复"],
    ["OTA升级\n（远程/本地）", "10/10\n（100%）", "约70s", "以本地升级包测试为准，断电中断后的回滚保护有效"],
]

left, right = 80, W - 80
top_title_y = 52
top_line_y = 105
header_y = 155
header_line_y = 205
bottom_y = 960

d.text((W / 2, top_title_y), title, fill=BLACK, font=F_TITLE, anchor="mm")
d.line((left, top_line_y, right, top_line_y), fill=BLACK, width=5)
d.line((left, header_line_y, right, header_line_y), fill=GRAY, width=2)
d.line((left, bottom_y, right, bottom_y), fill=BLACK, width=5)

table_w = right - left
col_widths = [0.23, 0.16, 0.19, 0.42]
col_x = [left]
for ratio in col_widths[:-1]:
    col_x.append(col_x[-1] + table_w * ratio)
col_x.append(right)
col_centers = [(col_x[i] + col_x[i + 1]) / 2 for i in range(4)]

for i, h in enumerate(headers):
    d.text((col_centers[i], header_y), h, fill=BLACK, font=F_HEAD, anchor="mm")

row_area_top = header_line_y + 20
row_h = (bottom_y - row_area_top - 18) / len(rows)

for r, row in enumerate(rows):
    cy = row_area_top + row_h * r + row_h / 2
    for c, text in enumerate(row):
        max_w = col_x[c + 1] - col_x[c] - 28
        fnt = F_BODY if c != 3 else F_SMALL
        lines = wrap_text(text, fnt, max_w)
        if c == 3:
            draw_left_lines(col_x[c] + 16, cy, lines, fnt, line_gap=7)
        else:
            draw_centered_lines(col_centers[c], cy, lines, fnt, line_gap=7)

os.makedirs(OUT_DIR, exist_ok=True)
img.save(OUT_PATH, quality=95)
print(OUT_PATH)
