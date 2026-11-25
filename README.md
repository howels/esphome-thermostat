# esphome-thermostat

ESPHome LVGL firmware and a single Home Assistant automation blueprint for a WT32S3-21S (ZX2D10GE01R-V4848) 480×480 rotary display acting as a central and multi-room thermostat.

## Architecture

- **ESPHome firmware** (`esphome/thermostat-wt32s3-21s-base.yaml`): drives the ST7701S display, rotary encoder, push button, backlight timeout with 5 s fade-out, and LVGL UI. It publishes `Node name` (`App.get_name()`) and registers as project `howels.thermostat`. Room lists and outdoor weather arrive over `esphome.<node>_set_rooms` and `esphome.<node>_set_weather`. Dial changes fire `esphome.thermostat_room_set` and `esphome.thermostat_rooms_request` with the node name in the event data.
- **Local device file** (`esphome/thermostat-example.yaml`): holds only per-device substitutions, Wi-Fi credentials, fallback AP details and the API encryption key, then includes the base package.
- **Home Assistant blueprint** (`blueprints/thermostat_rooms.yaml`): one automation per thermostat. Each automation selects its device and resolves that device's Room targets entity and action names on every run. It coordinates central heating, radiator valves (TRVs) and air conditioning for the areas configured in that automation.

## UI overview

- **Main dial**: target temperature roller, central mode colour, current temperature, and a condensed 1- or 2-line room summary showing each room's icon (or initial) and rounded temperature. When the heating is off, the main dial displays outdoor weather, daily high/low/rain, feels-like temperature, humidity, and wind.
- **Circular sub-menu** (short click from main dial): 8-slot ring with Back, Mode, and up to 6 rooms. Focused items appear in three centred lines (title, detail, secondary info). On Back, the centre displays the day/time and outdoor weather. Room buttons show each room's configured icon (or initial) coloured by action: orange for heating, blue for cooling, grey for off, white for idle.
- **Mode menu**: single ring selecting `Off`, `Heat only`, `Cool only`, or `Heat or cool`.
- **Room page**: rotate to adjust the selected room's target temperature (debounced by 700 ms); short click returns to the room ring; long press returns to the main dial.
- **Backlight timeout**: exposed as a `Backlight timeout` configuration number entity in Home Assistant (0–3600 s, 0 disables). When the timer expires, the backlight and RGB ring fade out over 5 s. Any press or rotation wakes the display without activating a widget.

## Installation

### 1. ESPHome

1. Copy the contents of `esphome/` into your ESPHome `config/` directory.
2. Copy `thermostat-example.yaml` to `thermostat-wt32s3-21s.yaml` and fill in your substitutions and `!secret` entries.
3. Compile and upload with ESPHome 2026.11 or newer:
   ```bash
   esphome run thermostat-wt32s3-21s.yaml
   ```
4. In Home Assistant, open Settings > Devices & services > ESPHome, choose Configure on the thermostat entry, and enable **Allow the device to perform Home Assistant actions**.

### 2. Home Assistant blueprint

1. Copy `blueprints/thermostat_rooms.yaml` to `/config/blueprints/automation/esphome-thermostat/thermostat_rooms.yaml`, or import it from this repository URL.
2. Reload automations (Settings > Automations & scenes > three-dot menu > Reload automations).
3. Create one automation from **Thermostat rooms controller** for each thermostat:
   - Select that thermostat under **Thermostat**. The device appears after the firmware that publishes project `howels.thermostat` and the Node name sensor has been flashed.
   - Configure up to six rooms in **Room 1** through **Room 6** (area, icon, optional external temperature sensor, and heating/cooling max offsets). Leave unused room areas empty.
   - Select the **Central heating thermostat** and an optional **Exclusion label**. Room targets are taken from the `Room targets` text entity on the selected device.
   - Configure valve capping, central target raising, and AC assist options.
   - Select your **Weather entity** and optional outdoor temperature, humidity, and feels-like sensor overrides.

## External sensor offset compensation

Each room accepts an optional external temperature sensor (otherwise the blueprint uses the median of temperature sensors in that area) plus separate heating and cooling maximum offsets.

- **Heating (TRVs and AC in heat mode)**: when the external sensor is below the requested target `T` and `0 < (unit_temp - external_temp) <= heat_max_offset`, the unit setpoint is raised by `unit_temp - external_temp` so it continues heating until the external sensor reaches `T`. If the gap exceeds `heat_max_offset`, no boost is applied.
- **Cooling (AC in cool mode)**: when the external sensor is above `T` and `0 < (external_temp - unit_temp) <= cool_max_offset`, the unit setpoint is lowered by `external_temp - unit_temp` so it continues cooling until the external sensor reaches `T`. If the gap exceeds `cool_max_offset`, no extra cooling offset is applied.

Requested room targets are stored in the selected thermostat's `Room targets` text entity, found from the device on every run, so periodic one-minute re-evaluation adjusts units without runaway. Readings outside 0–45 °C or gaps larger than the configured maximum offset are ignored, and the final setpoint is always clamped to `requested ± max_offset` and the unit's own min/max limits.

## Releases

Versions are Git tags of the form `vMAJOR.MINOR.PATCH`. A tag with a hyphenated suffix, such as `v1.1.0-rc.1`, is a pre-release. Pushing the tag runs `.github/workflows/release.yml`. The workflow checks that `esphome.project.version` in `esphome/thermostat-wt32s3-21s-base.yaml` equals the tag with the leading `v` and any pre-release suffix removed, then opens a GitHub release. The release notes are the annotated tag message. A lightweight tag has no message, and the workflow fails.

A change to the blueprint inputs, the `node` field on the thermostat events, or the `esphome.<node>_set_*` actions is breaking and takes a new major version. Set `project.version` to that version and commit it before tagging.

`v1.0.0` is the first release. It breaks automations imported from earlier unversioned `main`: the Room targets entity input is removed, each automation selects one thermostat, and the firmware that publishes project `howels.thermostat` and the Node name sensor must be flashed before that thermostat appears in the list. One automation per device uses the same blueprint.

From the repository root, after the version commit is on `main`:

```bash
git tag -a v1.0.0 -m "v1.0.0

Breaking: select the thermostat in each automation. Flash the 1.0.0 firmware first."
git push origin v1.0.0
```

Import a pinned blueprint from `https://github.com/howels/esphome-thermostat/blob/vX.Y.Z/blueprints/thermostat_rooms.yaml`. The `source_url` in the blueprint tracks `main`.

## Attribution

Based on [kancelott/neo-nesp](https://github.com/kancelott/neo-nesp) (forked from `velijv/nesp`), which provided the WT32S3-21S hardware configuration, ST7701S display initialisation, colour palette, and original LVGL dial layout.
