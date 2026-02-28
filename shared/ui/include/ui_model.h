#pragma once

struct UiData {
  int battery_percent; // 0-100, or -1 if unknown
  int rpm;             // engine RPM, or -1 if unknown
  int time_minutes;    // total minutes, or -1 if unknown
};
