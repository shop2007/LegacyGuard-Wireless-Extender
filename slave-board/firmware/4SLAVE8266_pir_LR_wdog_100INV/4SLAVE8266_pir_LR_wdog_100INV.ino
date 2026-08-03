/*
selezionare board "NodeMCU 1.0 (ESP-12e Module)"

100INV il flag di errore non viene resettato dal polling ma ricevendo il codice STA155
       ledDiag si accende e si spegne cn il flag allarme, 
70INV se usato con un PIR con contatto normalmente chiuso in serie all'opto pir
70 nuovo ssid e password
20 doppio blink diag led durante pir 
12 no-alarm sostituito da "STA100" ed alarm sostituito da "STA113"
11 trasmette numero di call master e db radio
10 wdog e stampa file ino

IMPOSTAZIONI ARDUINO IDE (Wemos D1 mini, ESP8266):
- Board: "LOLIN(WEMOS) D1 mini (clone)"
- CPU Frequency: 80MHz
- Nessuna opzione "USB CDC on boot" richiesta (seriale sempre attiva via CH340)
- Display OLED esterno da cablare: SDA->D2, SCL->D1, VCC->3V3, GND->GND
*/

#include <Wire.h>
#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <string.h>

#include "Slave_ID.h" // identificazione del device

// ===================== DISPLAY =====================
// OLED esterno cablato su D2 (SDA) / D1 (SCL)
#define OLED_SDA D2
#define OLED_SCL D1
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

// ===================== PIR - DELAY RISCALDAMENTO =====================

#define SkipDelayPir D6 // pull-up interno; a GND salta il delay
#define DelayPirSeconds 90 // secondi di attesa riscaldamento sensore PIR

// ===================== PIR - INGRESSO ALLARME =====================

#define InputPir D5 // pull-up interno
#define PirReverse false // se true: allarme quando InputPir legge 0 (e viceversa)

volatile bool pirAlarmLatched = false;

// Server TCP (dichiarazione mancante nello snippet originale ma necessaria)
// NOTA: WiFiServer tcpServer è già dichiarato all'interno di "Slave_ID.h" WiFiServer tcpServer(80);

// ===================== LED BUILTIN =====================

#define LED_BUILTIN_PIN LED_BUILTIN
#define LED_ACTIVE_LOW true

bool ledState = false;

void setLed(bool on) {
  digitalWrite(LED_BUILTIN_PIN, (on != LED_ACTIVE_LOW) ? HIGH : LOW);
}

void toggleLed() {
  ledState = !ledState;
  setLed(ledState);
}

void blinkWarmupDouble() {
  const bool savedLedState = ledState;

  setLed(true);
  feedWatchdog();
  delay(120);

  setLed(false);
  feedWatchdog();
  delay(120);

  setLed(true);
  feedWatchdog();
  delay(120);

  setLed(savedLedState);
  feedWatchdog();
  delay(640);
}


// ===================== LED DIAGNOSTICO ALLARME =====================
// D7 -> resistore -> anodo LED; catodo LED -> GND  (attivo HIGH)
#define LED_DIAG_PIN D7

void setDiagLed(bool on) {
  digitalWrite(LED_DIAG_PIN, on ? HIGH : LOW);
}

// ===================== VARIABILI GENERALI =====================

unsigned long lastDisplayUpdate = 0;
const unsigned long displayUpdateInterval = 200;
uint32_t masterPollCount = 0;

void feedWatchdog() {
  ESP.wdtFeed();
}

void initWatchdog() {
  ESP.wdtEnable(WDTO_8S);
  Serial.println("Watchdog software ESP8266 attivo: timeout lungo 8 s.");
}

const char* getSourceFileName() {
  const char* path = __FILE__;
  const char* base = path;

  while (*path != '\0') {
    if (*path == '/' || *path == '\\') {
      base = path + 1;
    }
    path++;
  }

  static char fileName[96];
  size_t len = strlen(base);

  if (len > 4 && strcmp(base + len - 4, ".cpp") == 0) {
    len -= 4;
  }

  if (len >= sizeof(fileName)) {
    len = sizeof(fileName) - 1;
  }

  memcpy(fileName, base, len);
  fileName[len] = '\0';
  return fileName;
}

void printSourceFileName() {
  Serial.print("File .ino: ");
  Serial.println(getSourceFileName());
}

// ===================== FUNZIONI PIR =====================

