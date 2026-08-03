# Specifiche Generali di Sistema — LegacyGuard Wireless Extender 

Questo documento descrive nel dettaglio l'architettura, le componenti, i protocolli di comunicazione e le modalità operative del sistema di collegamento wireless per una centrale antifurto legacy (wired)

---

## 1. Architettura di Rete e Comunicazione[cite: 2]

* **Tipologia Rete**: Wi-Fi Standard 802.11b (configurato per massimizzare la portata e la stabilità del segnale senza sleep radio).[cite: 2]
* **Topologia**: Stella (Master al centro, Slave periferici).[cite: 2]
* **Indirizzamento IP**:[cite: 2]
  * **Master**: `192.168.4.1`[cite: 2]
  * **Slave ID 2**: `192.168.4.2`[cite: 2]
  * **Slave ID 3**: `192.168.4.3`[cite: 2]
  * **Slave ID 4**: `192.168.4.4`[cite: 2]
  * **Slave ID 5**: `192.168.4.5`[cite: 2]
* **Porta del Server TCP Slave**: `80` (utilizzata dal Master per interrogare lo stato degli slave).[cite: 1, 2]
* **Potenza di Trasmissione TX (Slave)**: 20.5 dBm.[cite: 2]

---

## 2. Modulo Master (`4MASTER32_displ_pir_wdog_70`)[cite: 2]

Il nodo Master (basato su ESP32) coordina la rete, interroga periodicamente gli slave e gestisce la logica degli allarmi.[cite: 2]

### 2.1 Logica di Allarme e Relè[cite: 2]
* **Stati rilevati dallo Slave**:[cite: 2]
  * `STA100`: Slave presente, nessun allarme.[cite: 2]
  * `STA113`: Slave presente, allarme rilevato (sensore PIR attivato).[cite: 2]
* **Comportamento Relè**: Se almeno uno slave risponde con `STA113`, l'uscita a relè viene attivata. Il relè resta eccitato per un tempo minimo di **2 secondi**, anche se la condizione di allarme rientra prima.[cite: 2]
* **Mappatura Conservativa**: Un nodo visto anche solo una volta viene memorizzato come "Presente". Per essere dichiarato "Assente", deve fallire multiple risposte consecutive al polling.[cite: 2]

### 2.2 Display OLED Master[cite: 2]
La matrice del display mostra lo stato dei 4 nodi in forma sintetica:[cite: 2]
* `A2`: Slave 2 in Allarme.[cite: 2]
* `N3`: Slave 3 Normale / Senza allarme.[cite: 2]
* `..`: Slave Non rilevato / Assente.[cite: 2]

### 2.3 Indicatori LED Master[cite: 2]
* **LED di Bordo**: Alterna stato ad ogni ciclo completo di polling (indicatore di attività).[cite: 2]
* **LED Ausiliario (GPIO 9)**: Si illumina durante le fasi di scan e mappatura degli slave.[cite: 2]

### 2.4 Ingressi di Servizio (Ponticelli Master)[cite: 2]
* **GPIO 4 ➔ GND**: Salta il ritardo di riscaldamento iniziale di 90 secondi.[cite: 2]
* **GPIO 10 ➔ GND**: Forza la rimappatura dinamica degli slave ad ogni ciclo.[cite: 2]

### 2.5 Console Diagnostica (Seriale & Telnet)[cite: 2]
L'accesso alla console di amministrazione avviene tramite **Seriale USB (115200 baud)** oppure via **Telnet**:[cite: 2]
* **Accesso Telnet**: `telnet 192.168.4.1 2323`[cite: 2]
* **Credenziali Console**: User: `admin` | Password: `Pautax2006`[cite: 2]
* **Menu Diagnostico (Comando `m` o `M`)**:[cite: 2]
  * `0`: Reset hardware della scheda.[cite: 2]
  * `1`: Stampa il nome del file sorgente firmware.[cite: 2]
  * `2`: Stampa statistiche dettagliate (allarmi, chiamate OK, chiamate FALLITE per ogni slave).[cite: 2]
  * `3`: Abilita/Disabilita diagnostica estesa (tracing funzioni).[cite: 2]
  * `4`: Avvia scan continuo di presenza/assenza (uscita con `q`).[cite: 2]
  * `5`: Attiva la modalità aggiornamento OTA via Web.[cite: 2]
  * `6`: Stampa Indirizzo MAC.[cite: 2]
  * `99`: Uscita dal menu.[cite: 2]

