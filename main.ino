#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// =====================================================
// NETZWERK EINSTELLUNGEN
// =====================================================
const char* ssid = "";
const char* password = ";

// Telegram Bot Konfiguration
const char* botToken = "";
const char* chatId = "";

// =====================================================
// MQTT EINSTELLUNGEN 
// =====================================================
const char* mqtt_server = "";  // IP address mqtt server
const int mqtt_port     = 1883; // Port MQTT
const char* mqtt_user   = "; //  username 
const char* mqtt_pass   = "";   // password

// =====================================================
// Topics
// =====================================================

const char* topic_status =
  "farm/sensor/wasserstand2";

const char* topic_avail =
  "farm/sensor/verfuegbarkeit2";

const char* topic_config =
  "homeassistant/binary_sensor/farm_wasser2/config";

// =====================================================
// SENSOR
// =====================================================

const int SENSOR_PIN = 4; // GPIO 4

// Bei INPUT_PULLUP:
// LOW  = Sensor aktiv
// HIGH = Sensor nicht aktiv
const int SENSOR_WASSER_SIGNAL = LOW;

// =====================================================
// ZUSTÄNDE
// =====================================================

bool letzterZustand = false;
bool ersterMesswert = true;

bool temporaererZustand = false;

unsigned long statusWechselZeit = 0;
unsigned long letzteMqttSchnittzeit = 0;

const unsigned long DEBOUNCE_DELAY = 3000;

// =====================================================
// NETZWERKOBJEKTE
// =====================================================

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// =====================================================
// URL-CODIERUNG FÜR TELEGRAM
// =====================================================

String urlEncode(String text) {
  String encodedString = "";

  for (size_t i = 0; i < text.length(); i++) {
    char c = text.charAt(i);

    if (c == ' ') {
      encodedString += "%20";
    }
    else if (
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      c == '-' ||
      c == '_' ||
      c == '.' ||
      c == '~'
    ) {
      encodedString += c;
    }
    else {
      uint8_t code = static_cast<uint8_t>(c);
      char buffer[4];

      sprintf(buffer, "%%%02X", code);
      encodedString += buffer;
    }
  }

  return encodedString;
}

// =====================================================
// TELEGRAM-NACHRICHT SENDEN
// =====================================================

void sendTelegramMessage(String text) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TELEGRAM] Kein WLAN vorhanden.");
    return;
  }

  Serial.println("[TELEGRAM] Sende Nachricht: " + text);

  WiFiClientSecure client;
  client.setInsecure();

  if (client.connect("api.telegram.org", 443)) {
    String url =
      "/bot" +
      String(botToken) +
      "/sendMessage?chat_id=" +
      String(chatId) +
      "&text=" +
      urlEncode(text);

    client.print(
      String("GET ") + url + " HTTP/1.1\r\n" +
      "Host: api.telegram.org\r\n" +
      "User-Agent: ESP32-Wassertank-2\r\n" +
      "Connection: close\r\n\r\n"
    );

    Serial.println("[TELEGRAM] Nachricht gesendet.");
    client.stop();
  }
  else {
    Serial.println("[TELEGRAM] Verbindung zu Telegram fehlgeschlagen.");
  }
}

// =====================================================
// HOME-ASSISTANT MQTT DISCOVERY
// =====================================================

void sendeHomeAssistantDiscovery() {
  Serial.println("[MQTT] Sende Home-Assistant-Discovery für Sensor 2...");

  String payload =
    "{"
      "\"name\":\"C Farm Wassertank 2\","
      "\"state_topic\":\"farm/sensor/wasserstand2\","
      "\"availability_topic\":\"farm/sensor/verfuegbarkeit2\","
      "\"payload_on\":\"Fluessigkeit erkannt\","
      "\"payload_off\":\"Keine Fluessigkeit vorhanden\","
      "\"availability\":\"online\","
      "\"not_available\":\"offline\","
      "\"unique_id\":\"farm_wasser_sensor_002\","
      "\"device_class\":\"moisture\","
      "\"icon\":\"mdi:water-percent\","
      "\"device\":{"
        "\"identifiers\":[\"esp32_farm_controller_2\"],"
        "\"name\":\"Grow-Farm Controller 2\","
        "\"model\":\"ESP32 DevKit\","
        "\"manufacturer\":\"Espressif\""
      "}"
    "}";

  bool erfolgreich = mqttClient.publish(
    topic_config,
    payload.c_str(),
    true
  );

  if (erfolgreich) {
    Serial.println("[MQTT] Discovery erfolgreich gesendet.");
  }
  else {
    Serial.println("[MQTT] Discovery konnte nicht gesendet werden.");
  }
}

