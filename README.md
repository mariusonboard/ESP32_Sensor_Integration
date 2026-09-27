# Füllstandssensor ESP32

Aktuell für ein (erweiterbar) kapazitiven Sensor geschrieben.

Wenn das Modul mit einem oder mehreren Sensoren erweitert wird kann es via ZigBee2MQTT im HomeAssistant integriert werden sowie im Telegram Messenger über Sensorzustände benachrichtigen.
Das Skript erfordert hierfür die Eingabe der MQTT Server Login Daten, sowie Telegram Chat ID und WiFi Credentials. => "main.ino" Zeile 9 - 22

Bauteile
- ESP32 Modul
- Füllstandssensor (mit oder ohne LED Anzeige geschalten/nicht)

Zum Löten
- Vin = 5V Sensor Spannungsversorgung
- GND = GND Sensor Spannungsversorgung
- GPIO 4 = Sensor Signal
