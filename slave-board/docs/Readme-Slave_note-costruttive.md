# PIR Slave — ESP8266 / Wemos D1 mini (v100INV)

Documentazione completa del nodo **Slave** del sistema di allarme/ripetitore PIR Master-Slave.

Questo documento descrive hardware, pinout, configurazione Arduino IDE, protocollo di comunicazione, comportamento dei LED, logica di allarme e procedure di test. È pensato per chi deve programmare, collaudare o manutenere uno Slave già funzionante.


---

## 1. Panoramica

| Voce | Valore |
|------|--------|
| Microcontrollore | ESP8266 (Wemos D1 mini / LOLIN D1 mini clone) |
| Ruolo | Rileva allarme PIR, memorizza lo stato (latch), risponde al Master via TCP |
| Rete | Wi-Fi 802.11b, topologia a stella, IP statico |
| Porta TCP | **4210** |
| Watchdog | Software ESP8266, timeout 8 secondi |
| Firmware di riferimento | `4SLAVE8266_pir_LR_wdog_100INV.ino` |

**Principio di funzionamento**

1. All'avvio esegue  un countdown di 90 s per il riscaldamento del PIR, ad evitare falsi allarme in caso caduta rete 220V (escludibile inserendo il jumper su D6).
2. Quando il PIR rileva un movimento alza un flag di allarme **latched** e accende il LED diagnostico.
3. Il Master interroga periodicamente lo Slave via TCP (porta 4210).
4. Lo Slave risponde con `STA100` (normale) o `STA113` (allarme latched), più contatore chiamate e RSSI.
5. Il flag di allarme **non** viene azzerato dal semplice polling: si azzera **solo** ricevendo il comando `155` / `STA155` (via TCP dal Master o seriale).

---

## 2. Hardware e Pinout

### 2.1 Scheda

- **Wemos D1 mini** (o clone LOLIN) basata su ESP-12E/F.
- Alimentazione tipica: 5 V via USB oppure 3,3 V stabilizzato.
- LED built-in sulla scheda (attivo LOW).

### 2.2 Pin utilizzati

| Pin Wemos | GPIO | Funzione | Note |
|-----------|------|----------|------|
| D5 | GPIO14 | Input PIR | INPUT_PULLUP |
| D6 | GPIO12 | SkipDelayPir | INPUT_PULLUP — a GND salta i 90 s di warmup |
| D7 | GPIO13 | LED Diagnostico Allarme | Uscita attiva HIGH (anodo LED → resistore → D7, catodo → GND) |
| LED_BUILTIN | GPIO2 | LED attività | Attivo LOW |

### 2.3 Collegamenti consigliati

**Sensore PIR**
- Uscita del PIR (o contatto/opto) collegata a **D5** tramite optoisolatore come da schema elettrico.
- Logica configurabile con la costante `PirReverse` nel `.ino`:
  - `false` → allarme quando D5 legge HIGH
  - `true` → allarme quando D5 legge LOW (utile con contatto NC in serie a optoisolatore)

**LED diagnostico allarme**
- D7 → resistore 330–1 kΩ → anodo LED ; catodo LED → GND

