# CoolChain-Monitor-ESP32

Smart cold-chain monitoring & control using **ESP32 + NTC**:  
- Relay-based **heater & cooler** with hysteresis  
- **LCD 16x2** status display  
- **Buzzer** alerts
- **Telegram bot** notifications over Wi‑Fi
- Ready to run in **Wokwi** (or real ESP32 hardware)

## Features
- Real-time temperature from NTC (voltage divider to GPIO34)
- Control logic:
  - Heater ON < 0°C (OFF > 2°C)
  - Cooler ON > 10°C (OFF < 8°C)
- Alerts:
  - Buzzer + Telegram when < -1°C or > 12°C
  - One-message cooldown to avoid spam
- Clean repo with `secrets.h` excluded from Git

## Hardware (Wokwi / Real)
- ESP32 DevKit V1
- 10k NTC Thermistor + 10k resistor (to GND) as divider
- 2× Relay modules (Heater on GPIO25, Cooler on GPIO27)
- Active buzzer on GPIO26
- LCD 16x2 I2C on SDA=GPIO21, SCL=GPIO22

## Wiring (summary)
- NTC: 3.3V — [NTC] — node → **GPIO34**, and node — **10kΩ** → GND
- Relays: IN pins to GPIO25/27, VCC=5V, GND=GND
- Buzzer: + → GPIO26, − → GND
- LCD I2C: SDA→GPIO21, SCL→GPIO22, VCC=5V, GND

See **circuit.png** / **circuit.pdf** for a clear diagram.

## Setup
1. Clone/download this repo.
2. Copy `src/secrets_example.h` → `src/secrets.h`, then fill:
   - `WIFI_SSID`, `WIFI_PASSWORD`
   - `BOT_TOKEN` (Telegram bot token from BotFather)
   - `CHAT_ID` (your chat/user ID)
3. Open `src/ColdChainMonitor.ino` in Arduino IDE (select **ESP32 Dev Module**).
4. Install libraries if prompted: `LiquidCrystal_I2C` (and `Wire` is built-in).
5. Upload to ESP32 or run in Wokwi.

### Telegram Notes
- Create a bot via **@BotFather**, get the token.
- Start a chat with your bot and send a message once, then use your **chat ID** (you can find it via @userinfobot or API).

## Wokwi
- Add ESP32 DevKit V1, NTC (thermistor), two relays, buzzer, LCD 16x2 I2C.
- Wire as shown. Paste the code from `src/ColdChainMonitor.ino`.
- Use the NTC widget temperature slider to test heater/cooler + alerts.
