# Hermes-Agent auf der M5Stack StopWatch

Ziel ist ein kleiner Avatar auf der ESP32-S3-Uhr, über den man mit einem Hermes-Agenten sprechen kann. Die [ursprünglichen Projektziele](goal.md) beschreiben lokales WLAN, Bluetooth LE als Rückfall und optionalen Fernzugriff.

## Projektstand

**20. November 2026 · Meilenstein 1** — Zwei interaktive Avatare für das runde 466 × 466 AMOLED sind gebaut und wurden nacheinander auf dieselbe StopWatch geflasht. Zuletzt wurde Himmelsgravur wieder aufgespielt.

| Bereich | Status | Nachweis oder offene Arbeit |
| --- | --- | --- |
| Himmelsgravur | Auf der StopWatch installiert | Firmware geflasht, alle Schreibvorgänge verifiziert und Neustart ausgelöst. [Renderer und Build](display/alter-ego/) |
| Mary / Piepi | Auf der StopWatch erprobt | Mac-Vorschau gebaut, ESP32-S3-Firmware verifiziert und Funktion auf dem Gerät bestätigt. [Renderer und Build](display/alter_ego_mary/) |
| Hermes über lokales WLAN | Offen | Agent und StopWatch verbinden und den vollständigen Gesprächsablauf testen. |
| Bluetooth LE als Rückfall | Offen | Automatischen Wechsel zu Bluetooth und zurück zu WLAN umsetzen und testen. |
| Optionaler Fernzugriff | Offen | Tailscale-Anbindung mit Einstellung zum Ein- und Ausschalten planen. |

Die Avatar-Demo startet direkt nach der Display- und Touch-Initialisierung. Für die Hermes-Integration ist das normale StopWatch-Profil der Ausgangspunkt.

## Einstieg

- [Übergabe für Codex und neue Mitwirkende](codex/README.md)
- [Himmelsgravur auf Mac und ESP32-S3 bauen](display/alter-ego/README.md)
- [Mary auf Mac und ESP32-S3 bauen](display/alter_ego_mary/README.md)
- [Hinweise für UI-Arbeit](ui_elements/README.md)

Bei Änderungen an Firmware, Gerät oder Projektzielen den Status in dieser README aktualisieren. Reproduzierbare Abläufe und Erkenntnisse stehen in [codex/](codex/README.md). Der Git-Tag `meilenstein-1` markiert den Stand vom 20. November 2026.
