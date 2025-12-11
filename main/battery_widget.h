#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void battery_widget_create(lv_obj_t *parent);          // создать виджет
void battery_set_level(uint8_t percent);               // мгновенно установить уровень
void battery_animate_to(uint8_t target, uint32_t ms);  // анимировать до уровня

#ifdef __cplusplus
}
#endif