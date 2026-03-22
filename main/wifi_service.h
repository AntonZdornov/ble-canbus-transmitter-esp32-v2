#pragma once

void initWifi();
void requestWifiReconnect();
void toggleWifiEnabled();
void wifiServiceTick();
bool isWifiManualReconnectInProgress();
bool isWifiEnabled();
