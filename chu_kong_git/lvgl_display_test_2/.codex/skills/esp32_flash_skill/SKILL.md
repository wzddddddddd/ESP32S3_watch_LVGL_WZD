---
name: esp32_flash_skill
description: Use this skill when user asks to run ESP32 deployment pipeline after code changes: build, clear target COM port occupancy, flash firmware, and clear the port again. Supports fixed COMx and defaults to COM6.
---

# ESP32 Flash Skill

## When to use
Use this skill when user wants the standard operational flow:
1. Build succeeds.
2. Clear serial port occupancy (for example COM6).
3. Flash firmware to that port.
4. Clear the same port again after flashing.

## Required defaults
- Default port: `COM6`
- Do not run monitor unless user explicitly asks.
- Always clean occupying processes before flash and after flash.

## Execution steps
1. Run build:
   - `idf.py build`
2. Clear port holders:
   - Kill processes whose command line matches `COMx`, `idf.py ... monitor`, `esp_idf_monitor`, `pyserial`, or `miniterm`.
3. Run flash only:
   - `idf.py -p COMx flash`
4. Clear port holders again with the same filter.
5. Report status succinctly:
   - Build result
   - Flash result
   - Whether cleanup found/killed holders

## Command wrapper
Run this script from repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\.codex\skills\esp32_flash_skill\scripts\run.ps1 -Port COM6
```

Optional:
- Skip build if already built in current cycle:
  - `-SkipBuild`
