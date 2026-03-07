#pragma once
#include <WiFiClient.h>

// Вызывается в loop для чтения SOC
// Возвращает true, если удалось получить значение
bool readSocRaw(WiFiClient &client, uint8_t &soc, uint32_t waitMs = 200);
bool readEngineRpm(WiFiClient &client, uint16_t &rpm, uint32_t waitMs = 200);
bool readVehicleSpeed(WiFiClient &client, uint8_t &speedKmh, uint32_t waitMs = 200);
bool readEngineLoad(WiFiClient &client, uint8_t &loadPercent, uint32_t waitMs = 200);
bool readFuelLevel(WiFiClient &client, uint8_t &fuelPercent, uint32_t waitMs = 200);
