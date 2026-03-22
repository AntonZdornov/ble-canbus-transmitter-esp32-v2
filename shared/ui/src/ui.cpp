#include <lvgl.h>
#include <stdio.h>
#include "ui.h"

// UI components
static lv_obj_t *main_screen;
static lv_obj_t *service_screen;
static lv_obj_t *time_value_label;
static lv_obj_t *distance_value_label;
static lv_obj_t *ev_distance_value_label;
static lv_obj_t *fuel_icon_canvas;
static lv_obj_t *fuel_value_label;
static lv_obj_t *soc_icon_canvas;
static lv_obj_t *battery_percent_label;
static lv_obj_t *service_distance_value_label;
static lv_obj_t *service_ev_distance_value_label;
static lv_obj_t *wifi_icon_canvas;
static lv_obj_t *wifi_toggle_label;
static void (*reset_distance_cb)() = nullptr;
static void (*wifi_reconnect_cb)() = nullptr;
static void (*wifi_toggle_cb)() = nullptr;

static lv_color_t icon_fuel_color = {0};
static lv_color_t icon_soc_color = {0};

static lv_color_t fuel_icon_buf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(22, 22)];
static lv_color_t soc_icon_buf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(22, 22)];
static lv_color_t wifi_icon_buf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(22, 22)];

extern const lv_font_t lv_font_montserrat_48;
extern const lv_font_t lv_font_montserrat_38;
extern const lv_font_t lv_font_montserrat_28;
extern const lv_font_t lv_font_montserrat_26;
extern const lv_font_t lv_font_montserrat_12;
extern const lv_font_t lv_font_montserrat_20;
extern const lv_font_t lv_font_montserrat_22;

static void set_label_text(lv_obj_t *label, const char *text) {
  if (label != nullptr) {
    lv_label_set_text(label, text);
  }
}

static lv_color_t color_for_fuel(int percent) {
  if (percent < 20) {
    return lv_color_hex(0xFF4040); // red
  } else if (percent < 35) {
    return lv_color_hex(0xFFD050); // yellow
  } else if (percent < 75) {
    return lv_color_hex(0x7CFF7C); // green
  }
  return lv_color_hex(0x1D34FF);   // blue
}

static lv_color_t color_for_soc(int percent) {
  if (percent < 25) {
    return lv_color_hex(0xFF4040); // red
  } else if (percent < 45) {
    return lv_color_hex(0xFFD050); // yellow
  } else if (percent < 80) {
    return lv_color_hex(0x7CFF7C); // green
  }
  return lv_color_hex(0x1D34FF);   // blue
}

static void draw_fuel_icon(lv_obj_t *canvas, lv_color_t c) {
  if (!canvas) return;
  lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_TRANSP);

  lv_draw_rect_dsc_t circle;
  lv_draw_rect_dsc_init(&circle);
  circle.bg_opa = LV_OPA_COVER;
  circle.bg_color = c;
  circle.radius = LV_RADIUS_CIRCLE;
  lv_canvas_draw_rect(canvas, 6, 7, 10, 10, &circle);

  lv_draw_line_dsc_t tri;
  lv_draw_line_dsc_init(&tri);
  tri.color = c;
  tri.width = 2;
  lv_point_t l1[] = {{11, 2}, {6, 9}};
  lv_point_t l2[] = {{6, 9}, {16, 9}};
  lv_point_t l3[] = {{16, 9}, {11, 2}};
  lv_canvas_draw_line(canvas, l1, 2, &tri);
  lv_canvas_draw_line(canvas, l2, 2, &tri);
  lv_canvas_draw_line(canvas, l3, 2, &tri);
}

