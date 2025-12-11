#include <lvgl.h>
#include "pin_config.h"
#include "lv_conf.h"
#include "globals.h"
#include "Arduino_GFX_Library.h"
#include <Arduino.h>
#include <Wire.h>
#include "touth_driver.h"

static const uint16_t screenWidth = LCD_WIDTH;
static const uint16_t screenHeight = LCD_HEIGHT;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

#define Frequency 1000
#define Resolution 10

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

void backlight_Init(void) {
  ledcAttach(LCD_BL, Frequency, Resolution);
  ledcWrite(LCD_BL, 0);
}

void setBacklight(uint8_t light) {

  if (light > 100 || light < 0)
    printf("Set Backlight parameters in the range of 0 to 100 \r\n");
  else {
    uint32_t backlight = light * 10;
    ledcWrite(LCD_BL, backlight);
  }
}

void initLvgl() {
  /* Initialize the LVGL */
  gfx->begin();

  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);
  backlight_Init();
  setBacklight(50);

  String LVGL_Arduino = "Arduino: " + String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  USBSerial.println(LVGL_Arduino);
  USBSerial.println("LVGL_Arduino");

  /* Initialize the display */
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);

  /* Change the following line to your display resolution */
  disp_drv.hor_res = LCD_WIDTH;
  disp_drv.ver_res = LCD_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /* Initialize the (dummy) input device driver */
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touch_read;
  lv_indev_drv_register(&indev_drv);

}