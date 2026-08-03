# PIR Repeater System (Master-Slave Alarm System)

Un sistema di allarme / ripetitore di presenza distribuito basato su architettura **Master-Slave** in rete Wi-Fi locale. Il sistema consente a una scheda centrale (Master) di monitorare lo stato di fino a 4 nodi periferici (Slave con sensori PIR) e attivare un relè in caso di allarme rilevato.
E' utile per aggiungere sensori PIR a sistemi di allarme cablati di vecchia generazione, senza dover aggiungere 
cablaggi. 

---

## 📐 Architettura del Sistema

```text
               +----------------------------------+
               |        MASTER (ESP32)            |
               |       IP: 192.168.4.1            |
               |   Uscita Relè / Display OLED     |
               +-----------------+----------------+
                                 |
         +-----------------------+-----------------------+
         | (Wi-Fi 802.11b - Rete 192.168.4.x / Port 4210) |
         v                                               v
+------------------+                           +------------------+
| SLAVE 2 (ESP8266)|                           | SLAVE 3..5       |
| Sensor PIR       |  ... (fino a 4 slave) ... | (ESP8266)        |
| IP: 192.168.4.2  |                           | IP: 192.168.4.x  |
+------------------+                           +------------------+

![SystemArchitecture.jpg](img/SystemArchitecture.jpg) 
