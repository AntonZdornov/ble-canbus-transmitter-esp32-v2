#pragma once
#include <WiFiClient.h>

// Вызывается в loop для чтения SOC
// Возвращает true, если удалось получить значение
bool readSocRaw(WiFiClient &client, uint8_t &soc);
bool readEngineRpm(WiFiClient &client, uint16_t &rpm);
bool readVehicleSpeed(WiFiClient &client, uint8_t &speedKmh);
bool readEngineLoad(WiFiClient &client, uint8_t &loadPercent);
bool readFuelLevel(WiFiClient &client, uint8_t &fuelPercent);