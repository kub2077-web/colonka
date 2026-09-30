#pragma once

#include <cstddef>
#include <string>

namespace radio_station_store {

struct Station {
  std::string id;
  std::string name;
  std::string url;
};

struct StationSlot {
  const char *id;
  std::string name;
  std::string url;
  bool enabled;
};

inline bool valid(const StationSlot &station) {
  return !station.name.empty() && !station.url.empty();
}

inline int enabled_count(const StationSlot *stations, std::size_t station_count) {
  int count = 0;
  for (std::size_t index = 0; index < station_count; index++) {
    if (stations[index].enabled && valid(stations[index])) {
      count++;
    }
  }
  return count;
}

inline bool get_enabled_station(const StationSlot *stations, std::size_t count,
                                int index, Station &result) {
  if (index < 0) {
    return false;
  }

  int enabled_index = 0;
  for (std::size_t slot = 0; slot < count; slot++) {
    const StationSlot &station = stations[slot];
    if (!station.enabled || !valid(station)) {
      continue;
    }
    if (enabled_index != index) {
      enabled_index++;
      continue;
    }

    result.id = station.id;
    result.name = station.name;
    result.url = station.url;
    return true;
  }

  return false;
}

}  // namespace radio_station_store