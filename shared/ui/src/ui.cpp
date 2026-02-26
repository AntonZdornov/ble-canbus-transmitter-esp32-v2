#include <lvgl.h>
#include <stdio.h>
#include "ui.h"

// UI components
static lv_obj_t *time_value_label;
static lv_obj_t *distance_value_label;
static lv_obj_t *battery_percent_label;
static lv_obj_t *battery_caption_label;

extern const lv_font_t lv_font_montserrat_48;
extern const lv_font_t lv_font_montserrat_38;
extern const lv_font_t lv_font_montserrat_12;
extern const lv_font_t lv_font_montserrat_22;

static void set_label_text(lv_obj_t *label, const char *text) {
  if (label != nullptr) {
    lv_label_set_text(label, text);
  }
}

static void format_time_minutes(int minutes, char *buf, size_t buf_len) {
  if (minutes < 0) {
    snprintf(buf, buf_len, "--:-- h");
    return;
  }
  int hours = minutes / 60;
  int mins = minutes % 60;
  snprintf(buf, buf_len, "%d:%02d h", hours, mins);
}

void ui_init() {
  lv_obj_clean(lv_scr_act());
  lv_obj_t *root_container = lv_obj_create(lv_scr_act());
  lv_obj_set_size(root_container, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_pad_all(root_container, 10, LV_PART_MAIN);
  lv_obj_set_style_border_width(root_container, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(root_container, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(root_container, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_clear_flag(root_container, LV_OBJ_FLAG_SCROLLABLE);

  // Time (top-left)
  lv_obj_t *time_label = lv_label_create(root_container);
  lv_label_set_text(time_label, "Time");
  lv_obj_set_style_text_color(time_label, lv_color_hex(0xB0B0B0), 0);
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_12, 0);
  lv_obj_align(time_label, LV_ALIGN_TOP_LEFT, 0, 0);

  time_value_label = lv_label_create(root_container);
  lv_label_set_text(time_value_label, "--:-- h");
  lv_obj_set_style_text_color(time_value_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(time_value_label, &lv_font_montserrat_22, 0);
  lv_obj_align_to(time_value_label, time_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);

  // Distance (top-right)
  lv_obj_t *distance_label = lv_label_create(root_container);
  lv_label_set_text(distance_label, "Distance");
  lv_obj_set_style_text_color(distance_label, lv_color_hex(0xB0B0B0), 0);
  lv_obj_set_style_text_font(distance_label, &lv_font_montserrat_12, 0);
  lv_obj_align(distance_label, LV_ALIGN_TOP_RIGHT, 0, 0);

  distance_value_label = lv_label_create(root_container);
  lv_label_set_text(distance_value_label, "-- km");
  lv_obj_set_style_text_color(distance_value_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(distance_value_label, &lv_font_montserrat_22, 0);
  lv_obj_align_to(distance_value_label, distance_label, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 2);

  // Center: Battery percent
  battery_percent_label = lv_label_create(root_container);
  lv_label_set_text(battery_percent_label, "--%");
  lv_obj_set_style_text_color(battery_percent_label, lv_color_hex(0x6CEB6C), 0);
  lv_obj_set_style_text_font(battery_percent_label, &lv_font_montserrat_48, 0);
  lv_obj_align(battery_percent_label, LV_ALIGN_CENTER, 0, 12);

  battery_caption_label = lv_label_create(root_container);
  lv_label_set_text(battery_caption_label, "Battery SOC");
  lv_obj_set_style_text_color(battery_caption_label, lv_color_hex(0x808080), 0);
  lv_obj_set_style_text_font(battery_caption_label, &lv_font_montserrat_12, 0);
  lv_obj_align_to(battery_caption_label, battery_percent_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
}

void ui_set_data(const UiData &data) {
  char buf[32];

  format_time_minutes(data.time_minutes, buf, sizeof(buf));
  set_label_text(time_value_label, buf);

  if (data.distance_km < 0) {
    set_label_text(distance_value_label, "-- km");
  } else {
    snprintf(buf, sizeof(buf), "%d km", data.distance_km);
    set_label_text(distance_value_label, buf);
  }

  if (data.battery_percent < 0) {
    set_label_text(battery_percent_label, "--%");
  } else {
    snprintf(buf, sizeof(buf), "%d%%", data.battery_percent);
    set_label_text(battery_percent_label, buf);
  }
}

void updateBatteryLevel(int raw) {
  UiData data{};
  data.battery_percent = raw;
  data.distance_km = -1;
  data.time_minutes = -1;
  ui_set_data(data);
}
