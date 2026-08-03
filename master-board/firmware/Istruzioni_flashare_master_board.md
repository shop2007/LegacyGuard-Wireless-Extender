**Passi consigliati per chi parte da zero:**
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