#include "elm327_service.h"
#include <vector>
#include <WiFi.h>
#include "globals.h"

static const char *ELM327Socket = "192.168.0.10";
static const uint16_t ELM327Port = 35000;
static const uint16_t ELM327Timeout = 3000;
static const uint32_t ELM327LineBufferSize = 96;
static const uint32_t PidLogIntervalMs = 2000;
static const uint32_t PidSummaryIntervalMs = 5000;

enum class PidOutcome : uint8_t {
  Ok = 0,
  Timeout = 1,
  NoData = 2,
  Format = 3,
};

struct PidStats {
  const char *name;
  uint32_t ok;
  uint32_t timeout;
  uint32_t no_data;
  uint32_t format;
  uint32_t last_log_ms;
};

static PidStats g_soc_stats = {"SOC", 0, 0, 0, 0, 0};
static PidStats g_rpm_stats = {"RPM", 0, 0, 0, 0, 0};
static PidStats g_speed_stats = {"SPD", 0, 0, 0, 0, 0};
static PidStats g_fuel_stats = {"FUEL", 0, 0, 0, 0, 0};
static uint32_t g_last_summary_ms = 0;

static uint32_t pidErrorCount(const PidStats &s) {
  return s.timeout + s.no_data + s.format;
}

static void maybeLogPidSummary() {
  uint32_t now = millis();
  if (now - g_last_summary_ms < PidSummaryIntervalMs) return;
  g_last_summary_ms = now;

  USBSerial.printf(
      "ELM summary | SOC %lu/%lu RPM %lu/%lu SPD %lu/%lu FUEL %lu/%lu\n",
      (unsigned long)g_soc_stats.ok, (unsigned long)pidErrorCount(g_soc_stats),
      (unsigned long)g_rpm_stats.ok, (unsigned long)pidErrorCount(g_rpm_stats),
      (unsigned long)g_speed_stats.ok, (unsigned long)pidErrorCount(g_speed_stats),
      (unsigned long)g_fuel_stats.ok, (unsigned long)pidErrorCount(g_fuel_stats));
}

static void reportPidOutcome(PidStats &stats, PidOutcome outcome) {
  const char *label = "unknown";
  switch (outcome) {
    case PidOutcome::Ok:
      stats.ok++;
      label = "ok";
      break;
    case PidOutcome::Timeout:
      stats.timeout++;
      label = "timeout";
      break;
    case PidOutcome::NoData:
      stats.no_data++;
      label = "no_data";
      break;
    case PidOutcome::Format:
      stats.format++;
      label = "format";
      break;
  }

  uint32_t now = millis();
  if (now - stats.last_log_ms >= PidLogIntervalMs) {
    USBSerial.printf("ELM %s: %s\n", stats.name, label);
    stats.last_log_ms = now;
  }
  maybeLogPidSummary();
}

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

  client.setTimeout(80);
  client.print("ATZ\r");  delay(120);
  client.print("ATE0\r"); delay(60);
  client.print("ATL0\r"); delay(60);
  client.print("ATH0\r"); delay(60);
  client.print("ATSP6\r"); delay(120);

  // подчистим хвосты после init
  unsigned long t = millis();
  while (millis() - t < 120) {
    while (client.available()) client.read();
    delay(10);
  }

  return true;
}

