LVGL_DEMOS_EXT_DIR ?= $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
CSRCS += $(shell find -L $(LVGL_DEMOS_EXT_DIR) -name "*.c")