### 2.6 Aggiornamento Firmware Web OTA[cite: 2]
1. Aprire la sessione Telnet ed entrare nel menu diagnostico (`m`).[cite: 2]
2. Selezionare l'opzione `5` per abilitare l'OTA (il Master entra in modalità manutenzione).[cite: 2]
3. Aprire il browser all'indirizzo `http://192.168.4.1/update` (HTTP, non HTTPS).[cite: 2]
4. Inserire le credenziali OTA (`admin` / `Pautax2006`).[cite: 2]
5. Caricare il file `.bin` compilato e attendere il riavvio automatico.[cite: 2]
p.s. maggiori dettagli sulla programmazione e OTA sono disponibili al paragrafo 8. (Riprodurre il progetto: cosa serve)
---

## 3. Modulo Slave PIR (`ESP8266 / Wemos D1 mini`)[cite: 1, 2]

Ogni nodo Slave acquisisce il segnale dal sensore PIR e risponde alle chiamate TCP del Master.[cite: 2]

### 3.1 Funzionalità Principali[cite: 2]
* **Rilevamento Allarme Memorizzato (Latch)**: Quando il PIR rileva un'intrusione, il flag di allarme viene alzato e memorizzato (latched)[cite: 1]. **Importante**: il semplice polling da parte del Master NON azzera lo stato di allarme[cite: 1].
* **Reset Allarme via Comando `STA155`**: Lo stato di allarme memorizzato (e il relativo LED diagnostico) si azzera **esclusivamente** quando lo Slave riceve il comando specifico con codice `155` / `STA155` (inviato via TCP o Seriale)[cite: 1].
* **Watchdog Software**: Timer a 8 secondi con auto-reset della scheda in caso di blocco del codice[cite: 2].
* **Risposta TCP**: Invia lo stato (`STA100` in assenza di allarme, `STA113` in presenza di allarme latched), il numero progressivo di chiamata (`CALL`) e la potenza del segnale Wi-Fi (`RADIO` / `RSSI`)[cite: 1, 2].

### 3.2 Display OLED Slave[cite: 2]
* **Riga 1**: ID Slave (`SLAVE_ID`) e stato connessione Wi-Fi (`CONN` / `NOCN`)[cite: 2].
* **Riga 2**: Stato Allarme (`OK` / `AL`)[cite: 2].
* *Fase di avvio*: Mostra il countdown dei secondi rimanenti al riscaldamento del sensore PIR[cite: 2].

### 3.3 Codici e Funzionalità LED Slave[cite: 1, 2]
* **LED_BUILTIN** (attivo LOW):
  * **Toggle (1 “blink” di stato)**: Inverte il proprio stato ad ogni risposta trasmessa al Master (sia via TCP sia via comando seriale `?`). Serve da indicatore di attività/polling.
  * **Doppio blink (2 blink) durante riscaldamento PIR**: Nella fase di countdown di 90 s (se non saltata tramite D6→GND) esegue una sequenza fissa di **2 lampeggi rapidi** per ogni secondo di attesa:
    * ON 120 ms → OFF 120 ms → ON 120 ms → ripristino stato precedente + pausa 640 ms.
    * La sequenza è ripetuta per ogni secondo del countdown (funzione `blinkWarmupDouble()`).
  * Non sono implementati pattern a 3 o 4 blink; il LED_BUILTIN usa esclusivamente il toggle di attività e il doppio blink di warmup.
