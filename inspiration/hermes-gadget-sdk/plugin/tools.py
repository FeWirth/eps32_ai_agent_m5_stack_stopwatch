"""Agent tools for gadgets: inspect devices, show content on a screen, invoke
device-declared actions.

Schemas are static (prompt caching); what a device can do is discovered at run
time through ``gadget_devices`` and the per-device context Hermes receives with
each message. Tools are available in any session served by the gateway process
that hosts the gadget adapter, so a user can also drive a gadget from another
chat ("show the grocery list on the kitchen gadget").
"""

from __future__ import annotations

import asyncio
import hashlib
import json
import os
import subprocess
from pathlib import Path
import threading
from typing import Any

from . import runtime, textfmt

TOOLSET = "gadget"
CALL_TIMEOUT_S = 20.0

_DEVICE_PARAM = {
    "type": "string",
    "description": "Gadget name or device id. Omit to use the gadget the user is talking through "
                   "(or the only connected gadget).",
}

DEVICES_SCHEMA = {
    "name": "gadget_devices",
    "description": "List connected Hermes gadgets (small hardware devices) with their screen, audio, "
                   "available device actions and latest sensor readings.",
    "parameters": {"type": "object", "properties": {}, "required": []},
}

DISPLAY_SCHEMA = {
    "name": "gadget_display",
    "description": "Show a short card (title and text) on a gadget's screen, e.g. a list or note. "
                   "For timers use the device's timer.* actions through gadget_action when it lists them. "
                   "Screens are small: keep text brief and plain (no Markdown). While the screen is off "
                   "the card waits and appears when it lights up.",
    "parameters": {
        "type": "object",
        "properties": {
            "text": {"type": "string", "description": "Body text to show."},
            "title": {"type": "string", "description": "Optional short title."},
            "seconds": {"type": "integer", "description": "How long to show it; 0 keeps it until dismissed. Default 20."},
            "device": _DEVICE_PARAM,
        },
        "required": ["text"],
    },
}

ACTION_SCHEMA = {
    "name": "gadget_action",
    "description": "Invoke an action a gadget exposes (for example an LED, relay, buzzer or volume control). "
                   "Use gadget_devices first if you do not know the action names and parameters.",
    "parameters": {
        "type": "object",
        "properties": {
            "action": {"type": "string", "description": "Action name as listed by the device."},
            "args": {"type": "object", "description": "Arguments matching the action's parameter schema."},
            "plain_timer": {"type": "boolean", "description": "Set true only for an explicitly requested "
                            "standalone 25/50-minute timer without a Work Session on StopWatch."},
            "device": _DEVICE_PARAM,
        },
        "required": ["action"],
    },
}


def _error(message: str) -> str:
    return json.dumps({"success": False, "error": message})


def _adapter():
    from gateway.session_context import get_session_env

    return runtime.active(get_session_env("HERMES_SESSION_PROFILE") or None)


def _run(adapter, coro) -> Any:
    loop = adapter.loop
    if loop is None or loop.is_closed():
        coro.close()
        raise RuntimeError("gadget hub is not running")
    if getattr(loop, "_thread_id", None) == threading.get_ident():
        coro.close()
        raise RuntimeError("gadget tools cannot block the gateway event loop")
    return asyncio.run_coroutine_threadsafe(coro, loop).result(CALL_TIMEOUT_S)


def _resolve(adapter, ref: str | None):
    from gateway.session_context import get_session_env

    hub = adapter.hub
    if hub is None:
        return None, "gadget hub is not running"
    if ref:
        session = hub.find(ref)
        if session is None:
            known = ", ".join(f"{s.name} ({s.device_id})" for s in hub.sessions.values()) or "none connected"
            return None, f"no connected gadget matches {ref!r}; connected: {known}"
        return session, None
    if get_session_env("HERMES_SESSION_PLATFORM") == "gadget":
        session = hub.get(get_session_env("HERMES_SESSION_CHAT_ID"))
        if session is not None:
            return session, None
    sessions = list(hub.sessions.values())
    if len(sessions) == 1:
        return sessions[0], None
    if not sessions:
        return None, "no gadget is connected"
    return None, "several gadgets are connected; pass device: " + ", ".join(s.name for s in sessions)


