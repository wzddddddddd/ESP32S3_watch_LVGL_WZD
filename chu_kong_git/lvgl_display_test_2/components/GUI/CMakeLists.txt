file(GLOB_RECURSE srcs *.c          
                    custom/*.c
                    generated/*.c
                    generated/guider_customer_fonts/*.c
                    generated/guider_fonts/*.c
                    generated/images/*.c
                    )

# Exclude upstream Pico demo source that is not compatible with ESP-IDF.
list(FILTER srcs EXCLUDE REGEX ".*[\\\\/]game[\\\\/]snake[\\\\/]LVGL_example\\.c$")

# Keep newly added generated screens linked even when CMake glob cache is stale.
list(APPEND srcs
    generated/setup_scr_screen_game.c
    generated/setup_scr_screen_weather.c
)

# Flappy Bird game (SquareLine export adapted for LVGL8)
file(GLOB flappy_srcs
    game/flappy_bird/*.c
    game/flappy_bird/image/*.c
)
list(APPEND srcs ${flappy_srcs})
 
set(include_dirs      
    custom 
    generated 
    generated/guider_customer_fonts
    generated/guider_fonts
    generated/images
    game/flappy_bird
    )
 
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES lvgl bsp ota My_image
    )             