* **LED Diagnostico Allarme (`LED_DIAG_PIN` su D7)**:
  * **Si accende (HIGH)**: Non appena il PIR rileva un allarme e alza il flag interno[cite: 1].
  * **Rimane ACCESO**: Anche durante i normali cicli di polling/interrogazione, finché l'allarme resta memorizzato[cite: 1].
  * **Si spegne (LOW)**: Soltanto dopo la ricezione esplicita del comando di reset (inviata dal MASTER) `STA155`[cite: 1].

### 3.4 Ingressi Hardware e Interfacce Slave[cite: 1, 2]
* **Pin D6 (`SkipDelayPir`)**: Pull-up interno. Se ponticellato a GND all'avvio, salta il countdown di 90s del PIR (utilizzato per debug/test)[cite: 1, 2].
* **Pin D5 (`InputPir`)**: Ingresso del sensore PIR con pull-up interno. Supporta la configurazione con optocoppiatore/contatto normalmente chiuso (`PirReverse = false` o `true` in base alla logica del sensore)[cite: 1, 2].
* **Pin D7 (`LED_DIAG_PIN`)**: Uscita attiva alta collegata all'anodo del LED diagnostico allarme (con opportuna resistenza di limitazione verso GND)[cite: 1].
* **Interfaccia Seriale USB**:
  * Inviando `?` simula una chiamata di polling fornendo la risposta di stato[cite: 1].
  * Inviando `155` o `STA155` forza il reset immediato dell'allarme latched e lo spegnimento del LED diagnostico[cite: 1].# LegacyGuard Wireless Extender 

** Scheda Master di rete per monitoraggio 4 schede slave connesse a sensori tipo PIR, Doppia tecnologia, interruttori magnetici normalmente chiusi.**

La scheda **master**, basata su **ESP32-C3**, gestisce una rete locale WiFi proprietaria (Access Point) composta da un master e fino a **4 slave remoti** (espandibile a 8 con modifica firmware). Il master interroga periodicamente gli slave, ne acquisisce lo stato operativo (presenza / allarme) e pilota un'uscita a relè in caso di allarme. Lo stato dei 4 slave viene mostrato in tempo reale su un display OLED.

> Questo documento descrive l'hardware, il protocollo, il firmware e la procedura per riprodurre o modificare il progetto. 

---

## 1. Panoramica

| | |
|---|---|
| **MCU** | ESP32-C3 (API core: `WiFi.h`, `WebServer.h`, `esp_task_wdt.h`, `Update.h`) |
| **Ruolo** | Master di rete: polling di fino a 4 slave, gestione allarme, display, OTA |
| **Rete** | Access Point WiFi proprio (non serve avere il router WiFi acceso) |
| **Output** | Relè di allarme, display OLED, 2 LED di stato |
| **Aggiornamento** | OTA via interfaccia web, protetta da autenticazione |
| **Diagnostica** | Console seriale (115200 baud) o Telnet, con menu a comandi |

---

## 2. Architettura di rete

- Il master crea un proprio **Access Point WiFi** (SoftAP), a cui slave e postazione di diagnostica si collegano.
- Rete gestita internamente su un range statico (IP master fisso: `192.168.4.1`).
- Ogni slave ha un ID (`SLAVE_ID`) compreso, secondo la numerazione del progetto, tra 2 e 5 (il master ha ID 1, fisso, così master e slave non collidono sulla stessa rete).
- Ogni slave, quando interrogato, trasmette al master uno stato sintetico ricavato da un codice interno:
  - stato (display) **A** (allarme) ↔ codice reale `STA113`
  - stato (display) **N** (normale) ↔ codice reale `STA100`
  - stato (display) **..** ↔ slave assente / non rilevato

---

## 3. Logica di funzionamento del master

