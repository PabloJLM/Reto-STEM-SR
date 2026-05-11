// opta_webserver.ino  —  Servidor web simple para Arduino Opta
// Sirve PAGE_HTML con PAGE_CSS inyectado desde webpage.h
// Compatible con Opta WiFi (Advanced) y Opta Lite/RS485 (Ethernet)
//
// Librerias necesarias: ninguna extra (WiFi.h incluida en el BSP de Opta)
// Board: Arduino Opta  (Boards Manager: Arduino Mbed OS Opta Boards)

#include <Arduino.h>
#include <WiFi.h>
#include "webpage.h"

// ── Credenciales WiFi ──────────────────────────────────────────────────────
const char* SSID     = "Pjlm";
const char* PASSWORD = "yelyah1012";

// ── IP estatica opcional (comentar para usar DHCP) ────────────────────────
// IPAddress local_ip(192, 168, 1, 120);
// IPAddress gateway(192, 168, 1, 1);
// IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);

// Estado de los relays (D0..D3 = PI_6, PI_5, PI_7, PI_4)
const int RELAY_PINS[4] = {PI_6, PI_5, PI_7, PI_4};
bool relay_state[4]     = {false, false, false, false};

// Estado de entradas analogicas I1..I2 (PA0_C, PC2_C)
const int AIN_PINS[2] = {A0, A1};

// ── Setup ──────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Relays como salida, apagados al inicio
  for (int i = 0; i < 4; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
  }

  // LED de estado (azul = conectando, verde = listo)
  pinMode(LEDB, OUTPUT);
  pinMode(LED_USER, OUTPUT);
  digitalWrite(LEDB, HIGH);

  // Conexion WiFi
  Serial.print("Conectando a "); Serial.println(SSID);
  // WiFi.config(local_ip, gateway, subnet);  // descomentar para IP estatica
  WiFi.begin(SSID, PASSWORD);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 30) {
    delay(500);
    Serial.print(".");
    intentos++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFallo WiFi — revisar credenciales");
    digitalWrite(LEDR, HIGH);
    while (1);
  }

  digitalWrite(LEDB, LOW);
  digitalWrite(LED_USER, HIGH);
  Serial.print("\nIP: "); Serial.println(WiFi.localIP());
  server.begin();
}

// ── Loop ───────────────────────────────────────────────────────────────────
void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  // Leer primera linea de la peticion HTTP
  String req = client.readStringUntil('\r');
  client.readStringUntil('\n');  // saltar CRLF

  // Consumir el resto del header
  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;  // linea vacia = fin de headers
  }

  // ── Routing ───────────────────────────────────────────────────────────────
  if (req.startsWith("GET / ")) {
    // Servir pagina principal
    serve_page(client);

  } else if (req.startsWith("GET /status ")) {
    // JSON con estado actual
    serve_status(client);

  } else if (req.startsWith("POST /relay")) {
    // Leer body del POST
    String body = "";
    while (client.available()) body += (char)client.read();
    handle_relay(client, body);

  } else {
    // 404
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Not found");
  }

  client.stop();
}

// ── Funciones de respuesta ────────────────────────────────────────────────

void serve_page(WiFiClient& client) {
  // Construir HTML con CSS inyectado
  String page = String(PAGE_HTML);
  page.replace("<!-- CSS_PLACEHOLDER -->", String(PAGE_CSS));

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(page.length());
  client.println();
  client.print(page);
}

void serve_status(WiFiClient& client) {
  // Leer entradas analogicas (0..65535 en Opta Mbed)
  float ain0 = analogRead(AIN_PINS[0]) * (10.0f / 65535.0f);  // 0..10V
  float ain1 = analogRead(AIN_PINS[1]) * (10.0f / 65535.0f);

  String json = "{";
  json += "\"relays\":[";
  for (int i = 0; i < 4; i++) {
    json += relay_state[i] ? "true" : "false";
    if (i < 3) json += ",";
  }
  json += "],";
  json += "\"ain0\":" + String(ain0, 2) + ",";
  json += "\"ain1\":" + String(ain1, 2);
  json += "}";

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.print("Content-Length: "); client.println(json.length());
  client.println();
  client.print(json);
}

void handle_relay(WiFiClient& client, String body) {
  // Parseo simple de "relay=N&val=V"
  int r_idx = parse_param_int(body, "relay");
  int val   = parse_param_int(body, "val");

  if (r_idx >= 0 && r_idx < 4) {
    relay_state[r_idx] = (val == 1);
    digitalWrite(RELAY_PINS[r_idx], relay_state[r_idx] ? HIGH : LOW);
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("OK");
}

int parse_param_int(String body, String key) {
  String search = key + "=";
  int idx = body.indexOf(search);
  if (idx < 0) return -1;
  return body.substring(idx + search.length()).toInt();
}
