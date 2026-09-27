# Füllstandssensor-ESP32

Aktuell für ein (erweiterbar) kapazitiven Sensor geschrieben.

Das Skript erfordert die Eingabe MQTT Server Login Daten, sowie Telegram Chat und WiFi Login. => main.ino Zeile 9 - 22

Bauteile
- ESP32 Modul
- Füllstandssensor (mit oder ohne LED Anzeige geschalten/nicht)

Zum Löten
- Vin = 5V Sensor Spannungsversorgung
- GND = GND Sensor Spannungsversorgung
- GPIO 7 = Sensor Signal
