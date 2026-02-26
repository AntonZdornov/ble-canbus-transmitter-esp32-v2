#pragma once

struct UiData {
  int battery_percent; // 0-100, or -1 if unknown
  int distance_km;     // total distance, or -1 if unknown
  int time_minutes;    // total minutes, or -1 if unknown
};
