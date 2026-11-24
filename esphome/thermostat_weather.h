#pragma once
// Outdoor weather pushed by Home Assistant (ha/blueprints/automation/thermostat_rooms.yaml)
// through the set_weather API action.
//
// Format: "w1;" followed by one record, or nothing when weather is not configured.
// Record: condition|temp|feels|humidity|wind|wind_unit|high|low|rain|precip
// Empty means unknown. condition is a Home Assistant weather state such as
// sunny, partlycloudy or lightning-rainy. rain is a percentage, precip is mm.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "thermostat_rooms.h"

struct ThermostatWeather {
  bool valid{false};
  std::string condition;
  std::string wind_unit;
  float temp{NAN};
  float feels{NAN};
  float humidity{NAN};
  float wind{NAN};
  float high{NAN};
  float low{NAN};
  float rain{NAN};
  float precip{NAN};
  uint32_t stamp{0};  // millis() of the last successful parse

  void clear() {
    valid = false;
    condition.clear();
    wind_unit.clear();
    temp = feels = humidity = wind = high = low = rain = precip = NAN;
  }

  // Returns false if the payload is not version 1. "w1;" alone is accepted and
  // leaves valid false, which hides weather on the display.
  bool parse(const std::string &s, uint32_t now_ms) {
    clear();
    if (s.compare(0, 3, "w1;") != 0)
      return false;
    stamp = now_ms;
    std::string f[10];
    size_t start = 3;
    for (int n = 0; n < 10; n++) {
      size_t bar = s.find('|', start);
      if (bar == std::string::npos) {
        f[n] = s.substr(start);
        break;
      }
      f[n] = s.substr(start, bar - start);
      start = bar + 1;
    }
    condition = f[0];
    temp = ThermostatRooms::to_float(f[1]);
    feels = ThermostatRooms::to_float(f[2]);
    humidity = ThermostatRooms::to_float(f[3]);
    wind = ThermostatRooms::to_float(f[4]);
    wind_unit = f[5];
    high = ThermostatRooms::to_float(f[6]);
    low = ThermostatRooms::to_float(f[7]);
    rain = ThermostatRooms::to_float(f[8]);
    precip = ThermostatRooms::to_float(f[9]);
    valid = !condition.empty() || !std::isnan(temp);
    return true;
  }

  // Drops the data if nothing has arrived for max_age_ms. Returns true if it dropped.
  bool expire(uint32_t now_ms, uint32_t max_age_ms) {
    if (valid && (uint32_t) (now_ms - stamp) > max_age_ms) {
      clear();
      return true;
    }
    return false;
  }

  // Material Design Icons glyph for a Home Assistant weather state. All glyphs are
  // in the roboto_reg, roboto_med and roboto_huge fonts.
  static const char *icon_for(const std::string &c) {
    if (c == "clear-night") return "\U000F0594";
    if (c == "cloudy") return "\U000F0590";
    if (c == "fog") return "\U000F0591";
    if (c == "hail") return "\U000F0592";
    if (c == "lightning") return "\U000F0593";
    if (c == "lightning-rainy") return "\U000F067E";
    if (c == "partlycloudy") return "\U000F0595";
    if (c == "pouring") return "\U000F0596";
    if (c == "rainy") return "\U000F0597";
    if (c == "snowy" || c == "snowy-rainy") return "\U000F0598";
    if (c == "sunny") return "\U000F0599";
    if (c == "windy" || c == "windy-variant") return "\U000F059D";
    if (c == "exceptional") return "\U000F0F30";
    return "";
  }

  static const char *text_for(const std::string &c) {
    if (c == "clear-night") return "Clear";
    if (c == "cloudy") return "Cloudy";
    if (c == "fog") return "Fog";
    if (c == "hail") return "Hail";
    if (c == "lightning") return "Thunder";
    if (c == "lightning-rainy") return "Storm";
    if (c == "partlycloudy") return "Part cloud";
    if (c == "pouring") return "Heavy rain";
    if (c == "rainy") return "Rain";
    if (c == "snowy") return "Snow";
    if (c == "snowy-rainy") return "Sleet";
    if (c == "sunny") return "Sunny";
    if (c == "windy" || c == "windy-variant") return "Windy";
    if (c == "exceptional") return "Severe";
    return "";
  }

  const char *icon() const { return valid ? icon_for(condition) : ""; }

  // "6°C", or "--" when unknown.
  std::string temp_text() const {
    if (!valid || std::isnan(temp))
      return "--";
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f°C", roundf(temp));
    return buf;
  }

  // Today's high, low and chance of rain (or amount, if there is no chance).
  std::string forecast_line() const {
    if (!valid)
      return "";
    std::string out;
    char buf[24];
    if (!std::isnan(high)) {
      snprintf(buf, sizeof(buf), "\U000F0E03%.0f°", roundf(high));
      out += buf;
    }
    if (!std::isnan(low)) {
      snprintf(buf, sizeof(buf), "%s\U000F0E02%.0f°", out.empty() ? "" : " ", roundf(low));
      out += buf;
    }
    if (!std::isnan(rain)) {
      snprintf(buf, sizeof(buf), "%s\U000F0597%.0f%%", out.empty() ? "" : "  ", roundf(rain));
      out += buf;
    } else if (!std::isnan(precip) && precip >= 0.1f) {
      snprintf(buf, sizeof(buf), "%s\U000F0597%.1fmm", out.empty() ? "" : "  ", precip);
      out += buf;
    }
    return out;
  }

  // Feels-like temperature, humidity and wind.
  std::string detail_line() const {
    if (!valid)
      return "";
    std::string out;
    char buf[32];
    if (!std::isnan(feels)) {
      snprintf(buf, sizeof(buf), "Feels %.0f°C", roundf(feels));
      out += buf;
    }
    if (!std::isnan(humidity)) {
      snprintf(buf, sizeof(buf), "%s\U000F058E%.0f%%", out.empty() ? "" : "  ", roundf(humidity));
      out += buf;
    }
    if (!std::isnan(wind)) {
      snprintf(buf, sizeof(buf), "%s\U000F059D%.0f%s", out.empty() ? "" : "  ", roundf(wind), wind_unit.c_str());
      out += buf;
    }
    return out;
  }

  // One line for the menu and rooms pages: icon, outdoor temperature and condition.
  std::string outside_line() const {
    if (!valid)
      return "";
    std::string out = icon();
    if (!out.empty())
      out += " ";
    out += temp_text();
    const char *t = text_for(condition);
    if (t[0]) {
      out += "  ";
      out += t;
    }
    return out;
  }
};

inline ThermostatWeather thermostat_weather;
