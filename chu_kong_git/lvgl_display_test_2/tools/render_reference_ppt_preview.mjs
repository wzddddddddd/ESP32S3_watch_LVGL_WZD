import fs from "node:fs/promises";
import path from "node:path";
import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const { PresentationFile, FileBlob } = require("@oai/artifact-tool");

const ROOT = "C:/Users/86177/Desktop/ESP32_chukong/chu_kong_git/lvgl_display_test_2";
const PPTX = "C:/Users/86177/Downloads/校园失物招领系统答辩汇报.pptx";
const OUT_DIR = path.join(ROOT, "outputs", "reference_ppt_preview");

async function saveBlob(blob, outPath) {
  const ab = await blob.arrayBuffer();
  await fs.writeFile(outPath, Buffer.from(ab));
}

const presentation = await PresentationFile.importPptx(await FileBlob.load(PPTX));
await fs.mkdir(OUT_DIR, { recursive: true });
for (let i = 0; i < presentation.slides.count; i += 1) {
  const slide = presentation.slides.getItem(i);
  const png = await presentation.export({ slide, format: "png", scale: 1 });
  await saveBlob(png, path.join(OUT_DIR, `slide-${String(i + 1).padStart(2, "0")}.png`));
}
console.log(JSON.stringify({ slides: presentation.slides.count, outDir: OUT_DIR }, null, 2));
