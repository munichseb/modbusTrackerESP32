#include <WiFi.h>
#include <WebServer.h>
#include <map>
#include <queue>

const char* ssid = "Dein_WLAN_SSID";
const char* password = "Dein_WLAN_Passwort";

WebServer server(80);

#define RX_PIN 16
#define TX_PIN 17
#define RE_DE_PIN 4

// Datenstruktur zum Speichern der Register und ihrer Werte
std::map<uint16_t, uint16_t> modbusRegisters;

// Struktur für gespeicherte Anfragen
struct ModbusRequest {
  uint8_t address;
  uint8_t functionCode;
  uint16_t startRegister;
  uint16_t quantity;
};

// Warteschlange für Anfragen
std::queue<ModbusRequest> requestQueue;

uint8_t modbusBuffer[256];
int modbusBufferIndex = 0;

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
  // MODBUS-Daten einlesen
  while (Serial2.available()) {
    uint8_t byte = Serial2.read();
    modbusBuffer[modbusBufferIndex++] = byte;

    // Wenn genügend Daten für einen Frame empfangen wurden, Frame verarbeiten
    if (modbusBufferIndex >= 5) {
      if (isCompleteFrame(modbusBuffer, modbusBufferIndex)) {
        parseModbusFrame(modbusBuffer, modbusBufferIndex);
        modbusBufferIndex = 0; // Buffer zurücksetzen
      } else if (modbusBufferIndex >= sizeof(modbusBuffer)) {
        // Buffer übergelaufen, zurücksetzen
        modbusBufferIndex = 0;
      }
    }
  }

  server.handleClient();
}

bool isCompleteFrame(uint8_t* buffer, int len) {
  // Überprüfen, ob der Frame vollständig ist anhand der Länge und des Funktioncodes
  uint8_t functionCode = buffer[1];

  if (functionCode == 0x03 || functionCode == 0x04) {
    // Anfrage oder Antwort
    if (len >= 8 && (buffer[1] == 0x03 || buffer[1] == 0x04)) {
      // Möglicherweise Anfrage
      return true;
    } else if (len >= 5) {
      uint8_t byteCount = buffer[2];
      int expectedLen = 3 + byteCount + 2; // Adresse + Funktion + ByteCount + Daten + CRC
      if (len >= expectedLen) {
        // Antwort ist vollständig
        return true;
      }
    }
  }
  return false;
}

void parseModbusFrame(uint8_t* frame, int len) {
  if (len < 5) return;

  uint8_t address = frame[0];
  uint8_t functionCode = frame[1];

  if (functionCode == 0x03 || functionCode == 0x04) {
    // Prüfen, ob es eine Anfrage oder eine Antwort ist
    if (len == 8) {
      // Anfrage-Frame
      uint16_t startRegister = (frame[2] << 8) | frame[3];
      uint16_t quantity = (frame[4] << 8) | frame[5];

      // Anfrage speichern
      ModbusRequest request;
      request.address = address;
      request.functionCode = functionCode;
      request.startRegister = startRegister;
      request.quantity = quantity;
      requestQueue.push(request);

    } else {
      // Antwort-Frame
      if (!requestQueue.empty()) {
        ModbusRequest request = requestQueue.front();
        requestQueue.pop();

        uint8_t byteCount = frame[2];
        int numRegisters = byteCount / 2;
        for (int i = 0; i < numRegisters; i++) {
          uint16_t value = (frame[3 + i * 2] << 8) | frame[4 + i * 2];
          uint16_t registerAddress = request.startRegister + i;
          modbusRegisters[registerAddress] = value;
        }
      }
    }
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>MODBUS Register</title></head><body>";
  html += "<h1>Gefundene MODBUS Register und Werte</h1><table border='1'><tr><th>Register-Adresse</th><th>Letzter Wert</th></tr>";

  for (auto const& reg : modbusRegisters) {
    html += "<tr><td>" + String(reg.first) + "</td><td>" + String(reg.second) + "</td></tr>";
  }

  html += "</table></body></html>";
  server.send(200, "text/html", html);
}
