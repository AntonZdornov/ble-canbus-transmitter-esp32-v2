#pragma once

// Extend the library-provided LVGL config to enable extra fonts.
// This relies on GCC/Clang's include_next, which is supported by ESP32 toolchains.
#include_next "lv_conf.h"

#undef LV_FONT_MONTSERRAT_26
#define LV_FONT_MONTSERRAT_26 1

#undef LV_FONT_MONTSERRAT_28
#define LV_FONT_MONTSERRAT_28 1

#undef LV_FONT_MONTSERRAT_20
#define LV_FONT_MONTSERRAT_20 1
