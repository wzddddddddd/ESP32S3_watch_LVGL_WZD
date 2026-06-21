const pptxgen = require("pptxgenjs");
const fs = require("fs");
const path = require("path");

const ROOT = "C:/Users/86177/Desktop/ESP32_chukong/chu_kong_git/lvgl_display_test_2";
const OUT = path.join(ROOT, "docs", "ESP32S3低功耗触控智能手表系统答辩汇报_论文脉络版.pptx");

const pptx = new pptxgen();
pptx.defineLayout({ name: "WIDE", width: 13.333, height: 7.5 });
pptx.layout = "WIDE";
pptx.author = "Codex";
pptx.subject = "基于ESP32-S3R8与LVGL的低功耗触控智能手表系统设计与实现";
pptx.title = "ESP32-S3R8低功耗触控智能手表系统答辩汇报";
pptx.lang = "zh-CN";
pptx.theme = {
  headFontFace: "Microsoft YaHei",
  bodyFontFace: "Microsoft YaHei",
  lang: "zh-CN",
};

const C = {
  navy: "0D2B4C",
  blue: "0B63B6",
  blue2: "1687D9",
  cyan: "22B8D7",
  pale: "F4F8FC",
  grid: "E5EEF6",
  line: "C9D8E6",
  card: "FFFFFF",
  text: "172335",
  muted: "5D6D7E",
  orange: "F28B2E",
  green: "31A36B",
  red: "D65757",
};

function addGrid(slide) {
  slide.background = { color: C.pale };
  slide.addShape(pptx.ShapeType.rect, { x: 0, y: 0, w: 13.333, h: 7.5, fill: { color: C.pale }, line: { color: C.pale } });
  for (let x = 0.2; x < 13.3; x += 0.45) {
    slide.addShape(pptx.ShapeType.line, { x, y: 0, w: 0, h: 7.5, line: { color: C.grid, transparency: 35, width: 0.25 } });
  }
  for (let y = 0.2; y < 7.4; y += 0.45) {
    slide.addShape(pptx.ShapeType.line, { x: 0, y, w: 13.333, h: 0, line: { color: C.grid, transparency: 35, width: 0.25 } });
  }
  slide.addShape(pptx.ShapeType.rect, { x: 0, y: 7.08, w: 13.333, h: 0.42, fill: { color: "DDEDF8", transparency: 10 }, line: { transparency: 100 } });
}

function title(slide, no, zh, en) {
  slide.addText(no, { x: 0.52, y: 0.36, w: 0.36, h: 0.25, fontFace: "Microsoft YaHei", fontSize: 13, bold: true, color: C.navy, margin: 0 });
  slide.addText(zh, { x: 0.92, y: 0.34, w: 8.6, h: 0.36, fontFace: "Microsoft YaHei", fontSize: 18, bold: true, color: C.text, margin: 0 });
  slide.addText(en || "", { x: 0.92, y: 0.68, w: 2.8, h: 0.18, fontFace: "Arial", fontSize: 7.5, bold: true, color: C.blue, margin: 0, charSpace: 1 });
}

function footer(slide, n) {
  slide.addText("基于ESP32-S3R8与LVGL的低功耗触控智能手表系统设计与实现", { x: 0.55, y: 7.18, w: 5.7, h: 0.16, fontSize: 7, color: "7E8FA3", margin: 0 });
  slide.addText(String(n).padStart(2, "0"), { x: 12.42, y: 7.14, w: 0.42, h: 0.18, fontSize: 8, bold: true, color: C.blue, align: "right", margin: 0 });
}

function card(slide, x, y, w, h, head, body, opts = {}) {
  slide.addShape(pptx.ShapeType.roundRect, {
    x, y, w, h, rectRadius: 0.05,
    fill: { color: opts.fill || C.card, transparency: opts.transparency || 0 },
    line: { color: opts.line || "D8E4EE", width: opts.lineWidth || 0.8 },
  });
  if (opts.bar) slide.addShape(pptx.ShapeType.rect, { x, y, w: 0.05, h, fill: { color: opts.bar }, line: { transparency: 100 } });
  slide.addText(head, { x: x + 0.18, y: y + 0.17, w: w - 0.36, h: 0.25, fontSize: opts.hSize || 13.2, bold: true, color: opts.hColor || C.text, margin: 0, fit: "shrink" });
  if (body) slide.addText(body, { x: x + 0.18, y: y + 0.52, w: w - 0.36, h: h - 0.62, fontSize: opts.bSize || 9.5, color: opts.bColor || C.muted, margin: 0.02, fit: "shrink", breakLine: false });
}

