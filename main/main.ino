#include "globals.h"
#include <Arduino.h>
#include <Wire.h>
#include "ui.h"
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "touth_driver.h"
#include "lvgl_driver.h"
#include "elm327_service.h"
#include <WiFi.h>
#include "wifi_service.h"
#include "globals.h"
#include "utils.h"

// WIFISettings
WiFiClient client;

void setup() {
  USBSerial.begin(115200); /* prepare for possible serial debug */
  initTouth();
  initLvgl();
  ui_init();
  delay(5);
  initWifi();
}

void loop() {
  lv_timer_handler(); /* let the GUI do its work */

  static uint32_t lastQuery = 0;
  if (millis() - lastQuery > 1000) {
    uint8_t soc_raw = 0;
    UiData data{};
    data.rpm = -1;
    data.time_minutes = -1;
    data.battery_percent = -1;

    if (readSocRaw(client, soc_raw)) {
      data.battery_percent = convertBatteryData(soc_raw);
    }

    uint16_t rpm;
    if (readEngineRpm(client, rpm)) {
      data.rpm = rpm;
    }

    ui_set_data(data);
    lastQuery = millis();
  }

  delay(5);
}
