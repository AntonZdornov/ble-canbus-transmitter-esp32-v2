#pragma once

/*
 * LVGL config for PC simulator.
 * Sync LV_HOR_RES_MAX/LV_VER_RES_MAX with the target LCD if needed.
 */

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

#define LV_HOR_RES_MAX 240
#define LV_VER_RES_MAX 280

#define LV_USE_LOG 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_38 1
#define LV_FONT_MONTSERRAT_48 1

#define LV_FONT_DEFAULT &lv_font_montserrat_12

#define LV_USE_DRAW_SW 1

#define LV_USE_TICK_CUSTOM 0