function chip(slide, x, y, txt, color = C.blue) {
  slide.addShape(pptx.ShapeType.roundRect, { x, y, w: 1.1, h: 0.34, rectRadius: 0.04, fill: { color }, line: { transparency: 100 } });
  slide.addText(txt, { x, y: y + 0.08, w: 1.1, h: 0.12, fontSize: 8.2, bold: true, color: "FFFFFF", align: "center", margin: 0 });
}

function arrow(slide, x1, y1, x2, y2, color = C.blue) {
  slide.addShape(pptx.ShapeType.line, { x: x1, y: y1, w: x2 - x1, h: y2 - y1, line: { color, width: 1.2, endArrowType: "triangle" } });
}

function flowBox(slide, x, y, w, h, txt, color = C.blue) {
  slide.addShape(pptx.ShapeType.roundRect, { x, y, w, h, rectRadius: 0.04, fill: { color: "FFFFFF" }, line: { color, width: 1.0 } });
  slide.addText(txt, { x: x + 0.08, y: y + 0.15, w: w - 0.16, h: h - 0.22, fontSize: 10.5, bold: true, color: C.text, align: "center", valign: "mid", fit: "shrink", margin: 0 });
}

function metric(slide, x, y, val, label, color) {
  slide.addShape(pptx.ShapeType.ellipse, { x, y, w: 0.86, h: 0.86, fill: { color }, line: { color } });
  slide.addText(val, { x: x + 0.04, y: y + 0.26, w: 0.78, h: 0.18, fontSize: 11, bold: true, color: "FFFFFF", align: "center", margin: 0 });
  slide.addText(label, { x: x - 0.28, y: y + 0.98, w: 1.42, h: 0.25, fontSize: 9.2, bold: true, color: C.text, align: "center", margin: 0, fit: "shrink" });
}

function timeline(slide, x, y, steps) {
  const gap = 1.93;
  steps.forEach((s, i) => {
    const cx = x + i * gap;
    slide.addShape(pptx.ShapeType.rect, { x: cx, y, w: 1.35, h: 0.4, fill: { color: C.blue }, line: { transparency: 100 } });
    slide.addText(String(i + 1).padStart(2, "0"), { x: cx, y: y + 0.11, w: 1.35, h: 0.1, fontSize: 9, bold: true, color: "FFFFFF", align: "center", margin: 0 });
    card(slide, cx - 0.1, y + 0.65, 1.55, 1.05, s[0], s[1], { hSize: 10, bSize: 7.4 });
    if (i < steps.length - 1) arrow(slide, cx + 1.4, y + 0.2, cx + gap - 0.15, y + 0.2, C.blue);
  });
}

function simpleTable(slide, x, y, rows) {
  const col = [2.1, 1.25, 1.45, 2.4];
  const heads = ["测试项", "成功率", "响应/指标", "结论"];
  let cx = x;
  heads.forEach((h, i) => {
    slide.addShape(pptx.ShapeType.rect, { x: cx, y, w: col[i], h: 0.42, fill: { color: C.blue }, line: { color: "FFFFFF", width: 0.3 } });
    slide.addText(h, { x: cx + 0.04, y: y + 0.13, w: col[i] - 0.08, h: 0.12, fontSize: 8.5, bold: true, color: "FFFFFF", align: "center", margin: 0 });
    cx += col[i];
  });
  rows.forEach((r, ri) => {
    cx = x;
    r.forEach((v, i) => {
      slide.addShape(pptx.ShapeType.rect, { x: cx, y: y + 0.42 + ri * 0.46, w: col[i], h: 0.46, fill: { color: ri % 2 ? "F3F7FB" : "FFFFFF" }, line: { color: "DFE8F2", width: 0.35 } });
      slide.addText(v, { x: cx + 0.05, y: y + 0.54 + ri * 0.46, w: col[i] - 0.1, h: 0.12, fontSize: 7.8, color: C.text, align: i === 3 ? "left" : "center", margin: 0, fit: "shrink" });
      cx += col[i];
    });
  });
}