def _usable(session) -> str | None:
    return None if session.paired else f"{session.name} is waiting for pairing approval"


def gadget_devices(args: dict, **_kw) -> str:
    adapter = _adapter()
    if adapter is None or adapter.hub is None:
        return _error("gadget hub is not running in this process")
    online = {s.device_id: s.describe() for s in adapter.hub.sessions.values()}
    offline = []
    for device_id, rec in adapter.hub.store.devices().items():
        if device_id not in online:
            offline.append({"device_id": device_id, "name": rec.get("name") or device_id,
                            "board": rec.get("board"), "last_seen": rec.get("last_seen")})
    return json.dumps({"success": True, "connected": list(online.values()), "offline": offline})


def gadget_display(args: dict, **_kw) -> str:
    adapter = _adapter()
    if adapter is None:
        return _error("gadget hub is not running in this process")
    session, err = _resolve(adapter, args.get("device"))
    if err:
        return _error(err)
    if (err := _usable(session)):
        return _error(err)
    text = textfmt.for_device(str(args.get("text") or ""), session.charset)
    title = textfmt.for_device(str(args.get("title") or ""), session.charset)
    if not text and not title:
        return _error("text is required")
    seconds = args.get("seconds", 20)
    try:
        seconds = max(0, int(seconds))
    except (TypeError, ValueError):
        seconds = 20
    try:
        shown = _run(adapter, session.show_card(title, text, seconds))
    except Exception as exc:
        return _error(f"could not reach {session.name}: {exc}")
    result = {"success": True, "device": session.name, "shown_for_s": seconds or "until dismissed"}
    if shown is False:
        result["held"] = ("the screen is off; the card is shown when it lights up, "
                          "for what is left of its time, unless a newer card or image replaces it")
    return json.dumps(result)


def gadget_action(args: dict, **_kw) -> str:
    adapter = _adapter()
    if adapter is None:
        return _error("gadget hub is not running in this process")
    session, err = _resolve(adapter, args.get("device"))
    if err:
        return _error(err)
    if (err := _usable(session)):
        return _error(err)
    action = str(args.get("action") or "").strip()
    if not action:
        return _error("action is required")
    call_args = args.get("args") or {}
    if not isinstance(call_args, dict):
        return _error("args must be an object")
    if session.device_id == os.getenv("GADGET_POMODORO_DEVICE"):
        if action == "pomodoro.start" or (
            action == "timer.start" and str(call_args.get("seconds")) in {"1500", "3000", "1500.0", "3000.0"}
            and args.get("plain_timer") is not True
        ):
            return _error("Nutze gadget_pomodoro für 25/50 Minuten mit Projekt und Vault-Log. "
                          "Frag nach, falls das Projekt unklar ist. Nur bei ausdrücklich gewünschtem "
                          "reinem Timer ohne Vault-Eintrag ist plain_timer=true möglich.")
    try:
        result = _run(adapter, session.invoke_action(action, call_args, timeout=CALL_TIMEOUT_S - 2))
    except Exception as exc:
        return _error(str(exc))
    return json.dumps({"success": True, "device": session.name, "action": action, "result": result})


POMODORO_SCHEMA = {
    "name": "gadget_pomodoro",
    "description": "Use for a 25- or 50-minute project timer on the StopWatch even if called a timer. "
                   "Start the timer, log it in Obsidian with a linked Work Session, and publish to GitHub.",
    "parameters": {
        "type": "object",
        "properties": {
            "minutes": {"type": "integer", "enum": [25, 50]},
            "project": {"type": "string", "description": "Project name or alias as spoken"},
            "activity": {"type": "string", "description": "Optional activity or task as spoken"},
            "log_tag": {"type": "string", "description": "Activity tag from the vault rules, e.g. log/coding"},
        },
        "required": ["minutes", "project"],
    },
}