- Il master interroga (poll) ciclicamente tutti gli slave configurati.
- **Se almeno uno slave risulta in allarme**, il relè di uscita si attiva.
- **Isteresi minima**: una volta attivato, il relè resta attivo per **almeno 2 secondi**, anche se l'allarme rientra prima.
- **Rilevamento presenza slave (mappatura conservativa)**, pensato per reti radio non ottimali (distanza, segnale debole):
  - in fase di mappatura, uno slave visto anche una sola volta viene considerato presente;
  - in polling normale, uno slave viene dichiarato assente solo dopo **più mancate risposte consecutive** (non alla prima).

---

## 4. Interfaccia utente sulla scheda

### 4.1 Display OLED
Mostra in forma compatta lo stato dei 4 slave, due caratteri per slave:

| Codice | Significato |
|---|---|
| `A2` | slave 2 in allarme |
| `N3` | slave 3 presente, nessun allarme |
| `..` | slave non presente / non rilevato |

### 4.2 LED
- **LED di bordo (built-in)**: normale funzionamento del master, cambia stato a ogni ciclo completo di polling (utile per verificare "a colpo d'occhio" che il firmware non sia bloccato).
- **LED ausiliario (GPIO 9)**: acceso durante la mappatura/scan degli slave, spento a fine operazione.

### 4.3 Ponticelli di servizio (jumper verso GND)
| Pin  CHIP ESP32 C3  | Funzione |
|---|---|
| GPIO 4 → GND    | salta il ritardo iniziale di avvio (90 s) |
| GPIO 10 → GND   | forza la rimappatura completa degli slave a ogni ciclo, solo per debug |

---

## 5. Accesso diagnostico

Il master espone una console diagnostica raggiungibile in due modi:

- **Seriale USB**, 115200 baud
- **Telnet**, sulla rete WiFi del master, porta di default `2323`

Comando di connessione tipico da lanciare da console (finestra CMD):
```
telnet 192.168.4.1 2323
```

L'accesso richiede username/password di console. Per aprire il menu diagnostico, dalla sessione (seriale o Telnet) inviare il carattere **`m`** .


### Menu diagnostico — comandi disponibili

| Comando | Funzione |
|---|---|
| `0` | Reset hardware della scheda |
| `1` | Visualizza nome file sorgente del firmware in esecuzione |
| `2` | Statistiche per slave: n° allarmi, letture OK, letture fallite (`ALRM`, `OK`, `NON_OK`) utile per valutare la bontà della connessione WiFi a lungo termine |
| `3` | Abilita/disabilita diagnostica estesa (tracing dettagliato ingresso/uscita funzioni per lo sviluppatore) |
| `4` | Scan continuo slave presenti/assenti (termina con `q`) |
| `5` | Abilita/disabilita modalità OTA via web |
| `6` | Stampa MAC address della scheda |
| `99` | Esce dal menu |

Esempio di output del comando `2`: (n. allarmi, letture presenza in rete ok, letture presenza in rete fallite)
```
Slave2 ALRM=3 OK=6403 NON_OK=2
Slave3 ALRM=0 OK=0    NON_OK=6405
Slave4 ALRM=0 OK=0    NON_OK=6405
Slave5 ALRM=0 OK=0    NON_OK=6405
```

---

## 6. Aggiornamento firmware via OTA

Il firmware include un web server OTA (`WebServer` + libreria `Update.h`) sempre disponibile sull'IP del master, protetto da autenticazione HTTP Basic.

**Procedura completa:**
1. Scaricare il file (`.bin`) aggiornato .
2. Salvarlo in una cartella nota .
3. Scollegare il PC dalla rete del router e collegarlo alla rete WiFi `88888888` pass `88888888`` .
4. Aprire una sessione Telnet verso `192.168.4.1:2323`.  (Da prompt CMD scrivere "telnet 192.168.4.1 2323")
5. Autenticarsi con user 'admin' e password Pautax2006.
6. Inviare `m` per aprire il menu diagnostico.
7. Inviare `5` per attivare la modalità OTA.   ??????? verificare procedura
8. Aprire nel browser `http://192.168.4.1/update` (**http**, non https).
9. Inserire le credenziali OTA (username admin, password Pautax2006).
10. Sulla pagina web con sfoglia, cercare su disco il file `.bin`.
11. Attendere il completamento dell'upload.
12. Attendere il riavvio automatico della scheda.
13. Verificare il corretto funzionamento del nuovo firmware.