let page = 1;

// 1 cover
{
  const s = pptx.addSlide();
  s.background = { color: C.navy };
  s.addShape(pptx.ShapeType.rect, { x: 0, y: 0, w: 13.333, h: 7.5, fill: { color: C.navy }, line: { color: C.navy } });
  s.addShape(pptx.ShapeType.rect, { x: 0, y: 5.05, w: 13.333, h: 2.45, fill: { color: "126985", transparency: 15 }, line: { transparency: 100 } });
  for (let i = 0; i < 6; i++) {
    s.addShape(pptx.ShapeType.arc, { x: 6.6 + i * 0.18, y: 0.7 + i * 0.08, w: 5.2 - i * 0.35, h: 4.3 - i * 0.25, line: { color: "5DC6D9", transparency: 35 + i * 7, width: 1 }, fill: { transparency: 100 } });
  }
  s.addText("基于ESP32-S3R8与LVGL的", { x: 0.9, y: 1.45, w: 6.2, h: 0.36, fontSize: 25, bold: true, color: "FFFFFF", margin: 0 });
  s.addText("低功耗触控智能手表系统设计与实现", { x: 0.9, y: 1.95, w: 8.3, h: 0.46, fontSize: 30, bold: true, color: "FFFFFF", margin: 0 });
  s.addShape(pptx.ShapeType.line, { x: 0.9, y: 2.7, w: 4.2, h: 0, line: { color: "7DE0F0", width: 1.2 } });
  s.addText("毕业论文答辩汇报", { x: 0.92, y: 3.05, w: 2.0, h: 0.22, fontSize: 12, color: "DCECF7", margin: 0 });
  s.addText("研究内容：硬件系统设计、软件架构设计、关键功能实现、系统调试与测试", { x: 0.92, y: 6.35, w: 7.3, h: 0.2, fontSize: 9.5, color: "D8EEF5", margin: 0 });
  page++;
}

// 2 contents
{
  const s = pptx.addSlide(); addGrid(s);
  s.addText("目录", { x: 5.82, y: 0.38, w: 1.0, h: 0.32, fontSize: 18, bold: true, color: C.text, align: "center", margin: 0 });
  s.addText("CONTENTS", { x: 5.76, y: 0.74, w: 1.2, h: 0.16, fontSize: 6.5, bold: true, color: C.blue, align: "center", margin: 0, charSpace: 1.2 });
  const items = [
    ["01", "绪论：研究背景与意义", "说明课题来源、研究价值与系统目标"],
    ["02", "需求分析与总体架构", "梳理功能需求、性能指标与系统总体方案"],
    ["03", "硬件系统设计", "介绍主控、显示触控、存储、RTC与电源电路"],
    ["04", "软件系统设计", "说明FreeRTOS任务、LVGL刷新和关键业务模块"],
    ["05", "系统调试与测试", "展示功能、功耗、OTA与稳定性测试结果"],
    ["06", "总结与未来展望", "归纳成果、不足与后续优化方向"],
  ];
  items.forEach((it, i) => {
    const x = i % 2 === 0 ? 3.05 : 7.05;
    const y = 1.45 + Math.floor(i / 2) * 1.22;
    card(s, x, y, 3.45, 0.86, it[1], it[2], { hSize: 12.5, bSize: 8.3 });
    s.addText(it[0], { x: x + 0.18, y: y + 0.23, w: 0.55, h: 0.22, fontSize: 15, bold: true, color: C.blue, margin: 0 });
  });
  footer(s, page++);
}