**Ponticello Skip warmup**
- D6 → GND (solo all'accensione, per saltare i 90 s di riscaldamento PIR durante il collaudo board)

---

## 3. Configurazione Arduino IDE

### 3.1 Board e impostazioni

1. Installa il core **ESP8266** (Board Manager → cerca "esp8266" di ESP8266 Community).
2. Seleziona la board:
   - **LOLIN(WEMOS) D1 mini (clone)**
   oppure
   - **NodeMCU 1.0 (ESP-12E Module)** (funziona ugualmente su molti clone)
3. Impostazioni consigliate:
   - **CPU Frequency**: 80 MHz
   - **Flash Size**: 4 MB (FS: 2 MB OTA:~1019 KB) o analogo
   - **Upload Speed**: 921600 (o 115200 se instabile)
   - Nessuna opzione "USB CDC on boot" richiesta (la seriale passa sempre dal CH340).

### 3.2 Librerie necessarie

- **ESP8266WiFi** (già inclusa nel core)
- **Wire** (già inclusa nel core)

### 3.3 File di configurazione `Slave_ID.h`

Questo è l'unico file da modificare per creare 4 Slave diversi (`#define SLAVE_ID 2` // oppure 3, 4, 5).

```cpp
// Slave_ID.h
// board WEMOS LOLIN D1 clone
#ifndef SLAVE_ID_H
#define SLAVE_ID_H

#define SLAVE_ID 3 // valore da 2 a 5

// Controllo a tempo di compilazione: se fuori range, la compilazione si blocca
static_assert(SLAVE_ID >= 2 && SLAVE_ID <= 5, "SLAVE_ID deve essere compreso tra 2 e 5");

#endif

// ===================== CONFIGURAZIONE WIFI =====================

const char* WIFI_SSID = "WINDTRE-6EF994";
const char* WIFI_PASS = "7DHH383KKK33-2a";

IPAddress AP_IP(192, 168, 4, 1);
IPAddress GATEWAY(192, 168, 4, 1);
IPAddress SUBNET(255, 255, 255, 0);

// L'ultimo ottetto è preso direttamente da SLAVE_ID:
// non serve (e non bisogna) modificare manualmente l'IP.
IPAddress STATIC_IP(192, 168, 4, SLAVE_ID);

const uint16_t TCP_PORT = 4210;
WiFiServer tcpServer(TCP_PORT);
const float RangeOptimizedTxPowerDbm = 20.5f;
const WiFiPhyMode_t RangeOptimizedPhyMode = WIFI_PHY_MODE_11B;
```

L'indirizzo IP viene calcolato automaticamente:

| SLAVE_ID | IP assegnato automaticamente |
|----------|-------------------------------|
| 2 | 192.168.4.2 |
| 3 | 192.168.4.3 |
| 4 | 192.168.4.4 |
| 5 | 192.168.4.5 |

Il Master resta sempre su `192.168.4.1`.

Non è necessario (e non è consigliato) modificare a mano `STATIC_IP`: il costruttore `IPAddress(192, 168, 4, SLAVE_ID)` garantisce l'allineamento tra ID logico e indirizzo di rete. Se si inserisce un valore fuori da 2–5, la compilazione fallisce grazie a `static_assert`.

SSID e password Wi-Fi vanno impostati nello stesso file (`WIFI_SSID` / `WIFI_PASS`).

### 3.4 Upload

1. Collega la Wemos via USB.
2. Seleziona la porta COM corretta.
3. Carica lo sketch.
4. Apri il Monitor Seriale a 115200 baud.

All'avvio vedrai:
- nome del file `.ino`
- messaggio di attivazione watchdog
- stato del pin SkipDelayPir
- countdown PIR (se non saltato)
- tentativo di connessione Wi-Fi
- "Setup completato, avvio loop."

---

## 4. Logica di funzionamento dettagliata

### 4.1 Avvio e riscaldamento PIR

- Se D6 non è a GND → countdown di 90 secondi.
- Ogni secondo il LED_BUILTIN esegue un doppio blink (vedi sezione LED).
- Se D6 è a GND → il delay viene saltato (utile in fase di test).

### 4.2 Rilevamento allarme (Latch)

- La funzione `updatePirLatch()` legge continuamente D5.
- Alla prima rilevazione di allarme:
  - `pirAlarmLatched = true`
  - LED diagnostico (D7) acceso
  - messaggio seriale di conferma
- Il flag rimane alto anche se il PIR torna a riposo.
- Solo il comando di reset lo spegne.

### 4.3 Risposta al Master (TCP)

Quando arriva una connessione TCP sulla porta 4210 lo Slave:

1. Invia immediatamente:
   ```
   Id=<SLAVE_ID>
   STA100          oppure STA113
   CALL=<n> RADIO=<rssi>db
   ```
2. Se lo stato inviato era di allarme (STA113), attende fino a 500 ms un eventuale comando di reset (`155`).
3. Se riceve `155` → esegue `resetAlarmFlag()` (flag = false + LED D7 spento).
4. Chiude la connessione.

> **Importante**: il semplice polling non resetta l'allarme. Solo il codice `155` lo fa.

### 4.4 Comandi seriali (debug)

Con il Monitor Seriale aperto:

| Comando | Azione |
|---------|--------|
| `?` | Simula una chiamata del Master (stampa la stessa risposta che andrebbe via TCP) |
| `155` | Reset immediato del flag di allarme e spegnimento LED diagnostico (o qualsiasi stringa che contenga 155) |

### 4.5 Watchdog

- Abilitato in setup con timeout di 8 secondi (`ESP.wdtEnable(WDTO_8S)`).
- Alimentato (`ESP.wdtFeed()`) in tutti i punti critici (loop, delay lunghi, lettura TCP, ecc.).
- In caso di blocco del codice la scheda si riavvia autonomamente.

### 4.6 Riconnessione Wi-Fi

Se la connessione viene persa, ogni 5 secondi lo Slave tenta di riconnettersi in background senza bloccare il loop.

---

## 5. LED — Comportamento completo

### 5.1 LED_BUILTIN (GPIO2, attivo LOW)

| Situazione | Comportamento |
|------------|----------------|
| Risposta al Master (TCP o `?`) | Toggle dello stato (1 cambio di stato) |
| Countdown riscaldamento PIR | Doppio blink ogni secondo: ON 120 ms → OFF 120 ms → ON 120 ms → ripristino stato + pausa 640 ms |
| Pattern a 3 o 4 blink | Non implementati |

### 5.2 LED Diagnostico Allarme (D7, attivo HIGH)

| Evento | Azione |
|--------|--------|
| PIR rileva allarme | Si accende e resta acceso |
| Polling del Master | Resta acceso (il polling del Master non lo spegne) |
| Ricezione comando `155` / `STA155` | Si spegne |

---

## 6. Protocollo di comunicazione (riepilogo)

**Richiesta Master → Slave**

Semplice connessione TCP alla porta 4210 (nessun payload obbligatorio).

**Risposta Slave → Master**

```
Id=3
STA100
CALL=15 RADIO=-67db
```

oppure

```
Id=3
STA113
CALL=16 RADIO=-65db
```

**Reset allarme (Master → Slave, solo se lo Slave ha appena inviato STA113)**

```
155
```
(o qualsiasi riga contenente la sottostringa `155`)

---

## 7. Collaudo di uno Slave già funzionante

1. Alimentare la scheda e aprire il Monitor Seriale a 115200.
2. Verificare che compaia il messaggio di setup e l'IP statico (192.168.4.x dove x = SLAVE_ID).
3. Inviare `?` dalla seriale → deve rispondere con STA100 e mostrare il contatore CALL.
4. Attivare il PIR (o simulare il segnale su D5) → il LED su D7 deve accendersi e la seriale deve segnalare l'allarme.
5. Inviare di nuovo `?` → deve rispondere STA113.
6. Inviare `155` → il LED D7 si spegne e lo stato torna normale.
7. Verificare che il Master (se presente) veda correttamente lo stato e riesca a inviare il reset.

**Test rapido senza Master**

Tutto può essere collaudato solo con seriale + PIR + LED, senza necessità della rete.

---

## 8. Note operative e consigli

- Per massimizzare la portata radio lo Slave forza la modalità 802.11b e potenza TX a 20,5 dBm, con sleep radio disabilitato.
- Il latch di allarme è deliberato: evita di perdere un evento breve se il Master non interroga esattamente in quel momento.
- Il timeout di attesa del comando 155 dopo una risposta di allarme è di soli 500 ms: il Master deve essere abbastanza reattivo.
- Per creare un nuovo Slave è sufficiente modificare `SLAVE_ID` in `Slave_ID.h` e ricaricare lo sketch. L'IP si aggiorna da solo.
- Il ponticello D6→GND è utilissimo in laboratorio; in installazione definitiva va rimosso.
- La porta TCP usata dal sistema è 4210 (non 80).

---

## 9. File correlati

| File | Contenuto |
|------|-----------|
| `4SLAVE8266_pir_LR_wdog_100INV.ino` | Firmware completo Slave |
| `Slave_ID.h` | Identificativo, credenziali Wi-Fi, IP statico automatico, porta TCP 4210 |
| `README.md` | Documentazione operativa e di programmazione |
