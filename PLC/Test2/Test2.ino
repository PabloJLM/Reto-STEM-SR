/*
 * Arduino Opta - TCP/IP JSON Demo
 * LabVIEW HMI via WiFi
 *
 * Fix: eliminadas redeclaraciones de LED_D0..D3, BTN_USER, RELAY1..4
 *      que colisionaban con los #define del BSP mbed_opta
 */

#include <WiFi.h>
#include <Arduino.h>

const char* SSID     = "Pjlm";
const char* PASSWORD = "yelyah1012";
const uint16_t TCP_PORT = 8888;

// Entradas analogicas I1..I8
const int INPUTS[8] = {A0, A1, A2, A3, A4, A5, A6, A7};

// RELAY1..RELAY4, LED_D0..LED_D3, LED_USER, LEDR, BTN_USER
// ya estan definidos en pins_arduino.h del BSP -> NO redeclarar
const int RELAYS[4]      = {RELAY1, RELAY2, RELAY3, RELAY4};
const int STATUS_LEDS[4] = {LED_D0, LED_D1, LED_D2, LED_D3};

bool relay_state[4]  = {false, false, false, false};
bool led_status[4]   = {false, false, false, false};
bool led_user_state  = false;
bool led_r_state     = false;

WiFiServer server(TCP_PORT);
WiFiClient client;

void apply_relays() {
    for (int i = 0; i < 4; i++)
        digitalWrite(RELAYS[i], relay_state[i] ? HIGH : LOW);
}

void apply_leds() {
    for (int i = 0; i < 4; i++)
        digitalWrite(STATUS_LEDS[i], led_status[i] ? HIGH : LOW);
    digitalWrite(LED_USER, led_user_state ? HIGH : LOW);
    digitalWrite(LEDR,     led_r_state    ? HIGH : LOW);
}

String build_status_json() {
    String j = "{\"inputs_v\":[";
    for (int i = 0; i < 8; i++) {
        float v = analogRead(INPUTS[i]) * (10.0f / 4095.0f);
        j += String(v, 2);
        if (i < 7) j += ",";
    }
    j += "],\"inputs_raw\":[";
    for (int i = 0; i < 8; i++) {
        j += String(analogRead(INPUTS[i]));
        if (i < 7) j += ",";
    }
    j += "],\"relays\":[";
    for (int i = 0; i < 4; i++) {
        j += relay_state[i] ? "true" : "false";
        if (i < 3) j += ",";
    }
    j += "],\"leds_status\":[";
    for (int i = 0; i < 4; i++) {
        j += led_status[i] ? "true" : "false";
        if (i < 3) j += ",";
    }
    j += "],\"led_user\":";
    j += led_user_state ? "true" : "false";
    j += ",\"led_r\":";
    j += led_r_state ? "true" : "false";
    j += ",\"btn_user\":";
    j += (digitalRead(BTN_USER) == LOW) ? "true" : "false";
    j += "}";
    return j;
}

void process_command(const String& cmd, WiFiClient& c) {
    if (cmd == "GET_STATUS") {
        c.print(build_status_json() + "\n");
        return;
    }
    if (cmd.startsWith("SET_RELAY ")) {
        int n = cmd.charAt(10) - '0';
        int v = cmd.charAt(12) - '0';
        if (n >= 0 && n < 4) {
            relay_state[n] = (v != 0);
            apply_relays();
            c.print("{\"ok\":true,\"relay\":" + String(n) + ",\"val\":" + String(v) + "}\n");
        } else {
            c.print("{\"ok\":false,\"err\":\"range\"}\n");
        }
        return;
    }
    if (cmd.startsWith("TOGGLE_RELAY ")) {
        int n = cmd.charAt(13) - '0';
        if (n >= 0 && n < 4) {
            relay_state[n] = !relay_state[n];
            apply_relays();
            c.print("{\"ok\":true,\"relay\":" + String(n) + ",\"val\":" + String((int)relay_state[n]) + "}\n");
        }
        return;
    }
    if (cmd.startsWith("SET_ALL_RELAYS ")) {
        int v = cmd.charAt(15) - '0';
        for (int i = 0; i < 4; i++) relay_state[i] = (v != 0);
        apply_relays();
        c.print("{\"ok\":true,\"all_relays\":" + String(v) + "}\n");
        return;
    }
    if (cmd.startsWith("SET_LED_STATUS ")) {
        int n = cmd.charAt(15) - '0';
        int v = cmd.charAt(17) - '0';
        if (n >= 0 && n < 4) {
            led_status[n] = (v != 0);
            apply_leds();
            c.print("{\"ok\":true,\"led\":" + String(n) + ",\"val\":" + String(v) + "}\n");
        }
        return;
    }
    if (cmd.startsWith("SET_LED_USER ")) {
        int v = cmd.charAt(13) - '0';
        led_user_state = (v != 0);
        apply_leds();
        c.print("{\"ok\":true,\"led_user\":" + String(v) + "}\n");
        return;
    }
    if (cmd.startsWith("SET_LED_R ")) {
        int v = cmd.charAt(10) - '0';
        led_r_state = (v != 0);
        apply_leds();
        c.print("{\"ok\":true,\"led_r\":" + String(v) + "}\n");
        return;
    }
    c.print("{\"ok\":false,\"err\":\"unknown\"}\n");
}

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);

    for (int i = 0; i < 8; i++) pinMode(INPUTS[i], INPUT);
    for (int i = 0; i < 4; i++) { pinMode(RELAYS[i], OUTPUT); digitalWrite(RELAYS[i], LOW); }
    for (int i = 0; i < 4; i++) { pinMode(STATUS_LEDS[i], OUTPUT); digitalWrite(STATUS_LEDS[i], LOW); }
    pinMode(LED_USER, OUTPUT);
    pinMode(LEDR, OUTPUT);
    pinMode(BTN_USER, INPUT_PULLUP);

    digitalWrite(LEDR, HIGH);

    Serial.print("Conectando a "); Serial.println(SSID);
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.print("\nIP: "); Serial.println(WiFi.localIP());

    server.begin();
    digitalWrite(LEDR, LOW);
    digitalWrite(LED_USER, HIGH);
    led_user_state = true;
    Serial.print("TCP listo en puerto "); Serial.println(TCP_PORT);
}

String incoming = "";
bool btn_prev = true;

void loop() {
    if (!client || !client.connected()) {
        WiFiClient c = server.available();
        if (c) { client = c; incoming = ""; Serial.println("Cliente conectado"); }
    }

    if (client && client.connected()) {
        while (client.available()) {
            char ch = client.read();
            if (ch == '\n') {
                incoming.trim();
                if (incoming.length() > 0) process_command(incoming, client);
                incoming = "";
            } else if (ch != '\r') {
                incoming += ch;
            }
        }
    }

    bool btn_now = digitalRead(BTN_USER);
    if (btn_prev == HIGH && btn_now == LOW) {
        relay_state[0] = !relay_state[0];
        apply_relays();
        delay(50);
    }
    btn_prev = btn_now;
}
