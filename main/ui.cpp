#include <lvgl.h>
#include "pin_config.h"
#include "lv_conf.h"
#include <Arduino.h>
#include <Wire.h>
#include "globals.h"
#include "ui.h"
#include "Arduino_DriveBus_Library.h"
#include "battery_widget.h"
#include "utils.h"

//ui Components
lv_obj_t *battery_label;

extern const lv_font_t lv_font_montserrat_48;
extern const lv_font_t lv_font_montserrat_38;
extern const lv_font_t lv_font_montserrat_12;
extern const lv_font_t lv_font_montserrat_22;

void battery_touch_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    static int16_t start_y = 0;
    static uint8_t start_level = 0;

    if (code == LV_EVENT_PRESSED)
    {
        USBSerial.println("LV_EVENT_PRESSED");
        // запоминаем точку старта
        lv_indev_t *indev = lv_indev_get_act();
        lv_point_t p;
        lv_indev_get_point(indev, &p);

        start_y = p.y;
    }

    else if (code == LV_EVENT_PRESSING)
    {
        USBSerial.println("LV_EVENT_PRESSING");
        // читаем текущую позицию пальца
        lv_indev_t *indev = lv_indev_get_act();
        lv_point_t p;
        lv_indev_get_point(indev, &p);

        int16_t dy = start_y - p.y;           // вверх = +, вниз = –
        int new_level = start_level + dy / 2; // чувствительность свайпа

        if (new_level < 0)
            new_level = 0;
        if (new_level > 100)
            new_level = 100;

        // for test
        battery_set_level(new_level);

        char buf[16];
        snprintf(buf, sizeof(buf), "Battery: %d%%", new_level);
        lv_label_set_text(battery_label, buf);
    }

    else if (code == LV_EVENT_RELEASED)
    {
        USBSerial.println("LV_EVENT_RELEASED");
        // можно сделать анимацию завершения, если хочешь
    }
}

void initUI() {
  /* Create UI */ 
  lv_obj_clean(lv_scr_act());
  lv_obj_t *root_container = lv_obj_create(lv_scr_act());
  lv_obj_set_size(root_container, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_pad_all(root_container, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(root_container, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(root_container, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(root_container, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_layout(root_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(root_container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(root_container,
                        LV_FLEX_ALIGN_CENTER,         // горизонтально по центру
                        LV_FLEX_ALIGN_SPACE_BETWEEN,  // вертикально вниз
                        LV_FLEX_ALIGN_CENTER);

  lv_obj_add_flag(root_container, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(root_container, battery_touch_cb,
                      LV_EVENT_ALL,
                      NULL);


  /* Battery */
  lv_obj_t *battery_container = lv_obj_create(root_container);
  lv_obj_set_height(battery_container, LV_PCT(20));  // или LV_SIZE_CONTENT + grow
  lv_obj_set_width(battery_container, LV_PCT(100));  // если нужно
  lv_obj_set_style_pad_all(battery_container, 0, LV_PART_MAIN);
  lv_obj_set_flex_grow(battery_container, 1);

  lv_obj_set_layout(battery_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(battery_container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(battery_container,
                        LV_FLEX_ALIGN_CENTER,  // по горизонтали
                        LV_FLEX_ALIGN_CENTER,  // по вертикали
                        LV_FLEX_ALIGN_CENTER);

  lv_obj_set_style_bg_color(battery_container, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(battery_container, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_pad_all(battery_container, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(battery_container, 0, LV_PART_MAIN);

  lv_obj_add_flag(battery_container, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(battery_container, battery_touch_cb,
                      LV_EVENT_ALL,
                      NULL);

  battery_label = lv_label_create(battery_container);
  lv_label_set_text(battery_label, "Battery: 0%");
  lv_obj_set_style_text_align(battery_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(battery_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_22, 0);
  lv_obj_align(battery_label, LV_ALIGN_CENTER, 0, 0);

  battery_widget_create(battery_container);
}

void updateBatteryLevel(int soc) {
  uint8_t normalized = convertBatteryData(soc);
  char buf[16];
  snprintf(buf, sizeof(buf), "Battery: %d%%", normalized);
  battery_set_level(normalized);
  lv_label_set_text(battery_label, buf);
}