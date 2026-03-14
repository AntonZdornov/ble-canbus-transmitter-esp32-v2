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
static float ev_km_acc = 0.0f;
static float service_ev_km_acc = 0.0f;
static uint32_t lastDistanceUpdateMs = 0;
static uint32_t lastDistancePersistMs = 0;
static uint32_t lastObdPollMs = 0;
static uint32_t lastUiUpdateMs = 0;
static uint32_t lastFuelQueryMs = 0;
static const bool ENABLE_FUEL_POLL = false;

static int cached_battery_percent = -1;
static int cached_speed_kmh = -1;
static int cached_fuel_percent = -1;
static int cached_engine_on = -1;
static uint32_t cached_battery_ms = 0;
static uint32_t cached_speed_ms = 0;
static uint32_t cached_fuel_ms = 0;
static uint32_t cached_engine_ms = 0;

enum ObdQueryKind : uint8_t {
  OBD_QUERY_SOC = 0,
  OBD_QUERY_RPM = 1,
  OBD_QUERY_SPEED = 2,
  OBD_QUERY_FUEL = 3,
  OBD_QUERY_COUNT = 4
};
static ObdQueryKind nextObdQuery = OBD_QUERY_SOC;

static const uint32_t OBD_QUERY_INTERVAL_MS = 400;
static const uint32_t OBD_QUERY_TIMEOUT_MS = 300;
static const uint32_t OBD_QUERY_TIMEOUT_FUEL_MS = 120;
static const uint32_t OBD_FUEL_QUERY_INTERVAL_MS = 2500;
static const uint32_t UI_UPDATE_INTERVAL_MS = 100;
static const uint32_t TTL_FAST_MS = 2000;
static const uint32_t TTL_ENGINE_MS = 10000;
static const uint32_t TTL_BATTERY_MS = 30000;
static const uint32_t TTL_SLOW_MS = 10000;

static int valueWithTtl(int value, uint32_t lastSuccessMs, uint32_t ttlMs, uint32_t now) {
  if (lastSuccessMs == 0 || now - lastSuccessMs > ttlMs) return -1;
  return value;
}

static void resetDistance() {
  service_km_acc = 0.0f;
  service_ev_km_acc = 0.0f;
  lastDistanceUpdateMs = millis();
  prefs.putFloat("service_km", service_km_acc);
  prefs.putFloat("service_ev_km", service_ev_km_acc);
  lastDistancePersistMs = lastDistanceUpdateMs;
}

static void onWifiIconPressed() {
  requestWifiReconnect();
}

void setup() {
  USBSerial.begin(115200); /* prepare for possible serial debug */
  prefs.begin("trip", false);
  service_km_acc = prefs.getFloat("service_km", 0.0f);
  service_ev_km_acc = prefs.getFloat("service_ev_km", 0.0f);
  lastDistanceUpdateMs = millis();
  initTouth();
  initLvgl();
  ui_init();
  ui_set_reset_distance_cb(resetDistance);
  ui_set_wifi_reconnect_cb(onWifiIconPressed);
  delay(5);
  initWifi();
}

void loop() {
  lv_timer_handler(); /* let the GUI do its work */
  wifiServiceTick();

  uint32_t now = millis();

  if (now - lastObdPollMs >= OBD_QUERY_INTERVAL_MS) {
    switch (nextObdQuery) {
      case OBD_QUERY_SOC: {
        uint8_t soc_raw = 0;
        if (readSocRaw(client, soc_raw, OBD_QUERY_TIMEOUT_MS)) {
          cached_battery_percent = convertBatteryData(soc_raw);
          cached_battery_ms = now;
        }
        break;
      }
      case OBD_QUERY_RPM: {
        uint16_t rpm = 0;
        if (readEngineRpm(client, rpm, OBD_QUERY_TIMEOUT_MS)) {
          cached_engine_on = (rpm > 0) ? 1 : 0;
          cached_engine_ms = now;
        }
        break;
      }
      case OBD_QUERY_SPEED: {
        uint8_t speed_kmh = 0;
        if (readVehicleSpeed(client, speed_kmh, OBD_QUERY_TIMEOUT_MS)) {
          cached_speed_kmh = speed_kmh;
          cached_speed_ms = now;
        }
        break;
      }
      case OBD_QUERY_FUEL: {
        if (!ENABLE_FUEL_POLL) {
          break;
        }
        if (now - lastFuelQueryMs < OBD_FUEL_QUERY_INTERVAL_MS) {
          break;
        }
        lastFuelQueryMs = now;
        uint8_t fuel_percent = 0;
        if (readFuelLevel(client, fuel_percent, OBD_QUERY_TIMEOUT_FUEL_MS)) {
          cached_fuel_percent = fuel_percent;
          cached_fuel_ms = now;
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
    int speed_for_ui = valueWithTtl(cached_speed_kmh, cached_speed_ms, TTL_FAST_MS, now);
    int engine_for_ui = valueWithTtl(cached_engine_on, cached_engine_ms, TTL_ENGINE_MS, now);
    int battery_for_ui = valueWithTtl(cached_battery_percent, cached_battery_ms, TTL_BATTERY_MS, now);
    int fuel_for_ui = valueWithTtl(cached_fuel_percent, cached_fuel_ms, TTL_SLOW_MS, now);

    if (lastDistanceUpdateMs != 0 && speed_for_ui >= 0) {
      float hours = (now - lastDistanceUpdateMs) / 3600000.0f;
      float delta = (float)speed_for_ui * hours;
      trip_km_acc += delta;
      service_km_acc += delta;
      if (engine_for_ui == 0) {
        ev_km_acc += delta;
        service_ev_km_acc += delta;
      }
    }
    lastDistanceUpdateMs = now;

    UiData data{};
    data.speed_kmh = speed_for_ui;
    data.wifi_connected = isWifiManualReconnectInProgress()
                              ? 2
                              : ((WiFi.status() == WL_CONNECTED) ? 1 : 0);
    data.fuel_percent = fuel_for_ui;
    data.distance_km = (int)(trip_km_acc + 0.5f);
    data.ev_distance_km = (int)(ev_km_acc + 0.5f);
    data.service_distance_km = (int)(service_km_acc + 0.5f);
    data.service_ev_distance_km = (int)(service_ev_km_acc + 0.5f);
    data.engine_on = engine_for_ui;
    data.time_minutes = (int)(now / 60000UL);
    data.battery_percent = battery_for_ui;

    if (lastDistancePersistMs == 0 || now - lastDistancePersistMs > 10000) {
      prefs.putFloat("service_km", service_km_acc);
      prefs.putFloat("service_ev_km", service_ev_km_acc);
      lastDistancePersistMs = now;
    }

    ui_set_data(data);
    lastUiUpdateMs = now;
  }

  delay(5);
}
