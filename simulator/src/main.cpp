#include <lvgl.h>
#include <SDL2/SDL.h>
#include <stdint.h>
#include "sdl/sdl.h"
#include "ui.h"
#include "ui_model.h"

static void update_mock_data(UiData &data) {
  static int battery = 100;
  static int dir = -1;
  static int minutes = 0;
  static int distance = 0;

  battery += dir;
  if (battery <= 5) {
    battery = 5;
    dir = 1;
  } else if (battery >= 100) {
    battery = 100;
    dir = -1;
  }

  minutes += 1;
  distance += 1;

  data.battery_percent = battery;
  data.time_minutes = minutes;
  data.distance_km = distance;
}

int main() {
  lv_init();
  sdl_init();

  static lv_color_t buf1[LV_HOR_RES_MAX * 40];
  static lv_disp_draw_buf_t draw_buf;
  lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, LV_HOR_RES_MAX * 40);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.draw_buf = &draw_buf;
  disp_drv.flush_cb = sdl_display_flush;
  disp_drv.hor_res = LV_HOR_RES_MAX;
  disp_drv.ver_res = LV_VER_RES_MAX;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = sdl_mouse_read;
  lv_indev_drv_register(&indev_drv);

  ui_init();

  UiData data{};
  data.battery_percent = 100;
  data.distance_km = 0;
  data.time_minutes = 0;
  ui_set_data(data);

  uint32_t last_tick = SDL_GetTicks();
  uint32_t last_update = last_tick;

  while (true) {
    uint32_t now = SDL_GetTicks();
    uint32_t diff = now - last_tick;
    if (diff > 0) {
      lv_tick_inc(diff);
      last_tick = now;
    }

    if (now - last_update >= 500) {
      update_mock_data(data);
      ui_set_data(data);
      last_update = now;
    }

    lv_timer_handler();
    SDL_Delay(5);
  }

  return 0;
}
