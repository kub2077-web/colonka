# Persistent data schemas

These JSON files define the initial format for the data that will be stored in
internal flash by the firmware persistence layer.

## `stations.json`

Radio stations are kept separately from the SD card. This lets radio continue
working when the card is removed and lets the local application edit the
station list without touching MP3 files.

Each station will use:

```json
{
  "id": "stable-id",
  "name": "Station name",
  "url": "https://radio.example/stream.mp3",
  "enabled": true
}
```

`stations.json` содержит тестовый набор для шести слотов. При первой прошивке
эти значения дублируются в отдельных text entities `radio.yaml` через
`initial_value`, поэтому ручной ввод через веб-интерфейс не нужен. После запуска
редактируются уже сохранённые entities названия, URL и включения станции через
веб-интерфейс ESPHome.

## `settings.json`

The first version reserves:

- current source mode;
- selected station;
- shuffle history size;
- battery warning, critical, and shutdown thresholds.

Playback volume is intentionally not part of the persistent settings schema.
The device keeps it in RAM and starts at 30% after reboot.

The files are seeds/examples for the firmware persistence layer. Runtime
station data is stored in ESPHome Preferences/flash, not on the SD card.