def gadget_pomodoro(args: dict, **_kw) -> str:
    from gateway import session_context
    from gateway.session_context import get_session_env

    from . import pomodoro

    adapter = _adapter()
    vault = os.getenv("GADGET_POMODORO_VAULT")
    if adapter is None or adapter.hub is None or not vault:
        return _error("Pomodoro ist im Gateway nicht eingerichtet.")
    if get_session_env("HERMES_SESSION_PLATFORM") != "gadget":
        return _error("Pomodoro von der StopWatch aus starten.")
    session = adapter.hub.get(get_session_env("HERMES_SESSION_CHAT_ID"))
    if (session is None or session.device_id != os.getenv("GADGET_POMODORO_DEVICE")
            or not adapter._still_trusted(session)):
        return _error("Diese StopWatch ist für Pomodoro nicht freigegeben.")
    # The agent's request context survives overlapping device turns and delayed speech.
    # The adapter's display lifecycle is shared by all requests on this connection.
    current_event = getattr(session_context, "_SESSION_TURN_EVENT", None)
    event = current_event.get() if current_event is not None else None
    turn = (getattr(event, "message_id", None) or get_session_env("HERMES_SESSION_MESSAGE_ID")
            or adapter._turns.get(session.device_id))
    if not turn:
        return _error("Kein aktiver StopWatch-Sprachturn.")
    minutes = args.get("minutes")
    if type(minutes) is not int or minutes not in (25, 50):
        return _error("Soll der Pomodoro 25 oder 50 Minuten laufen?")
    vault = Path(vault)
    activity = " ".join(str(args.get("activity") or "").split())[:300]
    receipt = hashlib.sha256(f"{session.device_id}:{turn}".encode()).hexdigest()[:16]
    try:
        project = pomodoro.resolve(vault, str(args.get("project") or ""))
        area_tag, log_tag = pomodoro.tags(vault, project[2], args.get("log_tag"))
    except (OSError, ValueError) as exc:
        return _error(str(exc))
    try:
        _run(adapter, session.invoke_action("pomodoro.start", {
            "minutes": minutes,
            "project": textfmt.for_device(project[1], "ascii")[:96],
            "activity": textfmt.for_device(activity or "Fokusarbeit", "ascii")[:96],
            "receipt": receipt,
        }))
    except Exception as exc:
        return _error(f"Timerstart konnte nicht bestätigt werden: {exc}")
    try:
        note = pomodoro.record(vault, project, activity, minutes, receipt, area_tag, log_tag)
    except (OSError, ValueError) as exc:
        return json.dumps({"success": False, "timer_started": True,
                           "error": f"Timer läuft. Obsidian-Eintrag fehlgeschlagen: {exc}"})
    result = {"success": True, "minutes": minutes, "project": project[1], "work_session": note}
    if os.getenv("GADGET_POMODORO_GIT_SYNC") == "1":
        try:
            pomodoro.publish(vault, note)
        except (OSError, ValueError, subprocess.SubprocessError) as exc:
            result["github_synced"] = False
            result["sync_error"] = f"Eintrag auf dem Pi gespeichert. GitHub-Sync offen: {exc}"
        else:
            result["github_synced"] = True
    return json.dumps(result)


def available() -> bool:
    """Reachability: tools work only in the process hosting the gadget hub."""
    return runtime.any_active()


def register_tools(ctx) -> None:
    for schema, handler, emoji in (
        (DEVICES_SCHEMA, gadget_devices, "📟"),
        (DISPLAY_SCHEMA, gadget_display, "🖥️"),
        (ACTION_SCHEMA, gadget_action, "🎛️"),
        (POMODORO_SCHEMA, gadget_pomodoro, "⏱️"),
    ):
        ctx.register_tool(
            name=schema["name"], toolset=TOOLSET, schema=schema, handler=handler,
            check_fn=available, description=schema["description"], emoji=emoji)
