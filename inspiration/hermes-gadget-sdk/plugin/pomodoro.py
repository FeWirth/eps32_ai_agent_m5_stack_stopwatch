"""Persist a voice-started Pomodoro in the Felix vault."""

from contextlib import contextmanager
from datetime import datetime
import fcntl
from io import StringIO
import os
import re
from pathlib import Path
import stat
import subprocess
import tempfile
import unicodedata
from zoneinfo import ZoneInfo

try:
    from ruamel.yaml import YAML
except ImportError:
    # Older Hermes installations use PyYAML.
    import yaml

    def read_yaml(text):
        return yaml.safe_load(text)

    def write_yaml(data):
        return yaml.safe_dump(data, allow_unicode=True, sort_keys=False)
else:
    def read_yaml(text):
        return YAML(typ="safe", pure=True).load(text)

    def write_yaml(data):
        parser = YAML(typ="safe", pure=True)
        parser.default_flow_style = False
        parser.sort_base_mapping_type_on_output = False
        output = StringIO()
        parser.dump(data, output)
        return output.getvalue()


# Fallback for Pi checkouts that do not yet contain the local vault rules.
LOG_TAGS = {
    "log/admin",
    "log/admin/invoices",
    "log/admin/tax",
    "log/breakfast",
    "log/breath-work",
    "log/brunch",
    "log/call",
    "log/cleaning",
    "log/coding",
    "log/coffee",
    "log/cooking",
    "log/dinner",
    "log/drinks",
    "log/event",
    "log/expense",
    "log/experience",
    "log/gardening",
    "log/guitar",
    "log/gym/abs",
    "log/gym/back",
    "log/gym/biceps",
    "log/gym/chest",
    "log/gym/hiit",
    "log/gym/legs",
    "log/gym/shoulders",
    "log/gym/triceps",
    "log/highlight",
    "log/home",
    "log/ice-bath",
    "log/journaling",
    "log/learning",
    "log/lunch",
    "log/meditation",
    "log/meeting",
    "log/movie",
    "log/piano",
    "log/pilates",
    "log/podcast",
    "log/presentation",
    "log/reading",
    "log/report",
    "log/research",
    "log/run",
    "log/sauna",
    "log/series",
    "log/shopping",
    "log/swim",
    "log/walk",
    "log/win",
    "log/yoga",
}


def frontmatter(path):
    parts = path.read_text(encoding="utf-8").split("---", 2)
    return (read_yaml(parts[1]) or {}) if len(parts) == 3 and not parts[0] else {}


def normalized(value):
    return " ".join(re.findall(r"\w+", unicodedata.normalize("NFC", value).casefold()))


def resolve(vault, query):
    key = normalized(query)
    if not key:
        raise ValueError("Für welches Projekt soll der Pomodoro starten?")
    exact, partial = [], []
    for path in sorted((vault / "Notes").glob("Projekt - *.md")):
        meta = frontmatter(path)
        if meta.get("type") != "project":
            continue
        title = path.stem.removeprefix("Projekt - ")
        aliases = meta.get("aliases") or []
        names = [title, path.stem, *([aliases] if isinstance(aliases, str) else aliases)]
        keys = {normalized(name) for name in names if isinstance(name, str)}
        if key in keys:
            exact.append((path, title, meta))
        elif any(f" {key} " in f" {name} " for name in keys):
            partial.append((path, title, meta))
    matches = exact or partial
    if len(matches) == 1:
        return matches[0]
    if matches:
        raise ValueError("Welches Projekt meinst du? " + ", ".join(title for _, title, _ in matches))
    raise ValueError(f"Kein Projekt für „{query}“ gefunden. Wie heißt es im Vault?")


def replace(path, content):
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as temp:
        pending = Path(temp.name)
        try:
            if path.exists():
                pending.chmod(stat.S_IMODE(path.stat().st_mode))
            temp.write(content.encode("utf-8"))
            temp.flush()
            pending.replace(path)
        finally:
            pending.unlink(missing_ok=True)


def log_entry(original, time, entry):
    heading = re.search(r"(?m)^## Log[ \t]*$", original)
    if heading is None:
        raise ValueError("Die Tagesnotiz enthält keinen Abschnitt ## Log.")
    following = re.search(r"(?m)^#{1,2} ", original[heading.end():])
    end = heading.end() + following.start() if following else len(original)
    for block in re.finditer(r"(?m)^- (\d\d:\d\d)[ \t]*$", original[heading.end():end]):
        if block[1] == time:
            at = heading.end() + block.end()
            return original[:at] + "\n" + entry + original[at:]
        if block[1] > time:
            at = heading.end() + block.start()
            return original[:at] + f"- {time}\n{entry}\n" + original[at:]
    at = len(original[:end].rstrip("\n"))
    return original[:at] + f"\n- {time}\n{entry}" + (original[at:] or "\n")


