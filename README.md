# Füllstandssensor-ESP32

Aktuell für ein (erweiterbar) kapazitiven Sensor geschrieben.

Wenn das Modul via WLAN verwendet wird kann es via ZigBee2MQTT Bridge im HomeAssistant integriert werden sowie im Telegram Messenger über den Modulzustand benachrichtigen.
Das Skript erfordert hierfür die Eingabe MQTT Server Login Daten, sowie Telegram Chat und WiFi Login. => main.ino Zeile 9 - 22

Bauteile
- ESP32 Modul
- Füllstandssensor (mit oder ohne LED Anzeige geschalten/nicht)

Zum Löten
- Vin = 5V Sensor Spannungsversorgung
- GND = GND Sensor Spannungsversorgung
- GPIO 7 = Sensor Signal
