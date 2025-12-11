#include "wifi_service.h"
#include <WiFi.h>
#include "globals.h"

const char *ssid = "V-LINK";
const char *password = "";

void initWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  // WiFi.setSleep(false);
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