void countdownPir() {
  pinMode(SkipDelayPir, INPUT_PULLUP);
  delay(10);

  int pinState = digitalRead(SkipDelayPir);
  Serial.println();
  Serial.print("Slave id=");
  Serial.print(SLAVE_ID);
  Serial.print(" - Pin SkipDelayPir letto: ");
  Serial.println(pinState);

  if (pinState == 0) {
    Serial.println("Pin a GND -> delay riscaldamento PIR saltato.");
    return;
  }

  Serial.println("Avvio countdown riscaldamento PIR.");
  for (int t = DelayPirSeconds; t >= 0; t--) {
    Serial.print("id=");
    Serial.print(SLAVE_ID);
    Serial.print(" - Attesa riscaldamento PIR: ");
    Serial.print(t);
    Serial.println(" s");

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13_tf);
    char buf[10];
    snprintf(buf, sizeof(buf), "PIR %d", t);
    u8g2.drawStr(0, u8g2.getAscent(), buf);
    u8g2.sendBuffer();

    blinkWarmupDouble();
  }
  Serial.println("Countdown riscaldamento PIR completato.");
}

bool readPirAlarmNow() {
  int raw = digitalRead(InputPir);
  return PirReverse ? (raw == LOW) : (raw == HIGH);
}

void updatePirLatch() {
  if (readPirAlarmNow()) {
    if (!pirAlarmLatched) {
      Serial.print("id=");
      Serial.print(SLAVE_ID);
      Serial.println(" - ALLARME PIR rilevato -> flag alzato.");
      setDiagLed(true);
    }
    pirAlarmLatched = true;
  }
}

// ===================== WIFI =====================

void configureRangeOptimizedSta() {
  bool sleepOk = WiFi.setSleepMode(WIFI_NONE_SLEEP);
  bool phyOk = WiFi.setPhyMode(RangeOptimizedPhyMode);
  WiFi.setOutputPower(RangeOptimizedTxPowerDbm);

  Serial.print("id=");
  Serial.print(SLAVE_ID);
  Serial.println(" - Radio ottimizzata per massima distanza compatibile.");
  Serial.print("id=");
  Serial.print(SLAVE_ID);
  Serial.print(" - PHY 802.11b -> ");
  Serial.println(phyOk ? "OK" : "ERRORE");
  Serial.print("id=");
  Serial.print(SLAVE_ID);
  Serial.print(" - WIFI_NONE_SLEEP -> ");
  Serial.println(sleepOk ? "OK" : "ERRORE");
  Serial.print("id=");
  Serial.print(SLAVE_ID);
  Serial.print(" - TX power -> ");
  Serial.print(RangeOptimizedTxPowerDbm, 1);
  Serial.println(" dBm");
}

void setupWifi() {
  WiFi.mode(WIFI_STA);
  configureRangeOptimizedSta();
  WiFi.config(STATIC_IP, GATEWAY, SUBNET);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("id=");
  Serial.print(SLAVE_ID);
  Serial.print(" - Connessione a ");
  Serial.print(WIFI_SSID);
  Serial.print(" con IP ");
  Serial.println(STATIC_IP);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    feedWatchdog();
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("id=");
    Serial.print(SLAVE_ID);
    Serial.print(" - Connesso, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("id=");
    Serial.print(SLAVE_ID);
    Serial.println(" - Non connesso, ritento in background nel loop.");
  }

  tcpServer.begin();
}

