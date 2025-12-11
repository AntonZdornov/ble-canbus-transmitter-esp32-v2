#pragma once

#include <lvgl.h>

extern lv_obj_t *time_label;

void initUI();
void updateBatteryLevel(int raw);
