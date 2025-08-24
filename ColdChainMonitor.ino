#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"   // <-- create your own file from src/secrets_example.h

// LCD 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin mapping
#define NTC_PIN     34
#define BUZZER      26
#define RELAY_HEAT  25
#define RELAY_COOL  27

// Thresholds (°C) — adjust to your needs
float heaterOn  = 2.0;  // turn heater ON below this
float heaterOff = 3.0;  // turn heater OFF above this
float coolerOn  = 8.0;  // turn cooler ON above this
float coolerOff = 7.0;  // turn cooler OFF below this
float alarmLow  = 1.5;  // Telegram + buzzer if below
float alarmHigh = 9.0;  // Telegram + buzzer if above

// Track state
static String lastState = "NORMAL";

// Optional: simple cooldown so Telegram isn't spammed
unsigned long lastMsgMs = 0;
const unsigned long MSG_COOLDOWN = 30000; // 30s

///////////////////////////
// Telegram message sender
void sendTelegram(String message) {
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastMsgMs < MSG_COOLDOWN) return; // cooldown
  lastMsgMs = millis();

  // very-basic URL encoding for spaces
  message.replace(" ", "%20");

  String url = "https://api.telegram.org/bot" + BOT_TOKEN +
               "/sendMessage?chat_id=" + CHAT_ID +
               "&text=" + message;

  HTTPClient http;
  http.begin(url);
  http.GET();
  http.end();
}

// Read NTC temperature (fast approximate)
// NOTE: For best accuracy, replace with proper Steinhart–Hart using your NTC's Beta.
// Assumes 10k NTC with 10k to GND divider, 3.3V ref, ADC 12-bit (0..4095).
float readTemperature() {
  int raw = analogRead(NTC_PIN);
  // Avoid division by zero
  if (raw < 1) raw = 1;

  // Calculate thermistor resistance from divider:
  // Vout = 3.3 * (R_ntc / (R_ntc + R_fixed)), where R_fixed = 10k (to GND)
  // Rearranged: R_ntc = R_fixed * Vout / (3.3 - Vout)
  float vout = (raw * 3.3f) / 4095.0f;
  const float R_FIXED = 10000.0f; // 10k
  float r_ntc = (R_FIXED * vout) / (3.3f - vout);

  // Beta-model (quick) around 25°C
  const float BETA = 3950.0f;     // change if your NTC is different
  const float T0   = 298.15f;     // 25°C in Kelvin
  const float R0   = 10000.0f;    // 10k at 25°C

  float invT = (1.0f/BETA) * log(r_ntc / R0) + (1.0f / T0);
  float T    = (1.0f / invT) - 273.15f; // °C
  return T;
}


void setup() {
  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY_HEAT, OUTPUT);
  pinMode(RELAY_COOL, OUTPUT);

  digitalWrite(BUZZER, LOW);
  digitalWrite(RELAY_HEAT, LOW);
  digitalWrite(RELAY_COOL, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("ColdChain Init");

  // Wi-Fi connect
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lcd.setCursor(0, 1);
  lcd.print("WiFi Connecting");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(300);
    Serial.print(".");
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("WiFi Offline  ");
  }
  delay(1000);
  lcd.clear();
}


void loop() {
  // try to reconnect if needed
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  float T = readTemperature();
  Serial.printf("Temp: %.2f C\n", T);

  ////// Heater Control (hysteresis)
  static bool heaterOnState = false;
  if (!heaterOnState && T < heaterOn) {
    digitalWrite(RELAY_HEAT, HIGH);
    heaterOnState = true;
  }
  if (heaterOnState && T > heaterOff) {
    digitalWrite(RELAY_HEAT, LOW);
    heaterOnState = false;
  }

  ////// Cooler Control (hysteresis)
  static bool coolerOnState = false;
  if (!coolerOnState && T > coolerOn) {
    digitalWrite(RELAY_COOL, HIGH);
    coolerOnState = true;
  }
  if (coolerOnState && T < coolerOff) {
    digitalWrite(RELAY_COOL, LOW);
    coolerOnState = false;
  }

  ////// Alerts
  String currentState = "NORMAL";
  if (T < alarmLow) currentState = "LOW";
  else if (T > alarmHigh) currentState = "HIGH";

  static bool buzzerOn = false;
  if (currentState != lastState) {
    if (currentState == "LOW") {
      buzzerOn = true;
      sendTelegram("ALERT: Temperature TOO LOW! Current = " + String(T, 1) + " C");
    } else if (currentState == "HIGH") {
      buzzerOn = true;
      sendTelegram("ALERT: Temperature TOO HIGH! Current = " + String(T, 1) + " C");
    } else {
      buzzerOn = false;
      sendTelegram("Temperature back to NORMAL. Current = " + String(T, 1) + " C");
    }
    lastState = currentState;
  }

  // Buzzer beeps when in alert state
  if (buzzerOn) {
    digitalWrite(BUZZER, HIGH);
  } else {
    digitalWrite(BUZZER, LOW);
  }

  ////// LCD Display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:");
  lcd.print(T, 1);
  lcd.print((char)223); // degree symbol
  lcd.print("C");

  lcd.setCursor(0, 1);
  if (heaterOnState) lcd.print("Heater ON ");
  else if (coolerOnState) lcd.print("Cooler ON ");
  else lcd.print("Stable    ");

  delay(1000);
}