// 3 intro
{
  const s = pptx.addSlide(); addGrid(s); title(s, "01", "绪论：研究背景与意义", "BACKGROUND");
  card(s, 0.7, 1.35, 3.6, 3.8, "研究背景", "可穿戴设备从计时工具逐步发展为信息交互、健康监测和云端协同终端。低成本嵌入式平台若要实现流畅图形界面、多媒体和联网维护，仍面临算力、存储和功耗约束。", { hSize: 15, bSize: 12, bar: C.blue });
  card(s, 4.85, 1.35, 3.6, 3.8, "研究意义", "本课题验证在ESP32-S3R8微控制器上构建低功耗触控智能手表的可行方案，为轻量级物联网穿戴终端提供低成本、高集成度的软硬件实现参考。", { hSize: 15, bSize: 12, bar: C.cyan });
  card(s, 9.0, 1.35, 3.6, 3.8, "研究目标", "完成表盘显示、触摸交互、SD卡多媒体、小说阅读、休闲游戏、网络校时、天气获取、OneNET通信和OTA升级等功能，并验证功耗控制效果。", { hSize: 15, bSize: 12, bar: C.orange });
  footer(s, page++);
}

// 4 requirements
{
  const s = pptx.addSlide(); addGrid(s); title(s, "02", "需求分析：从功能需求落到性能指标", "REQUIREMENTS");
  metric(s, 0.95, 1.45, "25fps", "MJPEG视频", C.orange);
  metric(s, 2.75, 1.45, "<50ms", "触控响应", C.cyan);
  metric(s, 4.55, 1.45, "120s", "轻休眠触发", C.blue);
  metric(s, 6.35, 1.45, "3s", "深休眠触发", C.green);
  metric(s, 8.15, 1.45, "OTA", "云端/本地", C.red);
  const reqs = [
    ["基础服务", "RTC守时、NTP网络校时、天气信息获取"],
    ["图形交互", "LVGL全彩界面、触摸手势、快捷面板、系统设置"],
    ["多媒体", "SD卡图片、MJPEG视频、TXT小说断点续读"],
    ["物联网", "WiFi管理、MQTT/OneNET接入、OTA维护"],
    ["低功耗", "背光调节、WiFi按需启停、Light/Deep Sleep"],
  ];
  reqs.forEach((r, i) => {
    const y = 3.35 + i * 0.54;
    chip(s, 0.95, y, r[0], [C.blue,C.cyan,C.orange,C.green,C.red][i]);
    s.addText(r[1], { x: 2.35, y: y + 0.07, w: 8.5, h: 0.16, fontSize: 11.5, color: C.text, margin: 0 });
  });
  footer(s, page++);
}

// 5 architecture
{
  const s = pptx.addSlide(); addGrid(s); title(s, "02", "总体架构：硬件资源、软件任务与显示刷新解耦", "OVERALL ARCHITECTURE");
  flowBox(s, 5.0, 1.55, 3.1, 0.75, "ESP32-S3R8主控\n双核处理 + 8MB PSRAM", C.blue);
  const around = [
    ["显示触控\nST7789P3 / CST816T", 0.9, 1.25, C.cyan],
    ["存储扩展\nMicro SD / W25Q128", 0.9, 4.35, C.orange],
    ["时间基准\nSD3078 RTC", 9.85, 1.25, C.green],
    ["电源与唤醒\n锂电池 / LDO / GPIO", 9.85, 4.35, C.red],
  ];
  around.forEach(([t, x, y, c]) => flowBox(s, x, y, 2.45, 0.82, t, c));
  arrow(s, 3.35, 1.66, 5.0, 1.88, C.cyan);
  arrow(s, 3.35, 4.75, 5.0, 2.2, C.orange);
  arrow(s, 9.85, 1.66, 8.1, 1.88, C.green);
  arrow(s, 9.85, 4.75, 8.1, 2.2, C.red);
  card(s, 3.95, 3.2, 4.9, 0.98, "系统分层逻辑", "硬件层提供驱动接口；应用层完成业务处理；显示层读取消息队列并统一刷新界面。", { hSize: 14, bSize: 10.2, bar: C.blue });
  timeline(s, 1.0, 5.65, [["硬件支撑","显示、存储、RTC、电源"],["任务调度","FreeRTOS多任务协作"],["业务处理","多媒体、联网、OTA"],["消息上报","状态变化进入队列"],["界面刷新","LVGL统一输出"]]);
  footer(s, page++);
}

