"""Opt-in deterministic Daily Note receipt for the paired StopWatch pilot."""

from datetime import datetime
import fcntl
import os
from pathlib import Path
import re
import stat
import tempfile
from zoneinfo import ZoneInfo

TEXT = "ESP32-Verbindung zu Hermes getestet."
LINK = "[[Notes/Projekt - ESP32-Avatar|ESP32-Avatar]]"


def record(vault: Path, message_id: str) -> None:
    if not re.fullmatch(r"vault-smoke-hg-[0-9a-f]{16}", message_id):
        raise ValueError("invalid StopWatch receipt ID")
    daily = vault / "Journal" / "Daily"
    marker = f"<!-- {message_id} -->"
    # Lock the directory inode so atomic note replacement does not replace the lock.
    folder = os.open(daily, os.O_RDONLY | os.O_DIRECTORY)
    try:
        fcntl.flock(folder, fcntl.LOCK_EX)
        for note in daily.glob("*/*/*.md"):
            if any(line.endswith(marker) for line in note.read_text().splitlines()):
                return
        now = datetime.now(ZoneInfo("Europe/Berlin"))
        note = daily / now.strftime("%Y/%m/%Y-%m-%d.md")
        original = note.read_bytes().decode("utf-8")
        heading = re.search(r"(?m)^## Log[ \t]*\r?$", original)
        if not heading:
            raise ValueError("Daily Note has no Log section")
        newline = "\r\n" if "\r\n" in original else "\n"
        rest = original[heading.end():]
        following = re.search(r"(?m)^#{1,2} ", rest)
        end = heading.end() + (following.start() if following else len(rest))
        content = original[:end].rstrip("\r\n")
        entry = f"- {now:%H:%M} · {TEXT} · {LINK} · Felix · ESP32 {marker}"
        suffix = original[len(content):]
        with tempfile.NamedTemporaryFile(dir=note.parent, delete=False) as temp:
            pending = Path(temp.name)
            try:
                os.fchmod(temp.fileno(), stat.S_IMODE(note.stat().st_mode))
                temp.write((content + newline + entry + (suffix or newline)).encode("utf-8"))
                temp.flush()
                os.fsync(temp.fileno())
                os.replace(pending, note)
            finally:
                pending.unlink(missing_ok=True)
    finally:
        os.close(folder)
