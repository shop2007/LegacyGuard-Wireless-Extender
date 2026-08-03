// Slave_ID.h
//board WEMOS LIOLIN D1 clone
#ifndef SLAVE_ID_H
#define SLAVE_ID_H

#define SLAVE_ID 3 // valore da 2 a 5 (max 2 slave sulla rete)

// Controllo a tempo di compilazione: se fuori range, la compilazione si blocca
// con un messaggio di errore chiaro, invece di generare un IP sbagliato in silenzio.
static_assert(SLAVE_ID >= 2 && SLAVE_ID <= 5, "SLAVE_ID deve essere compreso tra 2 e 5");

#endif

// ===================== CONFIGURAZIONE WIFI =====================

const char* WIFI_SSID = "WINDTRE-6EF994";
const char* WIFI_PASS = "7DHH383KKK33-2a";

IPAddress AP_IP(192, 168, 4, 1);
IPAddress GATEWAY(192, 168, 4, 1);
IPAddress SUBNET(255, 255, 255, 0);

// L'ultimo ottetto e' preso direttamente da SLAVE_ID: niente rischio
// di disallineamento tra ID logico e IP di rete.
IPAddress STATIC_IP(192, 168, 4, SLAVE_ID);

const uint16_t TCP_PORT = 4210;
WiFiServer tcpServer(TCP_PORT);
const float RangeOptimizedTxPowerDbm = 20.5f;
const WiFiPhyMode_t RangeOptimizedPhyMode = WIFI_PHY_MODE_11B;