#include <lvgl.h>
#include <stdio.h>
#include "ui.h"

// UI components
static lv_obj_t *time_value_label;
static lv_obj_t *distance_value_label;
static lv_obj_t *fuel_value_label;
static lv_obj_t *battery_percent_label;
static lv_obj_t *battery_caption_label;
static lv_obj_t *drive_mode_label;

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
    snprintf(buf, buf_len, "--:--");
    return;
  }
  int hours = minutes / 60;
  int mins = minutes % 60;
  snprintf(buf, buf_len, "%d:%02d", hours, mins);
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

  // Top row: Time and Distance, each half width and centered
  lv_obj_t *top_row = lv_obj_create(root_container);
  lv_obj_set_size(top_row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(top_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(top_row, 0, 0);
  lv_obj_clear_flag(top_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(top_row, 0, 0);
  lv_obj_set_style_pad_row(top_row, 0, 0);
  lv_obj_set_style_pad_column(top_row, 0, 0);
  lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(top_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  // Time block (left half)
  lv_obj_t *time_block = lv_obj_create(top_row);
  lv_obj_set_size(time_block, LV_PCT(50), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(time_block, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(time_block, 0, 0);
  lv_obj_clear_flag(time_block, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(time_block, 0, 0);

  lv_obj_t *time_label = lv_label_create(time_block);
  lv_label_set_text(time_label, "Time");
  lv_obj_set_style_text_color(time_label, lv_color_hex(0xB0B0B0), 0);
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_12, 0);
  lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 0);

  time_value_label = lv_label_create(time_block);
  lv_label_set_text(time_value_label, "--:--");
  lv_obj_set_style_text_color(time_value_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(time_value_label, &lv_font_montserrat_22, 0);
  lv_obj_align_to(time_value_label, time_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

  // Distance block (right half)
  lv_obj_t *distance_block = lv_obj_create(top_row);
  lv_obj_set_size(distance_block, LV_PCT(50), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(distance_block, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(distance_block, 0, 0);
  lv_obj_clear_flag(distance_block, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(distance_block, 0, 0);

  lv_obj_t *distance_label = lv_label_create(distance_block);
  lv_label_set_text(distance_label, "Distance");
  lv_obj_set_style_text_color(distance_label, lv_color_hex(0xB0B0B0), 0);
  lv_obj_set_style_text_font(distance_label, &lv_font_montserrat_12, 0);
  lv_obj_align(distance_label, LV_ALIGN_TOP_MID, 0, 0);

  distance_value_label = lv_label_create(distance_block);
  lv_label_set_text(distance_value_label, "-- km");
  lv_obj_set_style_text_color(distance_value_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(distance_value_label, &lv_font_montserrat_22, 0);
  lv_obj_align_to(distance_value_label, distance_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

  // Fuel level (centered under top row)
  lv_obj_t *fuel_label = lv_label_create(root_container);
  lv_label_set_text(fuel_label, "Fuel");
  lv_obj_set_style_text_color(fuel_label, lv_color_hex(0xB0B0B0), 0);
  lv_obj_set_style_text_font(fuel_label, &lv_font_montserrat_12, 0);
  lv_obj_align_to(fuel_label, top_row, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

  fuel_value_label = lv_label_create(root_container);
  lv_label_set_text(fuel_value_label, "--%");
  lv_obj_set_style_text_color(fuel_value_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(fuel_value_label, &lv_font_montserrat_22, 0);
  lv_obj_align_to(fuel_value_label, fuel_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

  // Drive mode indicator (center)
  drive_mode_label = lv_label_create(root_container);
  lv_label_set_text(drive_mode_label, "--");
  lv_obj_set_style_text_color(drive_mode_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(drive_mode_label, &lv_font_montserrat_22, 0);
  lv_obj_align(drive_mode_label, LV_ALIGN_CENTER, 0, -26);

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

  if (data.fuel_percent < 0) {
    set_label_text(fuel_value_label, "--%");
  } else {
    snprintf(buf, sizeof(buf), "%d%%", data.fuel_percent);
    set_label_text(fuel_value_label, buf);
  }

  if (data.battery_percent < 0) {
    set_label_text(battery_percent_label, "--%");
  } else {
    snprintf(buf, sizeof(buf), "%d%%", data.battery_percent);
    set_label_text(battery_percent_label, buf);
  }

  // Drive mode: ICE if engine_on == 1, EV if 0
  if (drive_mode_label) {
    if (data.engine_on == 1) {
      set_label_text(drive_mode_label, "ICE");
    } else if (data.engine_on == 0) {
      set_label_text(drive_mode_label, "EV");
    } else {
      set_label_text(drive_mode_label, "--");
    }
  }
}

void updateBatteryLevel(int raw) {
  UiData data{};
  data.battery_percent = raw;
  data.speed_kmh = -1;
  data.fuel_percent = -1;
  data.distance_km = -1;
  data.engine_on = -1;
  data.time_minutes = -1;
  ui_set_data(data);
}