void checkWifiReconnect() {
  static unsigned long lastAttempt = 0;
  if (WiFi.status() != WL_CONNECTED && millis() - lastAttempt > 5000) {
    lastAttempt = millis();
    Serial.print("id=");
    Serial.print(SLAVE_ID);
    Serial.println(" - WiFi disconnesso, tento riconnessione...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
}

// ===================== RISPOSTA AL MASTER =====================

long getRadioDbForDebug() {
  return (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : -99;
}

void respondToMaster(Print& dest, bool isRealMasterPoll) {
  bool statusToSend = pirAlarmLatched;
  uint32_t currentMasterPollCount = masterPollCount;
  long currentRadioDb = getRadioDbForDebug();

  if (isRealMasterPoll) {
    currentMasterPollCount = ++masterPollCount;
    currentRadioDb = getRadioDbForDebug();
  }

  dest.print("Id=");
  dest.println(SLAVE_ID);
  dest.println(statusToSend ? "STA113" : "STA100");
  dest.print("CALL=");
  dest.print(currentMasterPollCount);
  dest.print(" RADIO=");
  dest.print(currentRadioDb);
  dest.println("db");

  Serial.print("id=");
  Serial.print(SLAVE_ID);
  if (isRealMasterPoll) {
    Serial.print(" - Interrogazione master #");
    Serial.print(currentMasterPollCount);
    Serial.print(", stato trasmesso: ");
  } else {
    Serial.print(" - Chiamata seriale simulata, contatore master=");
    Serial.print(currentMasterPollCount);
    Serial.print(", stato trasmesso: ");
  }
  Serial.print(statusToSend ? "STA113" : "STA100");
  Serial.print(", info: C=");
  Serial.print(currentMasterPollCount);
  Serial.print(" R=");
  Serial.print(currentRadioDb);
  Serial.println("db");

  // RIMOZIONE: pirAlarmLatched non viene più resettato qui!

  toggleLed();
  feedWatchdog();
}

// ===================== RESET FLAG ALLARME =====================

void resetAlarmFlag() {
  pirAlarmLatched = false;
  setDiagLed(false);
  Serial.print("id=");
  Serial.print(SLAVE_ID);
  Serial.println(" - Ricevuto codice 155 -> FLAG ALLARME RESETTATO!");
}


// ===================== TCP =====================

// Timeout TCP (ms) — evita blocchi su client lenti/persi
const unsigned long TcpPostResponseMs = 500;  // attesa "155" dopo risposta se allarme

// Legge una riga fino a \n con timeout assoluto (no readStringUntil bloccante).
bool readLineWithTimeout(WiFiClient& client, String& out, unsigned long timeoutMs) {
  out = "";
  const unsigned long deadline = millis() + timeoutMs;
  while (millis() < deadline && client.connected()) {
    feedWatchdog();
    while (client.available()) {
      char c = (char)client.read();
      if (c == '\n') {
        out.trim();
        return true;
      }
      if (c != '\r' && out.length() < 64) {
        out += c;
      }
    }
    delay(1);
  }
  out.trim();
  return out.length() > 0;
}

void handleTcpClients() {
  WiFiClient client = tcpServer.available();
  if (!client) return;

  client.setNoDelay(true);
  client.setTimeout(TcpPostResponseMs);

  const bool sentAlarm = pirAlarmLatched;

  respondToMaster(client, true);
  client.flush();
  feedWatchdog();

  if (sentAlarm) {
    String command;
    if (readLineWithTimeout(client, command, TcpPostResponseMs)) {
      Serial.print("id=");
      Serial.print(SLAVE_ID);
      Serial.print(" - comando TCP ricevuto: [");
      Serial.print(command);
      Serial.println("]");
      if (command.indexOf("155") != -1) {
        resetAlarmFlag();
      }
    } else {
      Serial.print("id=");
      Serial.print(SLAVE_ID);
      Serial.println(" - timeout attesa 155 (latch NON resettato).");
    }
  }

  client.stop();
}

// ===================== SERIALE ("?" = polling simulato, "155" = reset) =====================

void handleSerialCommand() {
  static String inputBuf = "";
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      inputBuf.trim();
      if (inputBuf == "?") {
        respondToMaster(Serial, false);
      } else if (inputBuf.indexOf("155") != -1) {
        resetAlarmFlag();
      }
      inputBuf = "";
    } else if (inputBuf.length() < 48) {
      inputBuf += c;
    }
  }
}



// ===================== DISPLAY DI STATO =====================

void updateStatusDisplay() {
  bool wifiConnected = (WiFi.status() == WL_CONNECTED);

  u8g2.setFont(u8g2_font_7x13_tf);
  int charH = u8g2.getMaxCharHeight();
  int ascent = u8g2.getAscent();
  int lineStep = charH + 1;

  char line1[16];
  snprintf(line1, sizeof(line1), "%d %s", SLAVE_ID, wifiConnected ? "CONN" : "NOCN");

  const char* line2 = pirAlarmLatched ? "AL" : "OK";

  u8g2.clearBuffer();
  u8g2.drawStr(0, ascent, line1);
  u8g2.drawStr(0, ascent + lineStep, line2);
  u8g2.sendBuffer();
}

// ===================== SETUP =====================

void setup() {
  Serial.begin(115200);
  delay(1500);
  printSourceFileName();
  initWatchdog();

  u8g2.begin();
  u8g2.setContrast(255);
  u8g2.setBusClock(400000);

  Serial.print("=== Reset slave id=");
  Serial.print(SLAVE_ID);
  Serial.println(" ===");

  pinMode(LED_BUILTIN_PIN, OUTPUT);
  setLed(false);

  pinMode(LED_BUILTIN_PIN, OUTPUT);
  setLed(false);

  pinMode(LED_DIAG_PIN, OUTPUT);
  setDiagLed(false);


  countdownPir();

  pirAlarmLatched = false;

  pinMode(InputPir, INPUT_PULLUP);

  setupWifi();

  Serial.print("id=");
  Serial.print(SLAVE_ID);
  Serial.println(" - Setup completato, avvio loop.");
}

// ===================== LOOP =====================

void loop() {
  feedWatchdog();
  checkWifiReconnect();
  updatePirLatch();
  handleTcpClients();
  handleSerialCommand();

  if (millis() - lastDisplayUpdate >= displayUpdateInterval) {
    lastDisplayUpdate = millis();
    updateStatusDisplay();
  }
}