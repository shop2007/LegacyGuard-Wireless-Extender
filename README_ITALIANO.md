#  LegacyGuard Wireless Extender (Master to 4 Slave PIR/Sensors)

Un sistema di estensione wireless per collegare (WiFi) ulteriori 4 sensori PIR oppure contatti n.o., ad una centrale cablata. 
Basato su architettura **Master-Slave** , tramite una rete Wi-Fi locale, consente ad una scheda centrale (Master) di monitorare lo stato di fino a 4 nodi periferici (Slave cablati localmente con sensori PIR ed alimentati localmente) e attivare un relè in caso di allarme rilevato.
E' utile per aggiungere sensori PIR a sistemi di allarme cablati di vecchia generazione, senza dover aggiungere 
cablaggi. 

---

![System Architecture](img/SystemArchitecture.jpg) 
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


