#include <lvgl.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "lv_conf.h"
#include <Arduino.h>
#include <Wire.h>
#include "HWCDC.h"

HWCDC USBSerial;

/*Change to your screen resolution*/
static const uint16_t screenWidth = LCD_WIDTH;
static const uint16_t screenHeight = LCD_HEIGHT;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

lv_obj_t *label;  // Global label object

Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST /* RST */,
                                      0 /* rotation */, true /* IPS */, LCD_WIDTH, LCD_HEIGHT, 0, 20, 0, 0);

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

void setup() {
  USBSerial.begin(115200); /* prepare for possible serial debug */
  gfx->begin();
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  USBSerial.println(LVGL_Arduino);
  USBSerial.println("I am LVGL_Arduino");

  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);


  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = LCD_WIDTH;
  disp_drv.ver_res = LCD_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  lv_indev_drv_register(&indev_drv);

  lv_obj_t *label = lv_label_create(lv_scr_act());
  // Задаем черный фон
  lv_obj_set_style_bg_color(label, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(label, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_label_set_text(label, "Hello World,!");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);


  USBSerial.println("Setup done");
}

void loop() {
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}
