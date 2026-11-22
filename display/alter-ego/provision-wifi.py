#!/usr/bin/env python3
"""Save a Wi-Fi network on the Himmelsgravur StopWatch over USB."""

import argparse
import getpass
import json
import re
import sys
import time

import serial

ANSI = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")


def quote(value):
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def command(port, line):
    port.write((line + "\n").encode("ascii"))
    deadline = time.monotonic() + 4
    while time.monotonic() < deadline:
        raw = port.readline().decode(errors="replace")
        reply = ANSI.sub("", raw).rstrip("\r\n").rsplit("\r", 1)[-1].strip()
        if reply.startswith(("@wifi ", "@ok ", "@error ")):
            return reply
    raise SystemExit("Keine Antwort von der StopWatch.")


def status(port):
    reply = command(port, "wifi list")
    if not reply.startswith("@wifi "):
        raise SystemExit("WLAN-Status konnte nicht abgefragt werden.")
    return json.loads(reply[len("@wifi "):])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port")
    parser.add_argument("ssid", nargs="?", default="Zweibahnstrasse")
    parser.add_argument("--status", action="store_true", help="show saved networks and active connection")
    parser.add_argument("--fallback", action="store_true", help="save without promoting the network")
    args = parser.parse_args()
    if args.status and args.fallback:
        parser.error("--status and --fallback cannot be combined")

    if not args.status:
        if not sys.stdin.isatty():
            raise SystemExit("Bitte in einem lokalen Terminal mit verdeckter Passworteingabe ausführen.")
        if not (1 <= len(args.ssid) <= 32 and args.ssid.isascii() and args.ssid.isprintable()):
            raise SystemExit("Die SSID muss aus 1–32 druckbaren ASCII-Zeichen bestehen.")
        password = getpass.getpass(f"WLAN-Passwort für {args.ssid}: ")
        if not (8 <= len(password) <= 63 and password.isascii() and password.isprintable()):
            raise SystemExit("Erwartet wird ein WLAN-Passwort mit 8–63 druckbaren ASCII-Zeichen.")

    with serial.Serial(args.port, 115200, timeout=0.25, write_timeout=2) as port:
        port.reset_input_buffer()
        if args.status:
            print(json.dumps(status(port), ensure_ascii=False))
            return
        status(port)
        if command(port, f"wifi add {quote(args.ssid)} {quote(password)}") != "@ok wifi add":
            raise SystemExit("Das WLAN konnte nicht gespeichert werden.")
        if args.fallback:
            print(f"{args.ssid} als zusätzliches WLAN gespeichert. Das bevorzugte Netz bleibt an erster Stelle.")
            return
        if command(port, f"wifi join {quote(args.ssid)}") != "@ok wifi join":
            raise SystemExit("Das gespeicherte WLAN konnte nicht ausgewählt werden.")
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            if status(port).get("current") == args.ssid:
                print(f"StopWatch mit {args.ssid} verbunden und IP-Adresse erhalten.")
                return
            time.sleep(1)
    raise SystemExit("WLAN gespeichert. Die StopWatch versucht die Verbindung weiter.")


if __name__ == "__main__":
    main()