static bool readObdLineWithPrefix(WiFiClient &client, const char *prefix, String &outLine, uint32_t waitMs, bool *no_data = nullptr) {
  unsigned long start = millis();
  char lineBuf[ELM327LineBufferSize];
  size_t lineLen = 0;
  while (millis() - start < waitMs) {
    while (client.available()) {
      char ch = (char)client.read();
      if (ch == '\r') continue;

      bool lineReady = false;
      if (ch == '\n' || ch == '>') {
        lineReady = true;
      } else if (lineLen < sizeof(lineBuf) - 1) {
        lineBuf[lineLen++] = ch;
      }

      if (!lineReady) continue;

      lineBuf[lineLen] = '\0';
      String line = String(lineBuf);
      line.trim();
      lineLen = 0;
      if (line.length() == 0) continue;

      if (line == ">") continue;
      if (line == "NO DATA") {
        if (no_data) *no_data = true;
        return false;
      }

      int p = line.indexOf(prefix);
      if (p >= 0) {
        outLine = line.substring(p);
        outLine.trim();
        return true;
      }
    }
    delay(1);
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
bool readSocRaw(WiFiClient &client, uint8_t &soc, uint32_t waitMs) {
  soc = 0;
  if (!ensureElmConnected(client)) return false;

  while (client.available()) client.read();

  client.print("01 5B\r");
  USBSerial.println("Battery request: <01 5B>");

  String line;
  bool no_data = false;
  if (!readObdLineWithPrefix(client, "41 5B", line, waitMs, &no_data)) {
    reportPidOutcome(g_soc_stats, no_data ? PidOutcome::NoData : PidOutcome::Timeout);
    return false;
  }

  std::vector<String> parts;
  if (!splitTokens(line, parts)) {
    reportPidOutcome(g_soc_stats, PidOutcome::Format);
    return false;
  }

  if (parts.size() >= 3 && parts[0] == "41" && parts[1] == "5B") {
    long a = strtol(parts[2].c_str(), nullptr, 16);
    if (a < 0 || a > 255) return false;

    soc = (uint8_t)a;

    reportPidOutcome(g_soc_stats, PidOutcome::Ok);
    return true;
  }

  reportPidOutcome(g_soc_stats, PidOutcome::Format);
  return false;
}

// ===== RPM (PID 01 0C) =====
bool readEngineRpm(WiFiClient &client, uint16_t &rpm, uint32_t waitMs) {
  rpm = 0;
  if (!ensureElmConnected(client)) return false;

  while (client.available()) client.read();

  client.print("01 0C\r");
  USBSerial.println("RPM request: <01 0C>");

  String line;
  bool no_data = false;
  if (!readObdLineWithPrefix(client, "41 0C", line, waitMs, &no_data)) {
    reportPidOutcome(g_rpm_stats, no_data ? PidOutcome::NoData : PidOutcome::Timeout);
    return false;
  }

  std::vector<String> parts;
  if (!splitTokens(line, parts)) {
    reportPidOutcome(g_rpm_stats, PidOutcome::Format);
    return false;
  }

  if (parts.size() >= 4 && parts[0] == "41" && parts[1] == "0C") {
    long a = strtol(parts[2].c_str(), nullptr, 16);
    long b = strtol(parts[3].c_str(), nullptr, 16);
    if (a < 0 || a > 255 || b < 0 || b > 255) return false;

    rpm = (uint16_t)(((a * 256L) + b) / 4L);

    reportPidOutcome(g_rpm_stats, PidOutcome::Ok);
    return true;
  }

  reportPidOutcome(g_rpm_stats, PidOutcome::Format);
  return false;
}

// ===== Speed (PID 01 0D) =====
// A = km/h
bool readVehicleSpeed(WiFiClient &client, uint8_t &speedKmh, uint32_t waitMs) {
  speedKmh = 0;
  if (!ensureElmConnected(client)) return false;

  while (client.available()) client.read();

  client.print("01 0D\r");
  USBSerial.println("Speed request: <01 0D>");

  String line;
  bool no_data = false;
  if (!readObdLineWithPrefix(client, "41 0D", line, waitMs, &no_data)) {
    reportPidOutcome(g_speed_stats, no_data ? PidOutcome::NoData : PidOutcome::Timeout);
    return false;
  }

  std::vector<String> parts;
  if (!splitTokens(line, parts)) {
    reportPidOutcome(g_speed_stats, PidOutcome::Format);
    return false;
  }

  if (parts.size() >= 3 && parts[0] == "41" && parts[1] == "0D") {
    long a = strtol(parts[2].c_str(), nullptr, 16);
    if (a < 0 || a > 255) return false;

    speedKmh = (uint8_t)a;

    reportPidOutcome(g_speed_stats, PidOutcome::Ok);
    return true;
  }

  reportPidOutcome(g_speed_stats, PidOutcome::Format);
  return false;
}

// ===== Engine Load (PID 01 04) =====
// % = A*100/255
bool readEngineLoad(WiFiClient &client, uint8_t &loadPercent, uint32_t waitMs) {
  loadPercent = 0;
  if (!ensureElmConnected(client)) return false;

  while (client.available()) client.read();

  client.print("01 04\r");
  USBSerial.println("Engine Load request: <01 04>");

  String line;
  if (!readObdLineWithPrefix(client, "41 04", line, waitMs)) {
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
bool readFuelLevel(WiFiClient &client, uint8_t &fuelPercent, uint32_t waitMs) {
  fuelPercent = 0;
  if (!ensureElmConnected(client)) return false;

  while (client.available()) client.read();

  client.print("01 2F\r");
  USBSerial.println("Fuel Level request: <01 2F>");

  String line;
  bool no_data = false;
  if (!readObdLineWithPrefix(client, "41 2F", line, waitMs, &no_data)) {
    reportPidOutcome(g_fuel_stats, no_data ? PidOutcome::NoData : PidOutcome::Timeout);
    return false;
  }

  std::vector<String> parts;
  if (!splitTokens(line, parts)) {
    reportPidOutcome(g_fuel_stats, PidOutcome::Format);
    return false;
  }

  if (parts.size() >= 3 && parts[0] == "41" && parts[1] == "2F") {
    long a = strtol(parts[2].c_str(), nullptr, 16);
    if (a < 0 || a > 255) return false;

    fuelPercent = (uint8_t)((a * 100UL) / 255UL);

    reportPidOutcome(g_fuel_stats, PidOutcome::Ok);
    return true;
  }

  reportPidOutcome(g_fuel_stats, PidOutcome::Format);
  return false;
}
