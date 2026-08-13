# PIR Slave — ESP8266 / Wemos D1 mini (v100INV)

Complete documentation of the **Slave** node of the Master-Slave PIR alarm/repeater system.

This document describes hardware, pinout, Arduino IDE configuration, communication protocol, LED behaviour, alarm logic and test procedures. It is intended for anyone who needs to program, test or maintain an already working Slave.


---

## 1. Overview

| Item | Value |
|------|--------|
| Microcontroller | ESP8266 (Wemos D1 mini / LOLIN D1 mini clone) |
| Role | Detects PIR alarm, stores the state (latch), replies to the Master via TCP |
| Network | Wi-Fi 802.11b, star topology, static IP |
| TCP Port | **4210** |
| Watchdog | ESP8266 software, 8-second timeout |
| Reference firmware | `4SLAVE8266_pir_LR_wdog_100INV.ino` |

**Operating principle**

1. At start-up it runs a 90-second countdown for PIR warm-up, to avoid false alarms in case of 220 V power loss (can be skipped by inserting the jumper on D6).
2. When the PIR detects movement it raises a **latched** alarm flag and turns on the diagnostic LED.
3. The Master periodically polls the Slave via TCP (port 4210).
4. The Slave replies with `STA100` (normal) or `STA113` (latched alarm), plus call counter and RSSI.
5. The alarm flag is **not** cleared by a simple poll: it is cleared **only** when receiving the command `155` / `STA155` (via TCP from the Master or via serial).

---

## 2. Hardware and Pinout

### 2.1 Board

- **Wemos D1 mini** (or LOLIN clone) based on ESP-12E/F.
- Typical power supply: 5 V via USB or stabilised 3.3 V.
- On-board built-in LED (active LOW).

### 2.2 Pins used

| Wemos Pin | GPIO | Function | Notes |
|-----------|------|----------|------|
| D5 | GPIO14 | PIR Input | INPUT_PULLUP |
| D6 | GPIO12 | SkipDelayPir | INPUT_PULLUP — to GND skips the 90 s warm-up |
| D7 | GPIO13 | Alarm Diagnostic LED | Active HIGH output (LED anode → resistor → D7, cathode → GND) |
| LED_BUILTIN | GPIO2 | Activity LED | Active LOW |

### 2.3 Recommended connections

**PIR sensor**
- PIR output (or contact/opto) connected to **D5** through an optoisolator as per the electrical schematic.
- Logic configurable with the `PirReverse` constant in the `.ino`:
  - `false` → alarm when D5 reads HIGH
  - `true` → alarm when D5 reads LOW (useful with NC contact in series with optoisolator)

**Alarm diagnostic LED**
- D7 → 330–1 kΩ resistor → LED anode; LED cathode → GND

**Skip warm-up jumper**
- D6 → GND (only at power-on, to skip the 90 s PIR warm-up during board testing)

---

## 3. Arduino IDE Configuration

### 3.1 Board and settings

1. Install the **ESP8266** core (Board Manager → search for "esp8266" by ESP8266 Community).
2. Select the board:
   - **LOLIN(WEMOS) D1 mini (clone)**
   or
   - **NodeMCU 1.0 (ESP-12E Module)** (works equally well on many clones)
3. Recommended settings:
   - **CPU Frequency**: 80 MHz
   - **Flash Size**: 4 MB (FS: 2 MB OTA:~1019 KB) or similar
   - **Upload Speed**: 921600 (or 115200 if unstable)
   - No "USB CDC on boot" option required (serial always goes through the CH340).

### 3.2 Required libraries

- **ESP8266WiFi** (already included in the core)
- **Wire** (already included in the core)

### 3.3 Configuration file `Slave_ID.h`

