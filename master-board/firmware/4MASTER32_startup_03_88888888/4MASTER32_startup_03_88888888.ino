/* 
 * SKETCH MASTER - OTA & SERIAL ONLY
 * Modalità semplificata: Monitor Seriale + OTA sempre attivo
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <esp_task_wdt.h>
#include "Master_ID.h"

// ===================== CONFIGURAZIONE SERVER OTA =====================

WebServer otaServer(OTA_HTTP_PORT);

bool otaUpdateInProgress = false;
bool otaUpdateSessionOpen = false;
bool otaUpdateFinishedOk = false;
bool otaUploadAttempted = false;
bool otaRebootScheduled = false;

unsigned long otaRebootAt = 0;
unsigned long lastSerialPrint = 0;
const unsigned long serialPrintInterval = 2000; // Stampa ogni 2 secondi

size_t otaBytesReceived = 0;
size_t otaExpectedBytes = 0;
int otaLastProgressPercent = -1;
String otaLastStatus = "OTA sempre attiva.";

// ===================== UTILITIES & WATCHDOG =====================

void feedWatchdog() {
  esp_task_wdt_reset();
}

void initTaskWatchdog() {
  esp_err_t status = esp_task_wdt_status(NULL);
  if (status == ESP_OK) return;

  if (status == ESP_ERR_NOT_FOUND) {
    esp_task_wdt_add(NULL);
    return;
  }

  if (status == ESP_ERR_INVALID_STATE) {
    esp_task_wdt_config_t wdtConfig = {
      .timeout_ms = 8000,
      .idle_core_mask = 0,
      .trigger_panic = true
    };
    esp_task_wdt_init(&wdtConfig);
    esp_task_wdt_add(NULL);
  }
}

// ===================== PAGINA WEB OTA =====================

String buildOtaPageHtml() {
  String html;
  html.reserve(1200);

  html += F("<!doctype html><html lang='it'><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Master OTA</title><style>");
  html += F("body{font-family:Arial,sans-serif;max-width:600px;margin:20px auto;padding:20px;background:#f5f5f5;color:#202020;}");
  html += F(".card{background:#fff;border-radius:12px;padding:20px;box-shadow:0 3px 14px rgba(0,0,0,.08);margin-bottom:16px;}");
  html += F("code{background:#f0f0f0;padding:2px 6px;border-radius:6px;}input[type=file]{display:block;margin:12px 0;}button{padding:10px 16px;border:none;border-radius:8px;background:#005bbb;color:#fff;font-weight:700;cursor:pointer;}");
  html += F("</style></head><body>");
  html += F("<div class='card'><h1>Aggiornamento OTA Master</h1>");
  html += F("<p>SoftAP IP: <code>");
  html += WiFi.softAPIP().toString();
  html += F("</code></p><p>Stato: <strong>");
  html += otaLastStatus;
  html += F("</strong></p></div>");

  html += F("<div class='card'><h2>Carica Firmware (.bin)</h2>");
  html += F("<form method='POST' action='/update' enctype='multipart/form-data'>");
  html += F("<input type='file' name='update' accept='.bin' required>");
  html += F("<button type='submit'>Carica Firmware</button></form>");
  html += F("</div></body></html>");
  
  return html;
}

bool ensureOtaHttpAuth() {
  if (otaServer.authenticate(OTA_HTTP_USER, DIAG_TCP_PASSWORD)) {
    return true;
  }
  otaServer.requestAuthentication(BASIC_AUTH, "master-ota");
  return false;
}

void handleOtaRoot() {
  if (!ensureOtaHttpAuth()) return;
  otaServer.send(200, "text/html", buildOtaPageHtml());
}

void handleOtaUpdatePost() {
  if (!ensureOtaHttpAuth()) return;

  bool success = otaUploadAttempted && otaUpdateFinishedOk;
  otaServer.sendHeader("Connection", "close");

  if (success) {
    otaLastStatus = "OTA completata. Riavvio programmato.";
    otaServer.send(200, "text/plain", "Aggiornamento completato. Il master si riavvia.");
    otaRebootScheduled = true;
    otaRebootAt = millis() + 1500;
    Serial.println("OTA: aggiornamento completato. Riavvio in corso...");
  } else {
    otaLastStatus = "OTA fallita. Riprovare.";
    otaServer.send(500, "text/plain", "Aggiornamento fallito.");
    Serial.println("OTA: aggiornamento fallito.");
  }

  otaUpdateInProgress = false;
  otaUpdateSessionOpen = false;
  otaUploadAttempted = false;
  otaUpdateFinishedOk = false;
  otaLastProgressPercent = -1;
  otaBytesReceived = 0;
  otaExpectedBytes = 0;
}

void handleOtaUpdateUpload() {
  HTTPUpload& upload = otaServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    otaUpdateInProgress = true;
    otaRebootScheduled = false;
    otaUpdateSessionOpen = false;
    otaUploadAttempted = true;
    otaUpdateFinishedOk = false;
    otaBytesReceived = 0;
    otaExpectedBytes = upload.totalSize;
    otaLastProgressPercent = -1;
    otaLastStatus = "Upload OTA in corso...";

    Serial.println();
    Serial.println("OTA: Upload firmware avviato.");
    Serial.print("OTA: File = ");
    Serial.println(upload.filename);

    // Disabilita il task watchdog per tutta la durata dell'upload/scrittura flash:
    // handleClient() può restare bloccato a lungo durante la ricezione dei chunk
    // e la scrittura su flash, con rischio di reset a metà OTA.
    esp_task_wdt_delete(NULL);

    if (Update.begin(UPDATE_SIZE_UNKNOWN)) {
      otaUpdateSessionOpen = true;
    } else {
      otaLastStatus = "Errore inizializzazione OTA.";
      Update.printError(Serial);
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_WRITE) {
    if (!otaUpdateSessionOpen) return;

    size_t written = Update.write(upload.buf, upload.currentSize);
    if (written != upload.currentSize) {
      otaLastStatus = "Errore scrittura OTA.";
      Update.printError(Serial);
      return;
    }

    otaBytesReceived += upload.currentSize;
    if (otaExpectedBytes > 0) {
      int progress = (int)((otaBytesReceived * 100U) / otaExpectedBytes);
      if (progress != otaLastProgressPercent && (progress == 100 || progress / 10 != otaLastProgressPercent / 10)) {
        otaLastProgressPercent = progress;
        Serial.print("OTA: progresso ");
        Serial.print(progress);
        Serial.println('%');
      }
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_END) {
    if (!otaUpdateSessionOpen) {
      otaUpdateInProgress = false;
      return;
    }

    if (Update.end(true)) {
      otaUpdateFinishedOk = true;
      otaLastStatus = "Upload OTA completato con successo.";
      Serial.print("OTA: Upload completato. Byte ricevuti: ");
      Serial.println(upload.totalSize);
    } else {
      otaUpdateFinishedOk = false;
      otaLastStatus = "Errore finalizzazione OTA.";
      Update.printError(Serial);
    }
    initTaskWatchdog(); // riattiva il watchdog: upload terminato (successo o errore)
    return;
  }

  if (upload.status == UPLOAD_FILE_ABORTED) {
    otaUpdateInProgress = false;
    otaUpdateSessionOpen = false;
    otaUpdateFinishedOk = false;
    otaLastStatus = "Upload OTA interrotto.";
    Update.abort();
    Serial.println("OTA: Upload interrotto.");
    initTaskWatchdog(); // riattiva il watchdog anche in caso di interruzione
  }
}

void setupOtaServer() {
  otaServer.on("/", HTTP_GET, handleOtaRoot);
  otaServer.on("/update", HTTP_GET, handleOtaRoot);
  otaServer.on("/update", HTTP_POST, handleOtaUpdatePost, handleOtaUpdateUpload);
  otaServer.onNotFound([]() {
    if (!ensureOtaHttpAuth()) return;
    otaServer.send(404, "text/plain", "Risorsa non trovata. Usa / oppure /update.");
  });
  otaServer.begin();
}

void setupWifiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, GATEWAY, SUBNET);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);

  setupOtaServer();

  Serial.println("=========================================");
  Serial.print("SoftAP avviato - SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("IP per aggiornamento OTA: http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("=========================================");
}

// ===================== SETUP =====================

void setup() {
  Serial.begin(115200);
  delay(1000);

  initTaskWatchdog();
  setupWifiAP();
}

// ===================== LOOP =====================

void loop() {
  feedWatchdog();
  otaServer.handleClient();

  // Gestione riavvio dopo caricamento OTA completato
  if (otaRebootScheduled && millis() >= otaRebootAt) {
    otaRebootScheduled = false;
    Serial.println("Riavvio dispositivo in corso...");
    Serial.flush();
    delay(100);
    ESP.restart();
  }

  // Stampa periodica su Monitor Seriale se l'OTA non è in caricamento attivo
  if (!otaUpdateInProgress && (millis() - lastSerialPrint >= serialPrintInterval)) {
    lastSerialPrint = millis();
    Serial.println("eseguire aggiornamento OTA");
  }
}