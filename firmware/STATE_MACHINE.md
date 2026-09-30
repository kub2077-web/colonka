# Контракт аппарата состояний

Аппарат состояний намеренно разделён на независимые группы:

- связь: `OFFLINE`, `WIFI_ONLY`, `HOME_ASSISTANT_READY`;
- голосовая сессия: `IDLE`, `LOCAL_COMMAND_WINDOW`, `REMOTE_AUDIO`;
- источник воспроизведения: `NONE`, `RADIO`, `SD`;
- состояние воспроизведения: `STOPPED`, `STARTING`, `PLAYING`, `ERROR`.

Это не позволяет модулю воспроизведения захватить микрофон, а переходу сети
изменить текущий трек.

## Подключение

Добавить пакет в `esp32-s3-speaker.yaml`:

```yaml
packages:
  state_machine: !include state-machine.yaml
```

Пакет уже обрабатывает события переподключения Wi‑Fi/API и отключает оба
автоматических таймера перезагрузки. Сети Wi‑Fi, fallback AP, шифрование API
и аудиоустройства голосового ассистента остаются в основном файле.

## Подключение голосовых событий

Текущие callbacks голосового ассистента должны вызывать эти скрипты:

```yaml
voice_assistant:
  on_start:
    - script.execute: listening_indicator
  on_end:
    - script.execute: restore_playback_indicator
    - script.execute: sm_voice_session_finished
  on_error:
    - script.execute: error_indicator
    - script.execute: sm_voice_remote_failed
  on_client_connected:
    - logger.log: "Голосовой клиент Home Assistant подключён"
  on_client_disconnected:
    - logger.log: "Голосовой клиент Home Assistant отключён"
```

Модель `Скади` должна вызывать `sm_start_voice_session`. Командные модели
должны вызывать `sm_route_voice_command` со следующими значениями:

```text
Радио   0
Память  1
Назад   2
Дальше  3
Громче  4
Тише    5
Стоп    6
```

`sm_start_voice_session` выбирает Home Assistant только если состояние связи и
`voice_assistant.connected` это позволяют. Иначе открывается локальное окно
команд.

## Подключение воспроизведения

Модули радио и SD должны использовать:

- `sm_set_radio_source` / `sm_set_sd_source` when they become active;
- `sm_playback_started` после получения первых корректных аудиоданных;
- `sm_playback_paused` после постановки активного источника на паузу;
- `sm_playback_failed`, если станция или MP3 не работают;
- `sm_route_pause` как общую точку паузы/возобновления;
- `sm_select_radio_source` / `sm_select_sd_source` для взаимоисключающего
  выбора источника;
- `sm_toggle_playback_source` для переключения между радио и SD;
- `sm_route_next` и `sm_route_previous` как точки входа для навигации;
- `sm_volume_up` и `sm_volume_down` для общей громкости.

Сканер радио и кэш битых файлов SD остаются внутри своих модулей. Аппарат
состояний только маршрутизирует запрос и хранит общее состояние
воспроизведения.