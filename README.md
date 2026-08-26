# LegacyGuard Wireless Extender (Master to 4 Slave PIR/Sensors)

A wireless extension system to connect (via WiFi) up to 4 additional PIR sensors or N.O. contacts to a wired alarm control panel.  
Based on a **Master-Slave** architecture over a local Wi-Fi network, it allows a central board (Master) to monitor the status of up to 4 peripheral nodes (Slaves, wired locally to PIR sensors and powered locally) and activate a relay in case an alarm is detected.  
It is useful for adding PIR sensors to older-generation wired alarm systems without the need for additional cabling.

---

![System Architecture](img/SystemArchitecture.jpg)  
---

## 📐 System Architecture

```text
               +----------------------------------+
               |        MASTER (ESP32)            |
               |       IP: 192.168.4.1            |
               |   Relay Output / OLED Display    |
               +-----------------+----------------+
                                 |
         +-----------------------+-----------------------+
         | (Wi-Fi 802.11b - Network 192.168.4.x / Port 4210) |
         v                                               v
+------------------+                           +------------------+
| SLAVE 2 (ESP8266)|                           | SLAVE 3..5       |
| PIR Sensor       |  ... (up to 4 slaves) ... | (ESP8266)        |
| IP: 192.168.4.2  |                           | IP: 192.168.4.x  |
+------------------+                           +------------------+
```

link
- [master board ](https://github.com/shop2007/LegacyGuard-Wireless-Extender/tree/main/master-board)