// =====================================================
// WLAN VERBINDEN
// =====================================================

void mitWLANVerbinden() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("[WLAN] Verbinde");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int versuche = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    versuche < 20
  ) {
    delay(500);
    Serial.print(".");
    versuche++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WLAN] Verbindung erfolgreich.");
    Serial.print("[WLAN] IP-Adresse: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("[WLAN] Verbindung fehlgeschlagen.");
  }
}

// =====================================================
// MQTT VERBINDUNG PRÜFEN
// =====================================================

void mqttVerbindungPruefen() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (mqttClient.connected()) {
    return;
  }

  if (
    millis() - letzteMqttSchnittzeit < 10000 &&
    letzteMqttSchnittzeit != 0
  ) {
    return;
  }

  letzteMqttSchnittzeit = millis();

  Serial.print("[MQTT] Verbinde mit MQTT-Broker...");

  // Eindeutige Client-ID für den zweiten ESP32
  String clientId =
    "ESP32-FarmClient-2-" +
    String((uint32_t)ESP.getEfuseMac(), HEX);

  bool verbunden = mqttClient.connect(
    clientId.c_str(),
    mqtt_user,
    mqtt_pass,
    topic_avail,
    1,
    true,
    "offline"
  );

  if (verbunden) {
    Serial.println(" erfolgreich.");

    mqttClient.publish(
      topic_avail,
      "online",
      true
    );

    sendeHomeAssistantDiscovery();

    // Nach neuer MQTT-Verbindung aktuellen Zustand erneut senden
    ersterMesswert = true;
  }
  else {
    Serial.print(" fehlgeschlagen, Fehlercode: ");
    Serial.println(mqttClient.state());
  }
}

// =====================================================
// SENSORSTATUS VERÖFFENTLICHEN
// =====================================================

void sensorStatusSenden(bool sensorAktiv) {
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] Nicht verbunden, Status nicht gesendet.");
    return;
  }

  if (sensorAktiv) {
    Serial.println("[STATUS] Sensor 2: Flüssigkeit erkannt");

    mqttClient.publish(
      topic_status,
      "Fluessigkeit erkannt",
      true
    );

    sendTelegramMessage(
      "Sensor 2: Entwarnung. Fluessigkeit erkannt."
    );
  }
  else {
    Serial.println("[STATUS] Sensor 2: Keine Flüssigkeit vorhanden");

    mqttClient.publish(
      topic_status,
      "Keine Fluessigkeit vorhanden",
      true
    );

    sendTelegramMessage(
      "Sensor 2: Achtung! Keine Fluessigkeit vorhanden."
    );
  }
}

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("======================================");
  Serial.println(" ESP32 FARM WASSERTANK 2");
  Serial.println("======================================");

  pinMode(
    SENSOR_PIN,
    INPUT_PULLUP
  );

  mitWLANVerbinden();

  mqttClient.setServer(
    mqtt_server,
    mqtt_port
  );

  mqttClient.setBufferSize(768);

  sendTelegramMessage(
    "ESP32 Wassertank 2 gestartet. Ueberwachung aktiv."
  );
}

// =====================================================
// LOOP
// =====================================================

void loop() {
  mitWLANVerbinden();

  mqttClient.loop();
  mqttVerbindungPruefen();

  bool aktuellerMesswert =
    digitalRead(SENSOR_PIN) == SENSOR_WASSER_SIGNAL;

  // Änderung des Rohwerts erkannt
  if (aktuellerMesswert != temporaererZustand) {
    temporaererZustand = aktuellerMesswert;
    statusWechselZeit = millis();

    Serial.println("[SENSOR] Änderung erkannt, Entprellung läuft...");
  }

  // Zustand muss mindestens 3 Sekunden stabil sein
  if (
    millis() - statusWechselZeit >= DEBOUNCE_DELAY
  ) {
    bool statusHatSichGeaendert =
      aktuellerMesswert != letzterZustand;

    if (
      statusHatSichGeaendert ||
      ersterMesswert
    ) {
      letzterZustand = aktuellerMesswert;
      ersterMesswert = false;

      sensorStatusSenden(
        aktuellerMesswert
      );
    }
  }

  delay(50);
}
