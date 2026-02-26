#include "elm327_service.h"
#include <vector>
#include <WiFi.h>
#include "globals.h"

static const char *ELM327Socket = "192.168.0.10";
static const uint16_t ELM327Port = 35000;
static const uint16_t ELM327Timeout = 3000;

static bool ensureElmConnected(WiFiClient &client) {
  if (WiFi.status() != WL_CONNECTED) {
    USBSerial.println("WiFi not connected");
    return false;
  }

  if (client.connected()) return true;

  if (!client.connect(ELM327Socket, ELM327Port, ELM327Timeout)) {
    USBSerial.println("ELM327 not connected");
    return false;
  }

  client.print("ATZ\r");  delay(800);
  client.print("ATE0\r"); delay(200);
  client.print("ATL0\r"); delay(200);
  client.print("ATH0\r"); delay(200);
  client.print("ATSP6\r"); delay(500);

  // подчистим хвосты после init
  unsigned long t = millis();
  while (millis() - t < 300) {
    while (client.available()) client.read();
    delay(10);
  }

  return true;
}

static bool readObdLineWithPrefix(WiFiClient &client, const char *prefix, String &outLine, uint32_t waitMs = 1000) {
  unsigned long start = millis();
  while (millis() - start < waitMs) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;

      USBSerial.println(line);

      if (line == ">") continue;

      int p = line.indexOf(prefix);
      if (p >= 0) {
        outLine = line.substring(p);
        outLine.trim();
        return true;
      }
    }
  }
  return false;
}

static bool splitTokens(const String &line, std::vector<String> &parts) {
  parts.clear();

  char buf[96];
  line.toCharArray(buf, sizeof(buf));

  char *token = strtok(buf, " ");
  while (token) {
    parts.push_back(String(token));
    token = strtok(nullptr, " ");
  }

  return !parts.empty();
}

// ===== SOC (PID 01 5B) =====
// Возвращает raw байт A (0..255) как раньше
bool readSocRaw(WiFiClient &client, uint8_t &soc) {
  soc = 0;
  if (!ensureElmConnected(client)) return false;

  while (client.available()) client.read();

  client.print("01 5B\r");
  USBSerial.println("Battery request: <01 5B>");

  String line;
  if (!readObdLineWithPrefix(client, "41 5B", line, 1000)) {
    USBSerial.println("Timeout waiting for SOC response");
    return false;
  }

  std::vector<String> parts;
  if (!splitTokens(line, parts)) return false;

  if (parts.size() >= 3 && parts[0] == "41" && parts[1] == "5B") {
    long a = strtol(parts[2].c_str(), nullptr, 16);
    if (a < 0 || a > 255) return false;

    soc = (uint8_t)a;

    USBSerial.print("SOC parsed (raw A): ");
    USBSerial.println(soc);
    return true;
  }

  USBSerial.println("SOC response format mismatch");
  return false;
}

// ===== RPM (PID 01 0C) =====
bool readEngineRpm(WiFiClient &client, uint16_t &rpm) {
  rpm = 0;
  if (!ensureElmConnected(client)) return false;

  while (client.available()) client.read();

  client.print("01 0C\r");
  USBSerial.println("RPM request: <01 0C>");

  String line;
  if (!readObdLineWithPrefix(client, "41 0C", line, 1000)) {
    USBSerial.println("Timeout waiting for RPM response");
    return false;
  }

  std::vector<String> parts;
  if (!splitTokens(line, parts)) return false;

  if (parts.size() >= 4 && parts[0] == "41" && parts[1] == "0C") {
    long a = strtol(parts[2].c_str(), nullptr, 16);
    long b = strtol(parts[3].c_str(), nullptr, 16);
    if (a < 0 || a > 255 || b < 0 || b > 255) return false;

    rpm = (uint16_t)(((a * 256L) + b) / 4L);

    USBSerial.print("RPM parsed: ");
    USBSerial.println(rpm);
    return true;
  }

  USBSerial.println("RPM response format mismatch");
  return false;
}

// ===== Speed (PID 01 0D) =====
// A = km/h
bool readVehicleSpeed(WiFiClient &client, uint8_t &speedKmh) {
  speedKmh = 0;
  if (!ensureElmConnected(client)) return false;

  while (client.available()) client.read();

  client.print("01 0D\r");
  USBSerial.println("Speed request: <01 0D>");

  String line;
  if (!readObdLineWithPrefix(client, "41 0D", line, 1000)) {
    USBSerial.println("Timeout waiting for Speed response");
    return false;
  }

  std::vector<String> parts;
  if (!splitTokens(line, parts)) return false;

  if (parts.size() >= 3 && parts[0] == "41" && parts[1] == "0D") {
    long a = strtol(parts[2].c_str(), nullptr, 16);
    if (a < 0 || a > 255) return false;

    speedKmh = (uint8_t)a;

    USBSerial.print("Speed parsed (km/h): ");
    USBSerial.println(speedKmh);
    return true;
  }

  USBSerial.println("Speed response format mismatch");
  return false;
}

// ===== Engine Load (PID 01 04) =====
// % = A*100/255
bool readEngineLoad(WiFiClient &client, uint8_t &loadPercent) {
  loadPercent = 0;
  if (!ensureElmConnected(client)) return false;

  while (client.available()) client.read();

  client.print("01 04\r");
  USBSerial.println("Engine Load request: <01 04>");

  String line;
  if (!readObdLineWithPrefix(client, "41 04", line, 1000)) {
    USBSerial.println("Timeout waiting for Load response");
    return false;
  }

  std::vector<String> parts;
  if (!splitTokens(line, parts)) return false;

  if (parts.size() >= 3 && parts[0] == "41" && parts[1] == "04") {
    long a = strtol(parts[2].c_str(), nullptr, 16);
    if (a < 0 || a > 255) return false;

    loadPercent = (uint8_t)((a * 100UL) / 255UL);

    USBSerial.print("Engine Load parsed (%): ");
    USBSerial.println(loadPercent);
    return true;
  }

  USBSerial.println("Engine Load response format mismatch");
  return false;
}

// ===== Fuel Level (PID 01 2F) =====
// % = A*100/255
bool readFuelLevel(WiFiClient &client, uint8_t &fuelPercent) {
  fuelPercent = 0;
  if (!ensureElmConnected(client)) return false;

  while (client.available()) client.read();

  client.print("01 2F\r");
  USBSerial.println("Fuel Level request: <01 2F>");

  String line;
  if (!readObdLineWithPrefix(client, "41 2F", line, 1000)) {
    USBSerial.println("Timeout waiting for Fuel Level response");
    return false;
  }

  std::vector<String> parts;
  if (!splitTokens(line, parts)) return false;

  if (parts.size() >= 3 && parts[0] == "41" && parts[1] == "2F") {
    long a = strtol(parts[2].c_str(), nullptr, 16);
    if (a < 0 || a > 255) return false;

    fuelPercent = (uint8_t)((a * 100UL) / 255UL);

    USBSerial.print("Fuel Level parsed (%): ");
    USBSerial.println(fuelPercent);
    return true;
  }

  USBSerial.println("Fuel Level response format mismatch");
  return false;
}
