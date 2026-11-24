#pragma once
// Room list pushed by Home Assistant (blueprints/thermostat_rooms.yaml)
// through the set_rooms API action.
//
// Format: "v1;" followed by records separated by ';'.
// Record: area_id|name|temperature|target|action|icon
// Empty temperature or target means unknown. Action is heating, cooling, off or idle.
// Icon is an optional key mapped to a Material Design Icons glyph; empty uses the room initial.
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

struct ThermostatRoom {
  std::string id;
  std::string name;
  std::string action;
  std::string icon;
  float temp{NAN};
  float target{NAN};
};

struct ThermostatRooms {
  static constexpr int MAX = 6;
  ThermostatRoom room[MAX];
  int count{0};
  uint32_t stamp{0};  // millis() of the last successful parse

  static float to_float(const std::string &s) {
    if (s.empty())
      return NAN;
    char *end = nullptr;
    float v = strtof(s.c_str(), &end);
    return end == s.c_str() ? NAN : v;
  }

  // Material Design Icons glyph for a room icon key, or "" for initial-letter fallback.
  static const char *icon_for(const std::string &k) {
    if (k == "bed") return "\U000F02E3";
    if (k == "sofa") return "\U000F04B9";
    if (k == "stove") return "\U000F04DE";
    if (k == "silverware") return "\U000F0A70";
    if (k == "desk") return "\U000F1239";
    if (k == "shower") return "\U000F09A0";
    if (k == "table_chair") return "\U000F1061";
    if (k == "home") return "\U000F02DC";
    if (k == "stairs") return "\U000F04CD";
    if (k == "washing") return "\U000F072A";
    if (k == "garage") return "\U000F06D9";
    if (k == "baby") return "\U000F068F";
    if (k == "radiator") return "\U000F0438";
    if (k == "thermometer") return "\U000F050F";
    return "";
  }

  // Replaces the room list. Returns false and clears the list if the payload is not version 1.
  bool parse(const std::string &s, uint32_t now_ms) {
    count = 0;
    if (s.compare(0, 3, "v1;") != 0)
      return false;
    stamp = now_ms;
    size_t pos = 3;
    while (pos <= s.size() && count < MAX) {
      size_t end = s.find(';', pos);
      if (end == std::string::npos)
        end = s.size();
      std::string rec = s.substr(pos, end - pos);
      pos = end + 1;
      if (rec.empty())
        continue;
      std::string f[6];
      size_t start = 0;
      int n = 0;
      while (n < 6) {
        size_t bar = rec.find('|', start);
        if (bar == std::string::npos) {
          f[n++] = rec.substr(start);
          break;
        }
        f[n++] = rec.substr(start, bar - start);
        start = bar + 1;
      }
      if (n < 5 || f[0].empty())
        continue;
      ThermostatRoom &r = room[count++];
      r.id = f[0];
      r.name = f[1];
      r.temp = to_float(f[2]);
      r.target = to_float(f[3]);
      r.action = f[4];
      r.icon = (n >= 6) ? f[5] : "";
    }
    return true;
  }

  // Clears the list if nothing has arrived for max_age_ms. Returns true if it cleared.
  bool expire(uint32_t now_ms, uint32_t max_age_ms) {
    if (count > 0 && (uint32_t) (now_ms - stamp) > max_age_ms) {
      count = 0;
      return true;
    }
    return false;
  }

  // 1-based index of the room with this id, or 0.
  int find(const std::string &id) const {
    for (int i = 0; i < count; i++)
      if (room[i].id == id)
        return i + 1;
    return 0;
  }

  int heating() const {
    int n = 0;
    for (int i = 0; i < count; i++)
      if (room[i].action == "heating")
        n++;
    return n;
  }

  // Text colour for a room button: orange heating, blue cooling, grey off, white idle.
  static uint32_t color_for(const std::string &action) {
    if (action == "heating") return 0xFF9800;
    if (action == "cooling") return 0x2196F3;
    if (action == "off") return 0x455A64;
    return 0xFFFFFF;
  }

  // Glyph for a room button (0-based index): configured icon, or uppercase initial letter.
  std::string button_label(int idx) const {
    if (idx < 0 || idx >= count)
      return "";
    const char *g = icon_for(room[idx].icon);
    if (g[0])
      return std::string(g);
    if (!room[idx].name.empty()) {
      char c = (char) toupper((unsigned char) room[idx].name[0]);
      return std::string(1, c);
    }
    return "";
  }

  // Condensed 1- or 2-line room summary for the main dial (icon/initial + rounded temp).
  // 1-3 rooms on one line; 4 rooms as 2+2; 5-6 rooms as 3+2 or 3+3.
  std::string main_summary() const {
    if (count == 0)
      return "";
    int per_line = (count <= 3) ? count : (count == 4 ? 2 : 3);
    std::string out;
    char buf[16];
    for (int i = 0; i < count; i++) {
      if (i > 0)
        out += (i == per_line) ? "\n" : "   ";
      out += button_label(i);
      if (std::isnan(room[i].temp))
        snprintf(buf, sizeof(buf), " --");
      else
        snprintf(buf, sizeof(buf), " %.0f°", roundf(room[i].temp));
      out += buf;
    }
    return out;
  }

  std::string summary() const {
    std::string out;
    for (int i = 0; i < count; i++) {
      if (i)
        out += ", ";
      out += room[i].name;
    }
    return out.empty() ? "none" : out;
  }
};

inline ThermostatRooms thermostat_rooms;
