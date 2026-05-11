/*
 * Arduino Opta - VISA Serial Demo
 * LabVIEW HMI via USB-C (VISA)
 *
 * Conexion: cable USB-C al puerto USB del Opta
 * En LabVIEW: VISA Resource = "COMx::INSTR"
 * Baud rate: 115200, 8N1
 *
 * Mismo protocolo de comandos que la version TCP:
 *   GET_STATUS\n
 *   SET_RELAY n v\n
 *   TOGGLE_RELAY n\n
 *   SET_ALL_RELAYS v\n
 *   SET_LED_STATUS n v\n
 *   SET_LED_USER v\n
 *   SET_LED_R v\n
 */

#include <Arduino.h>

const int INPUTS[8]      = {A0, A1, A2, A3, A4, A5, A6, A7};
const int RELAYS[4]      = {RELAY1, RELAY2, RELAY3, RELAY4};
const int STATUS_LEDS[4] = {LED_D0, LED_D1, LED_D2, LED_D3};

bool relay_state[4] = {false, false, false, false};
bool led_status[4]  = {false, false, false, false};
bool led_user_state = false;
bool led_r_state    = false;

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

void process_command(const String& cmd) {
    if (cmd == "GET_STATUS") {
        Serial.println(build_status_json());
        return;
    }
    if (cmd.startsWith("SET_RELAY ")) {
        int n = cmd.charAt(10) - '0';
        int v = cmd.charAt(12) - '0';
        if (n >= 0 && n < 4) {
            relay_state[n] = (v != 0);
            apply_relays();
            Serial.println("{\"ok\":true,\"relay\":" + String(n) + ",\"val\":" + String(v) + "}");
        } else {
            Serial.println("{\"ok\":false,\"err\":\"range\"}");
        }
        return;
    }
    if (cmd.startsWith("TOGGLE_RELAY ")) {
        int n = cmd.charAt(13) - '0';
        if (n >= 0 && n < 4) {
            relay_state[n] = !relay_state[n];
            apply_relays();
            Serial.println("{\"ok\":true,\"relay\":" + String(n) + ",\"val\":" + String((int)relay_state[n]) + "}");
        }
        return;
    }
    if (cmd.startsWith("SET_ALL_RELAYS ")) {
        int v = cmd.charAt(15) - '0';
        for (int i = 0; i < 4; i++) relay_state[i] = (v != 0);
        apply_relays();
        Serial.println("{\"ok\":true,\"all_relays\":" + String(v) + "}");
        return;
    }
    if (cmd.startsWith("SET_LED_STATUS ")) {
        int n = cmd.charAt(15) - '0';
        int v = cmd.charAt(17) - '0';
        if (n >= 0 && n < 4) {
            led_status[n] = (v != 0);
            apply_leds();
            Serial.println("{\"ok\":true,\"led\":" + String(n) + ",\"val\":" + String(v) + "}");
        }
        return;
    }
    if (cmd.startsWith("SET_LED_USER ")) {
        int v = cmd.charAt(13) - '0';
        led_user_state = (v != 0);
        apply_leds();
        Serial.println("{\"ok\":true,\"led_user\":" + String(v) + "}");
        return;
    }
    if (cmd.startsWith("SET_LED_R ")) {
        int v = cmd.charAt(10) - '0';
        led_r_state = (v != 0);
        apply_leds();
        Serial.println("{\"ok\":true,\"led_r\":" + String(v) + "}");
        return;
    }
    Serial.println("{\"ok\":false,\"err\":\"unknown\"}");
}

void setup() {
    Serial.begin(115200);
    // Espera hasta 2s a que el host abra el puerto — no bloquea si no hay PC
    unsigned long t = millis();
    while (!Serial && millis() - t < 2000);

    analogReadResolution(12);

    for (int i = 0; i < 8; i++) pinMode(INPUTS[i], INPUT);
    for (int i = 0; i < 4; i++) {
        pinMode(RELAYS[i], OUTPUT);
        digitalWrite(RELAYS[i], LOW);
    }
    for (int i = 0; i < 4; i++) {
        pinMode(STATUS_LEDS[i], OUTPUT);
        digitalWrite(STATUS_LEDS[i], LOW);
    }
    pinMode(LED_USER, OUTPUT);
    pinMode(LEDR, OUTPUT);
    pinMode(BTN_USER, INPUT_PULLUP);

    digitalWrite(LEDR, LOW);
    digitalWrite(LED_USER, HIGH);
    led_user_state = true;

    // Mensaje de bienvenida — LabVIEW puede leerlo o ignorarlo
    Serial.println("{\"boot\":true,\"version\":\"1.0\"}");
}

String incoming = "";
bool btn_prev = true;

void loop() {
    // Leer comandos del Serial (VISA)
    while (Serial.available()) {
        char ch = Serial.read();
        if (ch == '\n') {
            incoming.trim();
            if (incoming.length() > 0) process_command(incoming);
            incoming = "";
        } else if (ch != '\r') {
            incoming += ch;
        }
    }

    // BTN_USER fisico: toggle relay D0
    bool btn_now = digitalRead(BTN_USER);
    if (btn_prev == HIGH && btn_now == LOW) {
        relay_state[0] = !relay_state[0];
        apply_relays();
        delay(50);
    }
    btn_prev = btn_now;
}
