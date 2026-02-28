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
  static uint32_t lastDistanceUpdateMs = 0;
  static float distance_km_acc = 0.0f;
  if (millis() - lastQuery > 1000) {
    uint8_t soc_raw = 0;
    UiData data{};
    data.speed_kmh = -1;
    data.fuel_percent = -1;
    data.distance_km = -1;
    data.engine_on = -1;
    data.time_minutes = -1;
    data.battery_percent = -1;

    if (readSocRaw(client, soc_raw)) {
      data.battery_percent = convertBatteryData(soc_raw);
    }

    uint16_t rpm;
    if (readEngineRpm(client, rpm)) {
      data.engine_on = (rpm > 0) ? 1 : 0;
    }

    uint8_t speed_kmh;
    if (readVehicleSpeed(client, speed_kmh)) {
      data.speed_kmh = speed_kmh;
    }

    uint8_t fuel_percent;
    if (readFuelLevel(client, fuel_percent)) {
      data.fuel_percent = fuel_percent;
    }

    // Integrate distance from speed (km/h) over time.
    if (data.speed_kmh >= 0) {
      uint32_t now = millis();
      if (lastDistanceUpdateMs != 0) {
        float hours = (now - lastDistanceUpdateMs) / 3600000.0f;
        distance_km_acc += (float)data.speed_kmh * hours;
      }
      lastDistanceUpdateMs = now;
      data.distance_km = (int)(distance_km_acc + 0.5f);
    }

    ui_set_data(data);
    lastQuery = millis();
  }

  delay(5);
}