static void draw_soc_icon(lv_obj_t *canvas, lv_color_t c) {
  if (!canvas) return;
  lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_TRANSP);

  lv_draw_rect_dsc_t body;
  lv_draw_rect_dsc_init(&body);
  body.bg_opa = LV_OPA_TRANSP;
  body.border_opa = LV_OPA_COVER;
  body.border_color = c;
  body.border_width = 2;
  body.radius = 2;
  lv_canvas_draw_rect(canvas, 7, 4, 8, 14, &body);

  lv_draw_rect_dsc_t tip;
  lv_draw_rect_dsc_init(&tip);
  tip.bg_opa = LV_OPA_COVER;
  tip.bg_color = c;
  tip.radius = 1;
  lv_canvas_draw_rect(canvas, 9, 2, 4, 2, &tip);

  lv_draw_rect_dsc_t level;
  lv_draw_rect_dsc_init(&level);
  level.bg_opa = LV_OPA_COVER;
  level.bg_color = c;
  level.radius = 1;
  lv_canvas_draw_rect(canvas, 9, 10, 4, 6, &level);
}

static void draw_wifi_icon(lv_obj_t *canvas, lv_color_t c, bool connected) {
  if (!canvas) return;
  lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_TRANSP);

  lv_draw_line_dsc_t d;
  lv_draw_line_dsc_init(&d);
  d.color = c;
  d.width = 2;

  lv_point_t arc1[] = {{3, 11}, {7, 8}, {11, 7}, {15, 8}, {19, 11}};
  lv_point_t arc2[] = {{6, 13}, {9, 11}, {11, 10}, {13, 11}, {16, 13}};
  lv_point_t arc3[] = {{9, 16}, {11, 15}, {13, 16}};
  lv_canvas_draw_line(canvas, arc1, 5, &d);
  lv_canvas_draw_line(canvas, arc2, 5, &d);
  lv_canvas_draw_line(canvas, arc3, 3, &d);

  lv_draw_rect_dsc_t dot;
  lv_draw_rect_dsc_init(&dot);
  dot.bg_opa = LV_OPA_COVER;
  dot.bg_color = c;
  dot.radius = LV_RADIUS_CIRCLE;
  lv_canvas_draw_rect(canvas, 10, 18, 3, 3, &dot);

  if (!connected) {
    lv_point_t slash[] = {{5, 19}, {19, 4}};
    lv_canvas_draw_line(canvas, slash, 2, &d);
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

static void handle_gesture_event(lv_event_t *e) {
  lv_obj_t *target = lv_event_get_target(e);
  lv_indev_t *indev = lv_indev_get_act();
  if (!indev) return;

  lv_dir_t dir = lv_indev_get_gesture_dir(indev);
  if (target == main_screen && dir == LV_DIR_LEFT) {
    lv_scr_load(service_screen);
  } else if (target == service_screen && dir == LV_DIR_RIGHT) {
    lv_scr_load(main_screen);
  }
}

static void handle_reset_btn_event(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED && reset_distance_cb) {
    reset_distance_cb();
  }
}

static void handle_wifi_icon_event(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED && wifi_reconnect_cb) {
    wifi_reconnect_cb();
  }
}

static void handle_wifi_toggle_btn_event(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED && wifi_toggle_cb) {
    wifi_toggle_cb();
  }
}

