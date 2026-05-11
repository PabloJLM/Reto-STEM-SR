import socket
import json
import time
import argparse
import threading

def send_cmd(sock, cmd: str) -> dict | None:
    sock.sendall((cmd + "\n").encode())
    data = b""
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            break
        data += chunk
        if b"\n" in data:
            break
    try:
        return json.loads(data.decode().strip())
    except Exception as e:
        print(f"  Parse error: {e}, raw: {data}")
        return None

def poll_loop(sock, interval=0.5):
    """Polling continuo - equivalente al While Loop de LabVIEW"""
    while True:
        status = send_cmd(sock, "GET_STATUS")
        if status:
            inputs_v = status.get("inputs_v", [])
            relays   = status.get("relays", [])
            leds     = status.get("leds_status", [])
            btn      = status.get("btn_user", False)
            led_u    = status.get("led_user", False)
            led_r    = status.get("led_r", False)

            print("\033[2J\033[H", end="")  # clear
            print("=== Arduino Opta - Monitor TCP ===")
            print(f"  Entradas (V): {[f'{v:.2f}' for v in inputs_v]}")
            print(f"  Relays D0-D3: {relays}")
            print(f"  LEDs D0-D3:   {leds}")
            print(f"  LED Usuario:  {led_u}  |  LED Rojo: {led_r}")
            print(f"  Boton User:   {btn}")
            print()
            print("Comandos: r0..r3=toggle relay, a=all on, o=all off, q=quit")
        time.sleep(interval)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip",   default="10.200.67.26")
    parser.add_argument("--port", default=8888, type=int)
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((args.ip, args.port))
    sock.settimeout(2.0)
    print(f"Conectado a {args.ip}:{args.port}")

    # Hilo de polling
    t = threading.Thread(target=poll_loop, args=(sock,), daemon=True)
    t.start()

    # Control interactivo
    while True:
        cmd = input().strip().lower()
        if cmd == "q":
            break
        elif cmd in ("r0","r1","r2","r3"):
            n = int(cmd[1])
            send_cmd(sock, f"TOGGLE_RELAY {n}")
        elif cmd == "a":
            send_cmd(sock, "SET_ALL_RELAYS 1")
        elif cmd == "o":
            send_cmd(sock, "SET_ALL_RELAYS 0")
        elif cmd.startswith("lu"):
            v = 1 if len(cmd) < 3 else int(cmd[2])
            send_cmd(sock, f"SET_LED_USER {v}")
        elif cmd.startswith("lr"):
            v = 1 if len(cmd) < 3 else int(cmd[2])
            send_cmd(sock, f"SET_LED_R {v}")

    sock.close()

if __name__ == "__main__":
    main()