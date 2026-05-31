# ESP32 Battery Experiments

Прошивка ESP32 для експериментального макета бакалаврської роботи "Система управління процесами заряджання і розряджання акумулятора".

Реалізує **6 експериментальних сценаріїв** з Розділу 4 БКР через єдиний інтерфейс. Перемикання між сценаріями — через локальний web-інтерфейс або фізичну кнопку.

## Апаратний склад макета

- **ESP32 DevKit V1** — центральний мікроконтролер
- **INA3221** — трьохканальний датчик струму/напруги (заряд)
- **INA219** — датчик струму/напруги (розряд)
- **Сонячна панель з USB-виходом** (~5 В)
- **ESP8266** — узгоджувальний інтерфейсний міст (окремий проєкт, див. `firmware-esp8266/`)
- **Холдер 18650 + Li-Ion комірка**
- **DC-DC підвищувальний 3.7→5В** з USB-виходом
- **OLED SSD1306** 0.96″ I²C (опціонально, для локального відображення)

## Експериментальні сценарії

| # | Сценарій | Опис |
|---|---|---|
| E1 | Заряд від USB 5В | Опорний експеримент зі стабільним джерелом |
| E2 | Заряд від PV (сонячно) | Прямі умови освітлення |
| E3 | Заряд від PV (хмарно) | Непрямі умови, порівняння з E2 |
| E4 | Розряд на навантаження | Крива розряду + ємність акумулятора |
| E5 | Верифікація точності SOC | Повний цикл заряд-розряд + порівняння |
| E6 | Автономність за різних SOC_min | Час роботи від рівнів 20/30/45/60/70% |

## Швидкий старт

```bash
# 1. Скопіювати приклад конфігурації
cp include/secrets.h.example include/secrets.h
# Заповнити WiFi credentials та ThingSpeak API ключ

# 2. Збірка та завантаження прошивки
pio run --target upload

# 3. Завантаження web-інтерфейсу
pio run --target uploadfs

# 4. Моніторинг
pio device monitor
```

Після перезавантаження ESP32:
- Підключається до WiFi (або переходить у AP-режим для першого налаштування)
- Відкриває web-інтерфейс на `http://[esp-ip]/`
- У web-інтерфейсі обираєш активний експеримент і запускаєш

## Структура проєкту

```
.
├── platformio.ini              # Конфігурація PlatformIO
├── src/
│   ├── main.cpp                # Точка входу, FreeRTOS таски
│   ├── SensorManager.cpp/.h    # Опитування INA3221 + INA219
│   ├── SOCEstimator.cpp/.h     # Кулонометрія + корекція по U(SOC)
│   ├── ExperimentRunner.cpp/.h # Логіка експериментів E1-E6
│   ├── WebServer.cpp/.h        # AsyncWebServer + WebSocket
│   ├── ThingSpeakPublisher.cpp/.h  # HTTP POST у ThingSpeak
│   ├── DisplayManager.cpp/.h   # OLED SSD1306
│   ├── EventLogger.cpp/.h      # Журнал подій у SPIFFS
│   ├── ConfigManager.cpp/.h    # Налаштування у NVS
│   └── DataExporter.cpp/.h     # Експорт CSV з SPIFFS
├── include/
│   ├── pin_config.h            # Розпинівка GPIO ESP32
│   ├── constants.h             # Q_nom, U_max, пороги SOC
│   └── secrets.h.example       # Шаблон WiFi/ThingSpeak credentials
├── data/                       # SPIFFS web-ресурси
│   ├── index.html
│   ├── style.css
│   └── app.js
├── test/                       # Unit-тести
│   └── test_soc_estimator/
└── .github/workflows/build.yml # CI/CD
```

## ThingSpeak налаштування

Створіть один Channel з 6 полями:
- Field 1: `V_charge` (В)
- Field 2: `I_charge` (мА)
- Field 3: `V_discharge` (В)
- Field 4: `I_discharge` (мА)
- Field 5: `SOC` (%)
- Field 6: `Power` (Вт)

Скопіювати Channel ID та Write API Key у `include/secrets.h`.

## Експорт даних для звіту

Після завершення експерименту:
1. Відкрити `http://[esp-ip]/api/export/csv` — отримати CSV з локальної SPIFFS
2. Або експортувати з ThingSpeak через UI
3. Завантажити дані у Excel/Origin для обробки графіків Розділу 4

## Ліцензія

MIT License. Робота виконана в межах бакалаврської кваліфікаційної роботи у Національному університеті "Львівська політехніка", 2026.
