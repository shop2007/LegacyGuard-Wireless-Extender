// Slave_ID.h
#ifndef MASTER_ID_H
#define MASTER_ID_H

#define MASTER_ID 1 // valore 1 (diverso da 2 a 9 (max 8 slave sulla rete)

// Controllo a tempo di compilazione: se fuori range, la compilazione si blocca
// con un messaggio di errore chiaro, invece di generare un IP sbagliato in silenzio.
//static_assert(SLAVE_ID >= 2 && SLAVE_ID <= 9, "SLAVE_ID deve essere compreso tra 2 e 9");

#endif

//const char* WIFI_SSID = "WINDTRE-6EF994";
//const char* WIFI_PASS = "7DHH383KKK33-2a";
const char* WIFI_SSID = "88888888";
const char* WIFI_PASS = "88888888";

IPAddress AP_IP(192, 168, 4, 1);
IPAddress GATEWAY(192, 168, 4, 1);
IPAddress SUBNET(255, 255, 255, 0);

const uint16_t TCP_PORT = 4210;
const uint16_t DIAG_TCP_PORT = 2323;
const char* DIAG_TCP_PASSWORD = "Pautax2006";
const uint8_t RangeOptimizedApProtocol = WIFI_PROTOCOL_11B;
const uint16_t OTA_HTTP_PORT = 80;
const char* OTA_HTTP_USER = "admin";
const char* OTA_FIRMWARE_LABEL = "4MASTER32_displ_pir_wdog_70";
const char* OTA_BUILD_ID = __DATE__ " " __TIME__;

// ===================== SLAVE =====================

#define NUM_SLAVES 4
#define FIRST_SLAVE_ID 2



/*
Misure di livello radio
30cm = -33dB
7 metri = -85/92db
*/