@contextmanager
def locked(folder):
    # The journal directory stays in place when daily notes are atomically replaced.
    fd = os.open(folder, os.O_RDONLY | os.O_DIRECTORY)
    try:
        fcntl.flock(fd, fcntl.LOCK_EX)
        yield
    finally:
        os.close(fd)


def record(vault, project, activity, minutes, receipt, area_tag, log_tag):
    path, title, meta = project
    journal = vault / "Journal" / "Daily"
    work_template = (vault / "Templates" / "Work Session.md").read_text(encoding="utf-8")
    with locked(journal):
        now = datetime.now(ZoneInfo("Europe/Berlin"))
        notes = vault / "Notes"
        existing = next(notes.glob(f"????-??-?? Work Session - * ({receipt}).md"), None)
        started = frontmatter(existing)["pomodoro-started"] if existing else now.isoformat()
        start = datetime.fromisoformat(started)
        date, time = start.strftime("%Y-%m-%d"), start.strftime("%H:%M")
        topic = re.sub(r'[<>:"/\\|?*\[\]#^]', "-", activity or title).strip(" .-")[:60] or "Pomodoro"
        session = existing or notes / f"{date} Work Session - {topic} ({receipt}).md"
        daily = journal / start.strftime("%Y/%m/%Y-%m-%d.md")
        original = (daily if daily.exists() else vault / "Templates" / "Daily.md").read_text(encoding="utf-8")
        original = original.replace("{{date:YYYY-MM-DD}}", date)
        project_link = f"[[Notes/{path.stem}]]"
        session_link = f"[[Notes/{session.stem}]]"
        if session_link in original:
            return str(session.relative_to(vault))
        hubs = meta.get("hub") or []
        header = {
            "type": "work-session", "date": date, "up": [project_link],
            "area": meta.get("area") or [], "hub": hubs, "related": [],
            "summary": None, "pomodoro-started": started,
        }
        description = f"Pomodoro gestartet, {minutes} Minuten geplant."
        goal = re.sub(r"([\\`*_\[\]<>#])", r"\\\1", activity)
        body = work_template.split("---", 2)[2]
        body = body.replace("## Ziel\n", f"## Ziel\n\n{goal}\n" if goal else "## Ziel\n", 1)
        body = body.replace("## Verlauf\n", f"## Verlauf\n\n- {time} {description}\n", 1)
        entry = (f"\t- [.] 🖥️ #{area_tag} #{log_tag} " + " ".join([project_link, *hubs])
                 + f"\n\t\t- {description}" + (f" {goal}" if goal else "")
                 + f"\n\t\t- {session_link}")
        updated = log_entry(original, time, entry)
        if not existing:
            replace(session, "---\n" + write_yaml(header).rstrip() + "\n---" + body)
        daily.parent.mkdir(parents=True, exist_ok=True)
        replace(daily, updated)
        return str(session.relative_to(vault))


def tags(vault, meta, requested):
    areas = {
        "beruf und forschung bereich": "area/career",
        "finanzen bereich": "area/wealth",
        "freizeit bereich": "area/leisure",
        "gesundheit bereich": "area/health",
        "persönliche entwicklung bereich": "area/personal-development",
        "soziales bereich": "area/social",
        "umfeld bereich": "area/environment",
    }
    links = meta.get("area") or []
    first = str(links[0]).split("|")[0].rstrip("]]").rsplit("/", 1)[-1] if links else ""
    area = areas.get(normalized(first), "area/personal-development")
    rules = vault / "Skills" / "MAIN_VAULT_AGENT.md"
    known = LOG_TAGS
    if rules.is_file():
        known = re.findall(r"(?m)^\| `#(log/[a-z/-]+)` \|", rules.read_text(encoding="utf-8"))
    tag = requested if requested in known else "log/focus"
    if area == "area/career" and tag == "log/research":
        area = "area/career/research"
    return area, tag


def publish(vault, note):
    started = datetime.fromisoformat(frontmatter(vault / note)["pomodoro-started"])
    daily = started.strftime("Journal/Daily/%Y/%m/%Y-%m-%d.md")

    def git(*args, timeout=15):
        return subprocess.run(
            ["git", *args], cwd=vault, env={**os.environ, "GIT_TERMINAL_PROMPT": "0"},
            check=True, capture_output=True, text=True, timeout=timeout,
        ).stdout.strip()

    with locked(vault / "Journal" / "Daily"):
        if git("branch", "--show-current") != "main":
            raise ValueError("Vault-Sync erwartet den Branch main.")
        if git("status", "--porcelain", "--", daily, note):
            git("add", "--", daily, note)
            git("commit", "--only", "-m", "Log Pomodoro in Obsidian", "--", daily, note)
        git("push", "origin", "main")