void ui_init() {
  main_screen = lv_obj_create(NULL);
  service_screen = lv_obj_create(NULL);
  lv_scr_load(main_screen);

  lv_obj_add_event_cb(main_screen, handle_gesture_event, LV_EVENT_GESTURE, nullptr);
  lv_obj_add_event_cb(service_screen, handle_gesture_event, LV_EVENT_GESTURE, nullptr);
  lv_obj_set_scroll_dir(service_screen, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(service_screen, LV_SCROLLBAR_MODE_OFF);

  lv_obj_t *root_container = lv_obj_create(main_screen);
  lv_obj_set_size(root_container, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_pad_all(root_container, 10, LV_PART_MAIN);
  lv_obj_set_style_border_width(root_container, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(root_container, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(root_container, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_clear_flag(root_container, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *header_row = lv_obj_create(root_container);
  lv_obj_set_size(header_row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(header_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(header_row, 0, 0);
  lv_obj_clear_flag(header_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_left(header_row, 20, 0);
  lv_obj_set_style_pad_right(header_row, 20, 0);
  lv_obj_set_style_pad_top(header_row, 1, 0);
  lv_obj_set_style_pad_bottom(header_row, 1, 0);
  lv_obj_set_flex_flow(header_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(header_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  time_value_label = lv_label_create(header_row);
  lv_label_set_text(time_value_label, "--:--");
  lv_obj_set_style_text_color(time_value_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(time_value_label, &lv_font_montserrat_22, 0);

  lv_obj_t *wifi_icon_hitbox = lv_obj_create(header_row);
  lv_obj_set_size(wifi_icon_hitbox, 44, 44);
  lv_obj_set_style_bg_opa(wifi_icon_hitbox, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(wifi_icon_hitbox, 0, 0);
  lv_obj_set_style_pad_all(wifi_icon_hitbox, 0, 0);
  lv_obj_clear_flag(wifi_icon_hitbox, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(wifi_icon_hitbox, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(wifi_icon_hitbox, handle_wifi_icon_event, LV_EVENT_CLICKED, nullptr);

  wifi_icon_canvas = lv_canvas_create(wifi_icon_hitbox);
  lv_canvas_set_buffer(wifi_icon_canvas, wifi_icon_buf, 22, 22, LV_IMG_CF_TRUE_COLOR);
  draw_wifi_icon(wifi_icon_canvas, lv_color_hex(0x6C6C6C), false);
  lv_obj_center(wifi_icon_canvas);

  lv_obj_t *bottom_row = lv_obj_create(root_container);
  lv_obj_set_size(bottom_row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(bottom_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(bottom_row, 0, 0);
  lv_obj_clear_flag(bottom_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(bottom_row, 0, 0);
  lv_obj_set_flex_flow(bottom_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bottom_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_align(bottom_row, LV_ALIGN_BOTTOM_MID, 0, -6);

  lv_obj_t *fuel_block = lv_obj_create(bottom_row);
  lv_obj_set_size(fuel_block, LV_PCT(50), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(fuel_block, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(fuel_block, 0, 0);
  lv_obj_clear_flag(fuel_block, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(fuel_block, 0, 0);

  lv_obj_t *distance_label = lv_label_create(fuel_block);
  lv_label_set_text(distance_label, "Dist");
  lv_obj_set_style_text_color(distance_label, lv_color_hex(0xB0B0B0), 0);
  lv_obj_set_style_text_font(distance_label, &lv_font_montserrat_22, 0);
  lv_obj_align(distance_label, LV_ALIGN_TOP_MID, 0, 0);

  distance_value_label = lv_label_create(fuel_block);
  lv_label_set_text(distance_value_label, "-- km");
  lv_obj_set_style_text_color(distance_value_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(distance_value_label, &lv_font_montserrat_22, 0);
  lv_obj_align_to(distance_value_label, distance_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

  lv_obj_t *fuel_sep = lv_obj_create(fuel_block);
  lv_obj_set_size(fuel_sep, LV_PCT(82), 1);
  lv_obj_set_style_bg_color(fuel_sep, lv_color_hex(0x5A5A5A), 0);
  lv_obj_set_style_bg_opa(fuel_sep, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(fuel_sep, 0, 0);
  lv_obj_set_style_radius(fuel_sep, 0, 0);
  lv_obj_clear_flag(fuel_sep, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align_to(fuel_sep, distance_value_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

  lv_obj_t *fuel_label = lv_label_create(fuel_block);
  lv_label_set_text(fuel_label, "Fuel");
  lv_obj_set_style_text_color(fuel_label, lv_color_hex(0xB0B0B0), 0);
  lv_obj_set_style_text_font(fuel_label, &lv_font_montserrat_22, 0);
  lv_obj_align_to(fuel_label, fuel_sep, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

  lv_obj_t *fuel_row = lv_obj_create(fuel_block);
  lv_obj_set_size(fuel_row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(fuel_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(fuel_row, 0, 0);
  lv_obj_clear_flag(fuel_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(fuel_row, 0, 0);
  lv_obj_set_style_pad_column(fuel_row, 4, 0);
  lv_obj_set_flex_flow(fuel_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(fuel_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_align_to(fuel_row, fuel_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

  fuel_icon_canvas = lv_canvas_create(fuel_row);
  lv_canvas_set_buffer(fuel_icon_canvas, fuel_icon_buf, 22, 22, LV_IMG_CF_TRUE_COLOR);
  icon_fuel_color = lv_color_hex(0x6C6C6C);
  draw_fuel_icon(fuel_icon_canvas, icon_fuel_color);

  fuel_value_label = lv_label_create(fuel_row);
  lv_label_set_text(fuel_value_label, "--%");
  lv_obj_set_style_text_color(fuel_value_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(fuel_value_label, &lv_font_montserrat_28, 0);

  lv_obj_t *battery_block = lv_obj_create(bottom_row);
  lv_obj_set_size(battery_block, LV_PCT(50), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(battery_block, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(battery_block, 0, 0);
  lv_obj_clear_flag(battery_block, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(battery_block, 0, 0);

  lv_obj_t *ev_distance_label = lv_label_create(battery_block);
  lv_label_set_text(ev_distance_label, "EV");
  lv_obj_set_style_text_color(ev_distance_label, lv_color_hex(0xB0B0B0), 0);
  lv_obj_set_style_text_font(ev_distance_label, &lv_font_montserrat_22, 0);
  lv_obj_align(ev_distance_label, LV_ALIGN_TOP_MID, 0, 0);

  ev_distance_value_label = lv_label_create(battery_block);
  lv_label_set_text(ev_distance_value_label, "-- km");
  lv_obj_set_style_text_color(ev_distance_value_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(ev_distance_value_label, &lv_font_montserrat_22, 0);
  lv_obj_align_to(ev_distance_value_label, ev_distance_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

  lv_obj_t *soc_sep = lv_obj_create(battery_block);
  lv_obj_set_size(soc_sep, LV_PCT(82), 1);
  lv_obj_set_style_bg_color(soc_sep, lv_color_hex(0x5A5A5A), 0);
  lv_obj_set_style_bg_opa(soc_sep, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(soc_sep, 0, 0);
  lv_obj_set_style_radius(soc_sep, 0, 0);
  lv_obj_clear_flag(soc_sep, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align_to(soc_sep, ev_distance_value_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

  lv_obj_t *battery_label = lv_label_create(battery_block);
  lv_label_set_text(battery_label, "SOC");
  lv_obj_set_style_text_color(battery_label, lv_color_hex(0xB0B0B0), 0);
  lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_22, 0);
  lv_obj_align_to(battery_label, soc_sep, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

  lv_obj_t *soc_row = lv_obj_create(battery_block);
  lv_obj_set_size(soc_row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(soc_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(soc_row, 0, 0);
  lv_obj_clear_flag(soc_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(soc_row, 0, 0);
  lv_obj_set_style_pad_column(soc_row, 4, 0);
  lv_obj_set_flex_flow(soc_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(soc_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_align_to(soc_row, battery_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

  soc_icon_canvas = lv_canvas_create(soc_row);
  lv_canvas_set_buffer(soc_icon_canvas, soc_icon_buf, 22, 22, LV_IMG_CF_TRUE_COLOR);
  icon_soc_color = lv_color_hex(0x6C6C6C);
  draw_soc_icon(soc_icon_canvas, icon_soc_color);

  battery_percent_label = lv_label_create(soc_row);
  lv_label_set_text(battery_percent_label, "--%");
  lv_obj_set_style_text_color(battery_percent_label, lv_color_hex(0x6CEB6C), 0);
  lv_obj_set_style_text_font(battery_percent_label, &lv_font_montserrat_28, 0);

  lv_obj_t *service_root = lv_obj_create(service_screen);
  lv_obj_set_size(service_root, LV_PCT(100), LV_PCT(135));
  lv_obj_set_style_pad_all(service_root, 10, LV_PART_MAIN);
  lv_obj_set_style_border_width(service_root, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(service_root, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(service_root, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_clear_flag(service_root, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *service_title = lv_label_create(service_root);
  lv_label_set_text(service_title, "Distance to Service");
  lv_obj_set_style_text_color(service_title, lv_color_hex(0xB0B0B0), 0);
  lv_obj_set_style_text_font(service_title, &lv_font_montserrat_22, 0);
  lv_obj_align(service_title, LV_ALIGN_CENTER, 0, -100);

  lv_obj_t *service_distance_label = lv_label_create(service_root);
  lv_label_set_text(service_distance_label, "Total Distance");
  lv_obj_set_style_text_color(service_distance_label, lv_color_hex(0xB0B0B0), 0);
  lv_obj_set_style_text_font(service_distance_label, &lv_font_montserrat_20, 0);
  lv_obj_align(service_distance_label, LV_ALIGN_CENTER, 0, -46);

  service_distance_value_label = lv_label_create(service_root);
  lv_label_set_text(service_distance_value_label, "-- km");
  lv_obj_set_style_text_color(service_distance_value_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(service_distance_value_label, &lv_font_montserrat_20, 0);
  lv_obj_align(service_distance_value_label, LV_ALIGN_CENTER, 0, -18);

  lv_obj_t *service_ev_distance_label = lv_label_create(service_root);
  lv_label_set_text(service_ev_distance_label, "EV Distance");
  lv_obj_set_style_text_color(service_ev_distance_label, lv_color_hex(0xB0B0B0), 0);
  lv_obj_set_style_text_font(service_ev_distance_label, &lv_font_montserrat_20, 0);
  lv_obj_align(service_ev_distance_label, LV_ALIGN_CENTER, 0, 14);

  service_ev_distance_value_label = lv_label_create(service_root);
  lv_label_set_text(service_ev_distance_value_label, "-- km");
  lv_obj_set_style_text_color(service_ev_distance_value_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(service_ev_distance_value_label, &lv_font_montserrat_20, 0);
  lv_obj_align(service_ev_distance_value_label, LV_ALIGN_CENTER, 0, 42);

  lv_obj_t *reset_btn = lv_btn_create(service_root);
  lv_obj_set_size(reset_btn, 120, 40);
  lv_obj_align_to(reset_btn, service_ev_distance_value_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 16);
  lv_obj_add_event_cb(reset_btn, handle_reset_btn_event, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *reset_label = lv_label_create(reset_btn);
  lv_label_set_text(reset_label, "Reset");
  lv_obj_center(reset_label);

  lv_obj_t *wifi_toggle_btn = lv_btn_create(service_root);
  lv_obj_set_size(wifi_toggle_btn, 120, 40);
  lv_obj_align_to(wifi_toggle_btn, reset_btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);
  lv_obj_add_event_cb(wifi_toggle_btn, handle_wifi_toggle_btn_event, LV_EVENT_CLICKED, nullptr);

  wifi_toggle_label = lv_label_create(wifi_toggle_btn);
  lv_label_set_text(wifi_toggle_label, "WiFi Off");
  lv_obj_center(wifi_toggle_label);
}

void ui_set_data(const UiData &data) {
  char buf[32];

  format_time_minutes(data.time_minutes, buf, sizeof(buf));
  set_label_text(time_value_label, buf);

  if (wifi_toggle_label) {
    if (data.wifi_enabled == 1) {
      set_label_text(wifi_toggle_label, "WiFi Off");
    } else if (data.wifi_enabled == 0) {
      set_label_text(wifi_toggle_label, "WiFi On");
    } else {
      set_label_text(wifi_toggle_label, "WiFi");
    }
  }

  if (data.wifi_connected == 1) {
    draw_wifi_icon(wifi_icon_canvas, lv_color_hex(0x6CEB6C), true);
  } else if (data.wifi_connected == 2) {
    draw_wifi_icon(wifi_icon_canvas, lv_color_hex(0x1D34FF), true);
  } else if (data.wifi_connected == 0) {
    draw_wifi_icon(wifi_icon_canvas, lv_color_hex(0xFF4040), false);
  } else {
    draw_wifi_icon(wifi_icon_canvas, lv_color_hex(0x6C6C6C), false);
  }

  if (data.distance_km < 0) {
    set_label_text(distance_value_label, "-- km");
  } else {
    snprintf(buf, sizeof(buf), "%d km", data.distance_km);
    set_label_text(distance_value_label, buf);
  }

  if (data.ev_distance_km < 0) {
    set_label_text(ev_distance_value_label, "-- km");
  } else {
    snprintf(buf, sizeof(buf), "%d km", data.ev_distance_km);
    set_label_text(ev_distance_value_label, buf);
  }

  if (data.fuel_percent < 0) {
    set_label_text(fuel_value_label, "--%");
    lv_obj_set_style_text_color(fuel_value_label, lv_color_white(), 0);
  } else {
    snprintf(buf, sizeof(buf), "%d%%", data.fuel_percent);
    set_label_text(fuel_value_label, buf);
    lv_obj_set_style_text_color(fuel_value_label, color_for_fuel(data.fuel_percent), 0);
  }

  if (data.battery_percent < 0) {
    set_label_text(battery_percent_label, "--%");
    lv_obj_set_style_text_color(battery_percent_label, lv_color_white(), 0);
  } else {
    snprintf(buf, sizeof(buf), "%d%%", data.battery_percent);
    set_label_text(battery_percent_label, buf);
    lv_obj_set_style_text_color(battery_percent_label, color_for_soc(data.battery_percent), 0);
  }

  if (fuel_icon_canvas && soc_icon_canvas) {
    if (data.engine_on == 1) {
      icon_fuel_color = lv_color_white();
      icon_soc_color = lv_color_hex(0x6C6C6C);
    } else if (data.engine_on == 0) {
      icon_fuel_color = lv_color_hex(0x6C6C6C);
      icon_soc_color = lv_color_white();
    } else {
      icon_fuel_color = lv_color_hex(0x6C6C6C);
      icon_soc_color = lv_color_hex(0x6C6C6C);
    }
    draw_fuel_icon(fuel_icon_canvas, icon_fuel_color);
    draw_soc_icon(soc_icon_canvas, icon_soc_color);
  }

  if (service_distance_value_label) {
    if (data.service_distance_km < 0) {
      set_label_text(service_distance_value_label, "-- km");
    } else {
      snprintf(buf, sizeof(buf), "%d km", data.service_distance_km);
      set_label_text(service_distance_value_label, buf);
    }
  }

  if (service_ev_distance_value_label) {
    if (data.service_ev_distance_km < 0) {
      set_label_text(service_ev_distance_value_label, "-- km");
    } else {
      snprintf(buf, sizeof(buf), "%d km", data.service_ev_distance_km);
      set_label_text(service_ev_distance_value_label, buf);
    }
  }
}

void ui_set_reset_distance_cb(void (*cb)()) {
  reset_distance_cb = cb;
}

void ui_set_wifi_reconnect_cb(void (*cb)()) {
  wifi_reconnect_cb = cb;
}

void ui_set_wifi_toggle_cb(void (*cb)()) {
  wifi_toggle_cb = cb;
}

void updateBatteryLevel(int raw) {
  UiData data{};
  data.battery_percent = raw;
  data.speed_kmh = -1;
  data.wifi_enabled = -1;
  data.fuel_percent = -1;
  data.distance_km = -1;
  data.ev_distance_km = -1;
  data.service_distance_km = -1;
  data.service_ev_distance_km = -1;
  data.engine_on = -1;
  data.time_minutes = -1;
  data.wifi_connected = -1;
  ui_set_data(data);
}
