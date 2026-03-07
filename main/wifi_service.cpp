#include "wifi_service.h"
#include <WiFi.h>
#include "globals.h"

const char *ssid = "V-LINK";
const char *password = "";
static bool manual_reconnect_in_progress = false;
static uint32_t manual_reconnect_start_ms = 0;
static const uint32_t manual_reconnect_timeout_ms = 15000;

void initWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  USBSerial.println("Scanning WiFi...");
  int networks = WiFi.scanNetworks();
  if (networks >= 0) {
    USBSerial.print("WiFi networks found: ");
    USBSerial.println(networks);
    for (int i = 0; i < networks; ++i) {
      USBSerial.print("  ");
      USBSerial.print(i + 1);
      USBSerial.print(". ");
      USBSerial.print(WiFi.SSID(i));
      USBSerial.print(" | RSSI ");
      USBSerial.print(WiFi.RSSI(i));
      USBSerial.print(" dBm | CH ");
      USBSerial.print(WiFi.channel(i));
      USBSerial.print(" | ENC ");
      USBSerial.println((int)WiFi.encryptionType(i));
    }
  } else {
    USBSerial.print("WiFi scan failed, status: ");
    USBSerial.println(networks);
  }
  WiFi.scanDelete();

  WiFi.begin(ssid, password);
  bool animation = true;

  USBSerial.println("wifi");

  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 30000; // 30 секунд
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
    delay(500);
    USBSerial.println(".");

    animation = !animation;
  }

  if (WiFi.status() != WL_CONNECTED) {
    USBSerial.println("Wifi not Connected");
    delay(2000);
    return;
  }

  USBSerial.println("Connected to WiFi");
  USBSerial.println("The WiFi initialized successfully.");
  delay(500);
}

void requestWifiReconnect() {
  USBSerial.println("Manual WiFi reconnect requested");
  WiFi.disconnect(false, false);
  delay(20);
  WiFi.begin(ssid, password);
  manual_reconnect_start_ms = millis();
  manual_reconnect_in_progress = true;
}

void wifiServiceTick() {
  if (!manual_reconnect_in_progress) return;

  if (WiFi.status() == WL_CONNECTED) {
    manual_reconnect_in_progress = false;
    USBSerial.println("Manual WiFi reconnect: connected");
    return;
  }

  if (millis() - manual_reconnect_start_ms > manual_reconnect_timeout_ms) {
    manual_reconnect_in_progress = false;
    USBSerial.println("Manual WiFi reconnect: timeout");
  }
}

bool isWifiManualReconnectInProgress() {
  return manual_reconnect_in_progress;
}
