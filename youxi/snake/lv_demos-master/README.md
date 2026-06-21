# lv_demos

Extended demo applications for LVGL (Light and Versatile Graphics Library). This repository contains niche and specialized demo applications that showcase advanced LVGL features and use cases.

## Overview

This repository contains additional standalone demo applications beyond the core LVGL demos. Each demo is self-contained and demonstrates specific features or real-world applications. Demos can be easily integrated into LVGL-based projects through CMake and FetchContent.

## Repository Structure

```
lv_demos/
├── src/
│   ├── ebike/               # eBike demo
│   │   ├── *.c              # Demo source files
│   │   └── *.h              # Demo header files
│   ├── smartwatch/          # Stress test demo
│   │   ├── *.c
│   │   └── *.h
│   └── lv_demos.h           # Main header with feature guards
├── CMakeLists.txt           # Build configuration
├── lv_demos_ext.h           # Public API header
└── README.md                # This file
```

## Demo Features

Each demo is conditionally compiled based on `LV_USE_DEMO_*` defines set in `lv_conf.h`. This allows you to include only the demos you need in your project, keeping the binary size minimal.

### Available Demos

Refer to `src/lv_demos.h` for the complete list of available demos and their corresponding `LV_USE_DEMO_*` flags.

## Building

The easiest way to try one of the demos is to add this repository to your application and build the source files by globbing them.

TODO: Document this approach

## CMake

You can also use CMake to integrate these demos into your application

### Prerequisites

- CMake 3.11 or higher
- C compiler (GCC, Clang, MSVC, etc.)
- LVGL library (as a dependency)

### Integration with FetchContent

The recommended way to use lv_demos_ext is to add it to your project's main CMakeLists.txt **after** LVGL:

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_app)

# 1. Add LVGL first via subdirectory or fetchcontent
add_subdirectory(lvgl)

# 2. Fetch and add lv_demos_ext
include(FetchContent)
FetchContent_Declare(
    lv_demos_ext
    GIT_REPOSITORY https://github.com/lvgl/lv_demos
    GIT_TAG master
)
FetchContent_MakeAvailable(lv_demos_ext)

# 3. Link lv_demos_ext with LVGL
target_link_libraries(lvgl PUBLIC lv_demos_ext)

# 4. Link LVGL to your application
add_executable(my_app src/main.c)
target_link_libraries(my_app PRIVATE lvgl)
```

**Important:** LVGL must be added before lv_demos_ext, as lv_demos_ext depends on LVGL being available.

### CMake Integration

For comprehensive CMake integration guidance, refer to the [LVGL CMake documentation](https://docs.lvgl.io/master/details/integration/building/cmake.html).

### Using with lv_port_linux

The [lv_port_linux](https://github.com/lvgl/lv_port_linux) repository can automatically fetch and integrate lv_demos_ext.
Simply enable one of the demos in your lv_port_linux configuration, and the build system will automatically include lv_demos_ext.

## Usage

Once integrated, enable the desired demos by defining their corresponding `LV_USE_DEMO_*` flags in your `lv_conf.h`:

```c
#define LV_USE_DEMO_EBIKE 1
#define LV_USE_DEMO_STRESS 1
// Add other demo defines as needed
```

Then include and call the demo in your application:

```c
#include "lv_demos_ext.h"

int main(void) {
    lv_init();
    
    /* Display a demo */
    lv_demo_ebike();
    
    /* Your event loop */
    while(1) {
        uint32_t sleep_ms = lv_timer_handler();
        lv_sleep_ms(sleep_ms);
    }
    
    return 0;
}
```

## Dependencies

- **LVGL**: Light and Versatile Graphics Library - required for all demos

## Contributing

Contributions are welcome! Please submit pull requests with:
- Clear descriptions of changes
- Any new demos should follow the existing code structure
- Proper documentation for new functionality

## Support

For questions regarding LVGL, visit: [LVGL Documentation](https://docs.lvgl.io/)

For issues specific to this repository, please open an issue on Github.