// 6 hardware
{
  const s = pptx.addSlide(); addGrid(s); title(s, "03", "硬件设计：围绕主控构建显示、存储、时间和电源链路", "HARDWARE DESIGN");
  const cards = [
    ["核心主控", "ESP32-S3R8芯片级最小系统，外接16MB Flash和40MHz晶振，兼顾集成度与扩展能力。", C.blue],
    ["显示触控", "1.83英寸240×284触控屏，ST7789P3走SPI显示，CST816T走I2C并支持中断唤醒。", C.cyan],
    ["存储系统", "Micro SD卡保存图片、视频、小说和本地升级包；W25Q128提供非易失扩展。", C.orange],
    ["时间基准", "SD3078独立RTC用于断网/掉电场景下的时间保持，并支持网络校时回写。", C.green],
    ["电源管理", "锂电池充电与ME6217 LDO供电，配合背光、WiFi和休眠策略控制整机功耗。", C.red],
  ];
  cards.forEach((c, i) => {
    const x = i % 2 === 0 ? 0.85 : 6.9;
    const y = 1.45 + Math.floor(i / 2) * 1.28;
    card(s, x, y, i === 4 ? 5.45 : 5.25, 0.9, c[0], c[1], { hSize: 14.5, bSize: 10, bar: c[2] });
  });
  footer(s, page++);
}

// 7 software
{
  const s = pptx.addSlide(); addGrid(s); title(s, "04", "软件设计：业务层处理状态，显示层统一刷新", "SOFTWARE DESIGN");
  flowBox(s, 1.0, 1.42, 2.4, 0.75, "硬件层\nSPI / I2C / GPIO / WiFi", C.blue);
  flowBox(s, 4.0, 1.42, 2.4, 0.75, "应用层\n多媒体 / 时间 / OTA / 功耗", C.orange);
  flowBox(s, 7.0, 1.42, 2.4, 0.75, "消息队列\n业务状态同步", C.cyan);
  flowBox(s, 10.0, 1.42, 2.4, 0.75, "显示层\nLVGL统一刷新", C.green);
  arrow(s, 3.4, 1.78, 4.0, 1.78); arrow(s, 6.4, 1.78, 7.0, 1.78); arrow(s, 9.4, 1.78, 10.0, 1.78);
  const modules = [
    ["UI主线程", "负责LVGL刷新与触摸事件分发，避免业务任务直接操作屏幕。"],
    ["存储工作线程", "异步读取SD卡小说、图片和视频帧，减少UI阻塞。"],
    ["网络线程", "维护WiFi状态，处理NTP、天气、MQTT和OTA通知。"],
    ["低功耗任务", "监测空闲时间和按键状态，控制LightSleep与DeepSleep。"],
  ];
  modules.forEach((m, i) => card(s, 1.0 + (i % 2) * 5.8, 3.0 + Math.floor(i / 2) * 1.15, 4.9, 0.82, m[0], m[1], { hSize: 14, bSize: 9.8, bar: [C.blue,C.orange,C.cyan,C.green][i] }));
  footer(s, page++);
}

// 8 key modules
{
  const s = pptx.addSlide(); addGrid(s); title(s, "04", "关键模块实现：多媒体、时间联网、OTA和低功耗形成完整闭环", "KEY MODULES");
  const list = [
    ["多媒体与文件系统", "SD卡挂载FatFS后统一扫描资源；图片分块读取，MJPEG帧解码刷新，TXT按页读取并记录偏移。"],
    ["时间与网络服务", "WiFi连接后使用NTP校时并回写SD3078；离线时由RTC守时，显示层周期读取统一时间缓存。"],
    ["OTA升级维护", "OneNET云端OTA与SD卡本地OTA均写入备份分区，完成校验后切换启动分区。"],
    ["低功耗控制", "120秒无操作进入LightSleep；确认键长按约3秒进入DeepSleep，兼顾快速恢复与长期收纳。"],
  ];
  list.forEach((m, i) => card(s, 0.85 + (i % 2) * 6.1, 1.55 + Math.floor(i / 2) * 1.85, 5.35, 1.25, m[0], m[1], { hSize: 15, bSize: 10.8, bar: [C.orange,C.cyan,C.blue,C.green][i] }));
  footer(s, page++);
}