This is the only file that needs to be modified to create 4 different Slaves (`#define SLAVE_ID 2` // or 3, 4, 5).

```cpp
// Slave_ID.h
// board WEMOS LOLIN D1 clone
#ifndef SLAVE_ID_H
#define SLAVE_ID_H

#define SLAVE_ID 3 // value from 2 to 5

// Compile-time check: if out of range, compilation fails
static_assert(SLAVE_ID >= 2 && SLAVE_ID <= 5, "SLAVE_ID must be between 2 and 5");

#endif

// ===================== WIFI CONFIGURATION =====================

const char* WIFI_SSID = "WINDTRE-6EF994";
const char* WIFI_PASS = "7DHH383KKK33-2a";

IPAddress AP_IP(192, 168, 4, 1);
IPAddress GATEWAY(192, 168, 4, 1);
IPAddress SUBNET(255, 255, 255, 0);

// The last octet is taken directly from SLAVE_ID:
// there is no need (and you must not) manually modify the IP.
IPAddress STATIC_IP(192, 168, 4, SLAVE_ID);

const uint16_t TCP_PORT = 4210;
WiFiServer tcpServer(TCP_PORT);
const float RangeOptimizedTxPowerDbm = 20.5f;
const WiFiPhyMode_t RangeOptimizedPhyMode = WIFI_PHY_MODE_11B;
```

The IP address is calculated automatically:

| SLAVE_ID | Automatically assigned IP |
|----------|-------------------------------|
| 2 | 192.168.4.2 |
| 3 | 192.168.4.3 |
| 4 | 192.168.4.4 |
| 5 | 192.168.4.5 |

The Master always stays on `192.168.4.1`.

It is not necessary (and not recommended) to manually modify `STATIC_IP`: the constructor `IPAddress(192, 168, 4, SLAVE_ID)` guarantees alignment between the logical ID and the network address. If a value outside 2–5 is entered, compilation fails thanks to `static_assert`.

Wi-Fi SSID and password must be set in the same file (`WIFI_SSID` / `WIFI_PASS`).

### 3.4 Upload

1. Connect the Wemos via USB.
2. Select the correct COM port.
3. Upload the sketch.
4. Open the Serial Monitor at 115200 baud.

At start-up you will see:
- name of the `.ino` file
- watchdog activation message
- status of the SkipDelayPir pin
- PIR countdown (if not skipped)
- Wi-Fi connection attempt
- "Setup completed, starting loop."

---

## 4. Detailed Operating Logic

### 4.1 Start-up and PIR warm-up

- If D6 is not connected to GND → 90-second countdown.
- Every second the LED_BUILTIN performs a double blink (see LED section).
- If D6 is connected to GND → the delay is skipped (useful during testing).

### 4.2 Alarm detection (Latch)

- The function `updatePirLatch()` continuously reads D5.
- On the first alarm detection:
  - `pirAlarmLatched = true`
  - Diagnostic LED (D7) turns on
  - Serial confirmation message
- The flag remains high even if the PIR returns to rest.
- Only the reset command clears it.

### 4.3 Reply to the Master (TCP)

When a TCP connection arrives on port 4210 the Slave:

1. Immediately sends:
   ```
   Id=<SLAVE_ID>
   STA100          or STA113
   CALL=<n> RADIO=<rssi>db
   ```
2. If the status sent was an alarm (STA113), it waits up to 500 ms for a possible reset command (`155`).
3. If it receives `155` → it executes `resetAlarmFlag()` (flag = false + LED D7 off).
4. Closes the connection.

> **Important**: a simple poll does not reset the alarm. Only the code `155` does so.

### 4.4 Serial commands (debug)

With the Serial Monitor open:

| Command | Action |
|---------|--------|
| `?` | Simulates a Master call (prints the same response that would go via TCP) |
| `155` | Immediate reset of the alarm flag and turning off of the diagnostic LED (or any string containing 155) |

### 4.5 Watchdog

- Enabled in setup with an 8-second timeout (`ESP.wdtEnable(WDTO_8S)`).
- Fed (`ESP.wdtFeed()`) at all critical points (loop, long delays, TCP reading, etc.).
- In case of code lock-up the board restarts autonomously.

### 4.6 Wi-Fi reconnection

If the connection is lost, every 5 seconds the Slave attempts to reconnect in the background without blocking the loop.

---

## 5. LEDs — Complete Behaviour

### 5.1 LED_BUILTIN (GPIO2, active LOW)

| Situation | Behaviour |
|------------|----------------|
| Reply to Master (TCP or `?`) | Toggle of state (1 state change) |
| PIR warm-up countdown | Double blink every second: ON 120 ms → OFF 120 ms → ON 120 ms → restore previous state + 640 ms pause |
| 3 or 4 blink patterns | Not implemented |

### 5.2 Alarm Diagnostic LED (D7, active HIGH)

| Event | Action |
|--------|--------|
| PIR detects alarm | Turns on and stays on |
| Master polling | Stays on (Master polling does not turn it off) |
| Reception of command `155` / `STA155` | Turns off |

---

## 6. Communication Protocol (summary)

**Master → Slave request**

Simple TCP connection to port 4210 (no mandatory payload).

**Slave → Master response**

```
Id=3
STA100
CALL=15 RADIO=-67db
```

or

```
Id=3
STA113
CALL=16 RADIO=-65db
```

**Alarm reset (Master → Slave, only if the Slave has just sent STA113)**

```
155
```
(or any line containing the substring `155`)

---

## 7. Testing an Already Working Slave

1. Power the board and open the Serial Monitor at 115200.
2. Verify that the setup message and the static IP appear (192.168.4.x where x = SLAVE_ID).
3. Send `?` from the serial → it must reply with STA100 and show the CALL counter.
4. Activate the PIR (or simulate the signal on D5) → the LED on D7 must turn on and the serial must report the alarm.
5. Send `?` again → it must reply STA113.
6. Send `155` → the D7 LED turns off and the status returns to normal.
7. Verify that the Master (if present) correctly sees the status and is able to send the reset.

**Quick test without Master**

Everything can be tested with only serial + PIR + LED, without needing the network.

---

## 8. Operating Notes and Tips

- To maximise radio range the Slave forces 802.11b mode and TX power at 20.5 dBm, with radio sleep disabled.
- The alarm latch is deliberate: it avoids losing a short event if the Master does not poll exactly at that moment.
- The timeout for waiting for the 155 command after an alarm response is only 500 ms: the Master must be sufficiently responsive.
- To create a new Slave it is sufficient to modify `SLAVE_ID` in `Slave_ID.h` and re-upload the sketch. The IP updates automatically.
- The D6→GND jumper is very useful in the laboratory; in the final installation it must be removed.
- The TCP port used by the system is 4210 (not 80).

---

## 9. Related Files

| File | Content |
|------|-----------|
| `4SLAVE8266_pir_LR_wdog_100INV.ino` | Complete Slave firmware |
| `Slave_ID.h` | Identifier, Wi-Fi credentials, automatic static IP, TCP port 4210 |
| `README.md` | Operational and programming documentation |
