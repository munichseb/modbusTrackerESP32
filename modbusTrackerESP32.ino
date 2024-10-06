#include <WiFi.h>
#include <WebServer.h>
#include <set>

const char* ssid = "Dein_WLAN_SSID";
const char* password = "Dein_WLAN_Passwort";

WebServer server(80);

#define RX_PIN 16
#define TX_PIN 17
#define RE_DE_PIN 4

std::set<uint16_t> modbusRegisters;

void setup() {
  Serial.begin(115200);
  Serial.println("Starte MODBUS Sniffer...");

  // RS485 Steuerpin initialisieren
  pinMode(RE_DE_PIN, OUTPUT);
  digitalWrite(RE_DE_PIN, LOW); // Empfangsmodus aktivieren

  // Serielle Schnittstelle für RS485 initialisieren
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  // WLAN verbinden
  WiFi.begin(ssid, password);
  Serial.print("Verbinde mit WLAN...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nVerbunden!");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());

  // Webserver-Routen definieren
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Webserver gestartet!");
}

void loop() {
  if (Serial2.available()) {
    uint8_t buffer[256];
    int len = Serial2.readBytes(buffer, sizeof(buffer));
    if (len > 0) {
      parseModbusFrame(buffer, len);
    }
  }
  server.handleClient();
}

void parseModbusFrame(uint8_t* frame, int len) {
  // Mindestlänge für MODBUS RTU ist 5 Bytes
  if (len < 5) return;

  uint8_t address = frame[0];
  uint8_t functionCode = frame[1];

  // Nur bestimmte Funktioncodes verarbeiten
  if (functionCode == 0x03 || functionCode == 0x04) {
    uint16_t startRegister = (frame[2] << 8) | frame[3];
    uint16_t quantity = (frame[4] << 8) | frame[5];

    // Registeradressen speichern
    for (uint16_t i = 0; i < quantity; i++) {
      modbusRegisters.insert(startRegister + i);
    }
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>MODBUS Register</title></head><body>";
  html += "<h1>Gefundene MODBUS Register</h1><ul>";

  for (auto reg : modbusRegisters) {
    html += "<li>Register-Adresse: " + String(reg) + "</li>";
  }

  html += "</ul></body></html>";
  server.send(200, "text/html", html);
}