// 9 testing
{
  const s = pptx.addSlide(); addGrid(s); title(s, "05", "系统调试：按功能、功耗和升级链路验证系统可用性", "SYSTEM TEST");
  const rows = [
    ["主菜单滑动", "30/30", "<50ms", "交互稳定"],
    ["SD壁纸切换", "25/30", "100~200ms", "大图解码偶发延迟"],
    ["MJPEG播放", "28/30", "约25fps", "高负载偶有掉帧"],
    ["电子书续读", "30/30", "<150ms", "阅读位置准确"],
    ["WiFi连接", "29/30", "约4.5s", "弱网下可恢复"],
    ["OTA升级", "10/10", "约70s", "回滚保护有效"],
  ];
  simpleTable(s, 0.75, 1.45, rows);
  card(s, 8.35, 1.45, 3.7, 1.0, "功耗测试结论", "10%亮度、WiFi关闭时约0.058A；100%亮度、WiFi关闭时约0.163A，亮屏功耗随背光升高明显增长。", { hSize: 14, bSize: 10.2, bar: C.orange });
  card(s, 8.35, 2.8, 3.7, 1.0, "休眠测试结论", "LightSleep适合短时空闲快速恢复；DeepSleep适合长时间不用和防误触唤醒。", { hSize: 14, bSize: 10.2, bar: C.green });
  card(s, 8.35, 4.15, 3.7, 1.0, "调试发现", "主要瓶颈集中在SD卡读取速度、大图解码时间和高负载视频刷新。", { hSize: 14, bSize: 10.2, bar: C.red });
  footer(s, page++);
}

// 10 summary
{
  const s = pptx.addSlide(); addGrid(s); title(s, "06", "总结与未来展望", "CONCLUSION");
  card(s, 0.75, 1.4, 5.45, 4.55, "研究总结", "本课题完成了基于ESP32-S3R8的低功耗触控智能手表系统，从硬件最小系统、显示触控驱动、SD卡存储、RTC时间管理，到LVGL图形界面、FreeRTOS任务调度、多媒体播放、联网服务和OTA升级，形成了一套较完整的嵌入式穿戴终端实现方案。", { hSize: 16, bSize: 12.2, bar: C.blue });
  const future = [
    ["多媒体性能", "优化缓存和解码流程，减少大图与视频场景卡顿"],
    ["功耗优化", "增加外设断电控制，进一步降低休眠电流"],
    ["结构优化", "优化PCB与屏幕布局，压缩整机厚度"],
    ["可靠性提升", "增强弱网重连、异常恢复和长期运行监控"],
  ];
  future.forEach((f, i) => card(s, 6.75, 1.4 + i * 1.13, 5.2, 0.78, f[0], f[1], { hSize: 13.5, bSize: 9.6, bar: [C.orange,C.green,C.cyan,C.red][i] }));
  footer(s, page++);
}

// 11 thanks
{
  const s = pptx.addSlide();
  s.background = { color: "EFF6FC" };
  s.addShape(pptx.ShapeType.rect, { x: 0, y: 0, w: 13.333, h: 7.5, fill: { color: "EFF6FC" }, line: { transparency: 100 } });
  s.addShape(pptx.ShapeType.rect, { x: 0, y: 5.05, w: 13.333, h: 2.45, fill: { color: "D8EBF7" }, line: { transparency: 100 } });
  for (let i = 0; i < 10; i++) {
    s.addShape(pptx.ShapeType.line, { x: 0.4 + i * 1.2, y: 6.2 - (i % 3) * 0.15, w: 0.9, h: -0.4, line: { color: "A8C8DD", transparency: 25, width: 0.8 } });
  }
  s.addText("感谢聆听", { x: 4.7, y: 2.55, w: 3.8, h: 0.55, fontSize: 29, bold: true, color: C.blue, align: "center", margin: 0 });
  s.addText("恳请各位老师批评指正", { x: 4.65, y: 3.28, w: 3.9, h: 0.26, fontSize: 13, color: C.text, align: "center", margin: 0 });
  s.addText("基于ESP32-S3R8与LVGL的低功耗触控智能手表系统设计与实现", { x: 3.25, y: 6.55, w: 6.8, h: 0.18, fontSize: 9, color: C.muted, align: "center", margin: 0 });
}

fs.mkdirSync(path.dirname(OUT), { recursive: true });
pptx.writeFile({ fileName: OUT }).then(() => console.log(OUT));
