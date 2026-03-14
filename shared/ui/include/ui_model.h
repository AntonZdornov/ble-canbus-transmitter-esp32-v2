#pragma once

struct UiData {
  int battery_percent; // 0-100, or -1 if unknown
  int speed_kmh;       // speed in km/h, or -1 if unknown
  int wifi_connected;  // 1 connected, 0 disconnected, 2 manual reconnect, -1 unknown
  int fuel_percent;    // fuel level, 0-100, or -1 if unknown
  int distance_km;     // total distance, or -1 if unknown
  int ev_distance_km;  // distance in EV mode (engine off), or -1 if unknown
  int service_distance_km; // service distance, or -1 if unknown
  int service_ev_distance_km; // service EV distance, or -1 if unknown
  int engine_on;       // 1 = ICE, 0 = EV, -1 unknown
  int time_minutes;    // total minutes, or -1 if unknown
};
