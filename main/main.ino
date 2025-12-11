#include "globals.h"
#include <Arduino.h>
#include <Wire.h>
#include "ui.h"
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "touth_driver.h"
#include "lvgl_driver.h"
#include "wifi_service.h"


void setup() {
  USBSerial.begin(115200); /* prepare for possible serial debug */
  initTouth();
  initLvgl();
  initUI();
  initWifi();
}

void loop() {
  lv_timer_handler(); /* let the GUI do its work */

  static uint32_t lastQuery = 0;
  if (millis() - lastQuery > 1000) {
    uint8_t soc = 0;
    if (readSocRaw(client, soc)) {
      updateBatteryLevel(soc);
    }
    lastQuery = millis();
  }

  delay(5);
}