Durante l'OTA il master entra in **modalità manutenzione** (il polling/le funzioni normali sono sospese).

---

## 7. Struttura firmware e file di progetto

```
4MASTER32/
├── 4MASTER32_startup_03_88888888.ino      # sketch di primo avvio / recovery (OTA + seriale)
├── 4MASTER32_displ_pir_wdog_100.ino.bin   # firmware vero e proprio  
```

### 7.1 Sketch di primo avvio (`4MASTER32_startup_02_88888888.ino`)
Lo sketch *startup* è una **variante minimale di primo avvio / recovery**: monta solo l'Access Point WiFi, il web server OTA e il watchdog hardware, stampando periodicamente sulla seriale un messaggio di invito ad aggiornare il firmware. **Non contiene** la logica di polling degli slave, display e relè descritta nel manuale: è pensato per flashare facilmenete una scheda "vergine" (o recuperare una scheda bloccata) e portarla via OTA al firmware applicativo completo, senza dover ricollegarla via USB.

Elementi tecnici principali:
- Watchdog hardware task (`esp_task_wdt`), timeout 8 s, con `trigger_panic` attivo — se il loop si blocca, la scheda si resetta da sola.
- Web server OTA (`WebServer` su porta 80) con autenticazione HTTP Basic.
- Endpoint `/update` per upload del binario, gestito in streaming (`UPLOAD_FILE_START` / `WRITE` / `END` / `ABORTED`) con log di avanzamento su seriale.
- Riavvio automatico (`ESP.restart()`) 1,5 s dopo il completamento dell'OTA.

### 7.2 `Master_ID.h`
Definisce l'identità del master sulla rete e i parametri di rete/console:
- `MASTER_ID` — fisso a `1` (gli slave usano 2–9, max 8 slave in rete).
- SSID / password dell'Access Point del master.
- IP statico, gateway, subnet dell'AP.
- Porta TCP applicativa, porta e password della console Telnet diagnostica.
- Porta, utente e password del web server OTA.
- `NUM_SLAVES`, `FIRST_SLAVE_ID` — numero e primo ID degli slave attesi in rete.
- Un promemoria empirico sulle misure di livello del segnale radio (es. a 30 cm circa −33 dB, a 7 m circa −85/−92 dB), utile per capire i limiti della portata dell'AP in fase di progettazione dell'impianto.

NOTA BENE: questo file non è accessibile, ma contenuto nel file .bin .

### 7.3 `Scelta dei parametri SSID e PASSWORD WiFi`
- Il nome della rete locale (SSID) e della password WiFi desiderate devono essere richieste all'indirizzo register@realmeteo.com , unitamente al MAC Address della scheda ESP32-C3, leggibile dal menù di servizio con il comando 6.
- Questi parametri (SSID e PASSWORD) dovranno essere riportati identici sul file Slave_ID.h prima di programmare i moduli SLAVE (WEMOS LIOLIN D1), editando queste due linee
- const char* WIFI_SSID = "NOME-DESIDERATO-RETE-WIFI";
- const char* WIFI_PASS = "PASSWORD-WIFI";

### 7.4 `Nota tecnica: partitions.csv`
Tabella di partizionamento flash ESP32-c3, schema **OTA dual-app** (necessario per gli aggiornamenti OTA sicuri, con rollback):

| Nome | Tipo | Sub-tipo | Offset | Dimensione |
|---|---|---|---|---|
| `nvs` | data | nvs | `0x9000` | 20 KB |
| `otadata` | data | ota | `0xe000` | 8 KB |
| `app0` | app | ota_0 | `0x10000` | 1900 KB |
| `app1` | app | ota_1 | — | 1900 KB |
| `coredump` | data | coredump | — | 128 KB |

