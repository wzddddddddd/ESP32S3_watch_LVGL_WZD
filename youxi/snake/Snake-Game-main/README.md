# Colorful Snake Game on Raspberry Pi Pico: Development Process & Achievements

## Introduction
This project was part of the **"Let's Do Activity"** (Session 2) organized by DigiKey and EEPW, focusing on developing a *Snake Game* using the **Raspberry Pi Pico** microcontroller. The goal was to implement core game mechanics, integrate a custom GUI with LVGL, and add personalized audio effects using PWM-driven speakers. Below, I detail my journey through hardware setup, software development, and creative customization.

---

## Hardware Setup
### Core Components
- **Raspberry Pi Pico** (ARM Cortex-M0+ MCU)  
- **Waveshare Pico-LCD-1.14** (1.14" LCD with directional buttons)  
- **PIOO Omnibus (Dual Expander Board)** for extended I/O  
- **8Ω Speaker** connected via PWM for audio effects  

**System Architecture**  
The Pico was mounted on the Dual Expander Board's central slot. The LCD occupied **Deck 1**, while **Deck 2** hosted the speaker via GPIO 27 (PWM output).  
![System Diagram](http://uphotos.eepw.com.cn/1713930123/pics/1724036918676284.png)

---

## Software Development
### Toolchain Setup
Initially, I struggled with Keil MDK due to licensing and compilation issues. After days of troubleshooting, I switched to the **official Pico SDK** with **VS Code** and **Visual Studio 2022**, following these steps:  
1. Install **Pico C/C++ SDK**, CMake, and VS Code.  
2. Configure `PICO_SDK_PATH` in the developer PowerShell.  
3. Compile code using:  
   ```bash
   mkdir build && cd build  
   cmake -G "NMake Makefiles" ..  
   nmake  
   ```

4. Flashed the generated `.uf2` file to the Pico.

### LVGL Integration for GUI
The **Waveshare LCD** utilized LVGL (Light and Versatile Graphics Library) with optimizations:

DMA Transfers: Reduced CPU usage to **<35%** by offloading SPI data transfer.

- **Double Buffering**: Ensured smooth animations by rendering in one buffer while transferring another.

- **Custom Widgets**: Designed four interactive pages:

  - **Splash Screen**: Displays the game logo.

  - **Level Selection**: Allows users to choose difficulty (Level 1-8).

  - **Gameplay Interface**: Renders the snake, food, and collision logic.

  - **Results Page**: Shows scores and game-over messages.

### Game Logic Implementation
#### Key Features
1. **Movement Mechanics**
   - Directional constraints: Vertical movement allows only horizontal turns, and vice versa.
2. **Speed Scaling**
   - Progressive difficulty: From 300ms/step (Level 1) to 80ms/step (Level 8).
3. **Collision Detection**
   - **Food Consumption**: Increases snake length and triggers sound effects.
   - **Boundary/Body Collision**: Ends the game and resets progress.

#### Data Structures
- `snake_body_t`: Stores coordinates of snake segments.
- `my_eat_snake`: Manages snake length, position, and growth.
- `my_snake_food`: Tracks food location and remaining count.
---
### Personalized Audio System
#### PWM-Driven Sound Effects
The speaker was controlled via GPIO 27 using **Pulse Width Modulation (PWM)**. Key steps:

1. PWM Configuration
```c
gpio_set_function(27, GPIO_FUNC_PWM);  
pwm_config config = pwm_get_default_config();  
pwm_config_set_clkdiv(&config, 1.f);  
pwm_config_set_wrap(&config, 20000);  
pwm_init(slice_num, &config, true); 
``` 
2. Interrupt-Driven Audio Playback
   - Defined musical notes (e.g., `C4=262Hz`, `D4=294Hz`) in an array.
   - Used `on_pwm_wrap()` interrupt to play:
      - **Background Music**: Triggered during gameplay (`sound_state[0] = 1`).
      - **Sound Effects**: Played when eating food (`sound_state[1] = 1`) or game-over (`sound_state[1] = 2`).

---
## Challenges & Solutions
1. Development Environment

   - Issue: Keil MDK compatibility issues and compilation errors.

   - Solution: Migrated to Pico SDK with VS Code, reducing build time by 50%.

2. Audio Quality Limitations

   - Issue: Distortion due to PWM signal noise.

   - Solution: Implemented frequency modulation and fade effects to minimize noise.

3. Memory Optimization

   - Issue: LVGL memory spikes during animations.

   - Solution: Reduced buffer sizes and simplified widget hierarchies.

## Conclusion & Learning Outcomes
This project enhanced my expertise in **embedded systems, real-time programming**, and **GUI development**. Key takeaways:

- **Adaptability**: Successfully pivoted toolsets to overcome environmental hurdles.

- **Problem-Solving**: Debugged hardware-software integration challenges.

- **Creativity**: Designed a responsive GUI and dynamic audio system within resource constraints.

**Demo Video**: <https://youtu.be/m1qLWg0Lp00>

**EEPW Forum Post**: 
<https://forum.eepw.com.cn/thread/384440/1> 
<https://forum.eepw.com.cn/thread/384453/1> 
<https://forum.eepw.com.cn/thread/384471/1> 
<https://forum.eepw.com.cn/thread/384521/1>
---
