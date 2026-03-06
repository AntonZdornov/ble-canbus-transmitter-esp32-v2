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
#include <Preferences.h>
#include "wifi_service.h"
#include "globals.h"
#include "utils.h"

// WIFISettings
WiFiClient client;
Preferences prefs;
static float trip_km_acc = 0.0f;
static float service_km_acc = 0.0f;
static uint32_t lastDistanceUpdateMs = 0;
static uint32_t lastDistancePersistMs = 0;
static uint32_t lastObdPollMs = 0;
static uint32_t lastUiUpdateMs = 0;

static int cached_battery_percent = -1;
static int cached_speed_kmh = -1;
static int cached_fuel_percent = -1;
static int cached_engine_on = -1;

enum ObdQueryKind : uint8_t {
  OBD_QUERY_SOC = 0,
  OBD_QUERY_RPM = 1,
  OBD_QUERY_SPEED = 2,
  OBD_QUERY_FUEL = 3,
  OBD_QUERY_COUNT = 4
};
static ObdQueryKind nextObdQuery = OBD_QUERY_SOC;

static const uint32_t OBD_QUERY_INTERVAL_MS = 80;
static const uint32_t OBD_QUERY_TIMEOUT_MS = 60;
static const uint32_t UI_UPDATE_INTERVAL_MS = 100;

static void resetDistance() {
  service_km_acc = 0.0f;
  lastDistanceUpdateMs = millis();
  prefs.putFloat("service_km", service_km_acc);
}

void setup() {
  USBSerial.begin(115200); /* prepare for possible serial debug */
  prefs.begin("trip", false);
  service_km_acc = prefs.getFloat("service_km", 0.0f);
  lastDistanceUpdateMs = millis();
  initTouth();
  initLvgl();
  ui_init();
  ui_set_reset_distance_cb(resetDistance);
  delay(5);
  initWifi();
}

void loop() {
  lv_timer_handler(); /* let the GUI do its work */

  uint32_t now = millis();

  if (now - lastObdPollMs >= OBD_QUERY_INTERVAL_MS) {
    switch (nextObdQuery) {
      case OBD_QUERY_SOC: {
        uint8_t soc_raw = 0;
        if (readSocRaw(client, soc_raw, OBD_QUERY_TIMEOUT_MS)) {
          cached_battery_percent = convertBatteryData(soc_raw);
        }
        break;
      }
      case OBD_QUERY_RPM: {
        uint16_t rpm = 0;
        if (readEngineRpm(client, rpm, OBD_QUERY_TIMEOUT_MS)) {
          cached_engine_on = (rpm > 0) ? 1 : 0;
        }
        break;
      }
      case OBD_QUERY_SPEED: {
        uint8_t speed_kmh = 0;
        if (readVehicleSpeed(client, speed_kmh, OBD_QUERY_TIMEOUT_MS)) {
          cached_speed_kmh = speed_kmh;
        }
        break;
      }
      case OBD_QUERY_FUEL: {
        uint8_t fuel_percent = 0;
        if (readFuelLevel(client, fuel_percent, OBD_QUERY_TIMEOUT_MS)) {
          cached_fuel_percent = fuel_percent;
        }
        break;
      }
      default:
        break;
    }

    nextObdQuery = (ObdQueryKind)((nextObdQuery + 1) % OBD_QUERY_COUNT);
    lastObdPollMs = now;
  }

  if (now - lastUiUpdateMs >= UI_UPDATE_INTERVAL_MS) {
    if (lastDistanceUpdateMs != 0 && cached_speed_kmh >= 0) {
      float hours = (now - lastDistanceUpdateMs) / 3600000.0f;
      float delta = (float)cached_speed_kmh * hours;
      trip_km_acc += delta;
      service_km_acc += delta;
    }
    lastDistanceUpdateMs = now;

    UiData data{};
    data.speed_kmh = cached_speed_kmh;
    data.fuel_percent = cached_fuel_percent;
    data.distance_km = (int)(trip_km_acc + 0.5f);
    data.service_distance_km = (int)(service_km_acc + 0.5f);
    data.engine_on = cached_engine_on;
    data.time_minutes = (int)(now / 60000UL);
    data.battery_percent = cached_battery_percent;

    if (lastDistancePersistMs == 0 || now - lastDistancePersistMs > 10000) {
      prefs.putFloat("service_km", service_km_acc);
      lastDistancePersistMs = now;
    }

    ui_set_data(data);
    lastUiUpdateMs = now;
  }

  delay(5);
}
