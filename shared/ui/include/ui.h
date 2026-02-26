#pragma once

#include <lvgl.h>
#include "ui_model.h"

void ui_init();
void ui_set_data(const UiData &data);

// Backward-compat wrapper
void updateBatteryLevel(int raw);