Due slot applicativi (`app0`/`app1`) da 1900 KB ciascuno permettono all'OTA di scrivere il nuovo firmware nello slot inattivo e passare a quello solo dopo un upload riuscito, mantenendo la possibilità di tornare al firmware precedente in caso di problemi. La partizione `coredump` conserva un dump in caso di crash per facilitare il debug.

---

## 8. Riprodurre il progetto: cosa serve

**Hardware:**
- Scheda ESP32‐C3 con display OLED da 0,42 pollici WiFi Bluetooth
- PCB
-Relè
- Sensori PIR e schede slave D1 dedicate (una per ogni punto da monitorare)
- 2 LED (bordo + ausiliario su GPIO 9)
- 2 ponticelli/pulsanti verso GND su GPIO 4 e GPIO 10

![ESP32-C3](img/ESP32-C3.png) 


**Toolchain:**
- Arduino IDE con core ESP32 installato ed impostazione del file "http://arduino.esp8266.com/stable/package_esp8266com_index.json" nelle preferenze
- Librerie: `WiFi.h`, `WebServer.h`, `Update.h`, `esp_task_wdt.h` (incluse nel core ESP32, nessuna installazione aggiuntiva richiesta)


**Programmazione scheda MASTER:**
0. Scaricare tutti i files presenti nella cartella firmware, copiandoli in una cartella nota sul nostro pc.
0.1 Opzionale ma preferibile: Smontare la scheda ESP32 C3 dalla board e connetterla al pc tramite il cavetto USB
1. Su Arduino IDE2 , impostare quanto segue: 
1.1 File->Preferences-> Additional board manager url-> http://arduino.esp8266.com/stable/package_esp8266com_index.jsonze 
1.2 Board "ESP32‐C3 Dev Module"
1.3 Tools-> Erase all flash before upload ->Enabled
1.4 Tools-> USB CDC on boot -> Enabled 
2. Aprire lo sketch "4MASTER32_startup_02_88888888.ino" e caricarlo su una scheda ESP32C3 (meglio se con display OLED da 0,42 pollici).
2.1 Verificare da terminale seriale a 115200 baud il seguente messaggio:  =========================================
SoftAP avviato - SSID: 88888888
IP per aggiornamento OTA: http://192.168.4.1
=========================================
eseguire aggiornamento OTA

3. Verificare con il PC che tra le altre reti WiFi ci sia anche la rete con SSID = **88888888** (si legge INTERNET NON DISPONIBILE). 
4. Scollegare il PC dalla rete wifi del vostro router e collegare la rete con SSID 88888888 usando password 88888888
5. Sul browser del pc digitare il seguente indirizzo (opzionalmente lasciare il monitor seriale attivo) http://192.168.4.1/update   (attenzione, non https) 
5.1 Verrà richiesto di autenticarsi: user 'admin' password 'Pautax2006' 
6. Con sfoglia/browse cercare il file 4MASTER32_displ_pir_wdog_100.bin (N.B. 100 è la versione, potreste anche trovare un upgrade con altro numero) .
6.1 Programmare la scheda con il pulsante 'Carica Firmware'
6.2 sul terminae seriale si legge:
OTA: Upload firmware avviato.
OTA: File = 4MASTER32_displ_pir_wdog_100.ino.bin
OTA: Upload completato. Byte ricevuti: 1072240
OTA: aggiornamento completato. Riavvio in corso...
eseguire aggiornamento OTA
Riavvio dispositivo in corso...
6.3 sul browser si legge: **Aggiornamento completato. Il master si riavvia.**

---

## 9. PCB

- I PCB per montare schede MASTER e SLAVE sono in sviluppo e verranno distribuiti a `prezzo di costo`, se sei interessato prenotati.