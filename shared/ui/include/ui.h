#pragma once

#include <lvgl.h>
#include "ui_model.h"

void ui_init();
void ui_set_data(const UiData &data);
void ui_set_reset_distance_cb(void (*cb)());

// Backward-compat wrapper
void updateBatteryLevel(int raw);
