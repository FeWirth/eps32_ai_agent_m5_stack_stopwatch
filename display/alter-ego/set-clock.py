#!/usr/bin/env python3
"""Set the Himmelsgravur StopWatch RTC from the host over USB."""

import argparse
from datetime import datetime
from time import monotonic
from zoneinfo import ZoneInfo

import serial


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="StopWatch USB serial port")
    parser.add_argument("--zone", default="Europe/Berlin", help="IANA time zone, default Europe/Berlin")
    parser.add_argument("--read", action="store_true", help="read the RTC without setting it")
    args = parser.parse_args()
    zone = ZoneInfo(args.zone)

    with serial.Serial(args.port, 115200, timeout=0.5, write_timeout=1) as port:
        port.reset_input_buffer()
        now = datetime.now(zone)
        command = "rtc\n" if args.read else now.strftime("rtc %Y %m %d %w %H %M %S\n")
        port.write(command.encode("ascii"))
        deadline = monotonic() + 5
        while monotonic() < deadline:
            reply = port.readline()
            if args.read and b"@rtc " in reply:
                value = reply.split(b"@rtc ", 1)[1].strip().decode("ascii")
                if value == "unset":
                    raise SystemExit("StopWatch RTC is unset.")
                print(f"StopWatch RTC: {value}")
                return
            if b"@rtc ok" in reply:
                print(f"StopWatch RTC verified: {now.isoformat(timespec='seconds')} ({zone.key})")
                return
            if b"@rtc error" in reply:
                raise SystemExit("StopWatch could not verify the RTC write.")
    raise SystemExit("No RTC response from the StopWatch.")


if __name__ == "__main__":
    main()
