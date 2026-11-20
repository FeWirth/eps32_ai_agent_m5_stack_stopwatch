# Hermes-Agent auf der M5Stack StopWatch

Ziel ist ein kleiner Avatar auf der ESP32-S3-Uhr, über den man mit einem Hermes-Agenten sprechen kann. Die [ursprünglichen Projektziele](goal.md) beschreiben lokales WLAN, Bluetooth LE als Rückfall und optionalen Fernzugriff.

## Projektstand

**21. November 2026 · Meilenstein 1** — Auf der StopWatch ist Himmelsgravur mit hellerem Sternenkreis, Vollmond und Sprechblase installiert. Beide interaktiven Avatare sind für das runde 466 × 466 AMOLED verfügbar.

| Bereich | Status | Nachweis oder offene Arbeit |
| --- | --- | --- |
| Himmelsgravur | Auf der StopWatch installiert | Aktualisierte Firmware per Prüfsumme verifiziert und die Uhr neu gestartet. Die Darstellung wurde in der Mac-Vorschau geprüft. [Renderer und Build](display/alter-ego/) |
| Mary / Piepi | Auf der StopWatch erprobt | Mac-Vorschau gebaut, ESP32-S3-Firmware verifiziert und Funktion auf dem Gerät bestätigt. [Renderer und Build](display/alter_ego_mary/) |
| Sprechblase für Himmelsgravur | Auf der StopWatch installiert | Wechsel zwischen „Hi, Felix!“ und Berliner Ortszeit sowie Erhalt der Uhrzeit nach Neustart am Gerät bestätigt. |
| Hermes über lokales WLAN | Offen | Agent und StopWatch verbinden und den vollständigen Gesprächsablauf testen. |
| Bluetooth LE als Rückfall | Offen | Automatischen Wechsel zu Bluetooth und zurück zu WLAN umsetzen und testen. |
| Optionaler Fernzugriff | Offen | Tailscale-Anbindung mit Einstellung zum Ein- und Ausschalten planen. |

Die Avatar-Demo startet direkt nach der Display- und Touch-Initialisierung. Für die Hermes-Integration ist das normale StopWatch-Profil der Ausgangspunkt.

## Einstieg

- [Übergabe für Codex und neue Mitwirkende](codex/README.md)
- [Himmelsgravur auf Mac und ESP32-S3 bauen](display/alter-ego/README.md)
- [Mary auf Mac und ESP32-S3 bauen](display/alter_ego_mary/README.md)
- [Hinweise für UI-Arbeit](ui_elements/README.md)

Bei Änderungen an Firmware, Gerät oder Projektzielen den Status in dieser README aktualisieren. Reproduzierbare Abläufe und Erkenntnisse stehen in [codex/](codex/README.md). Der Git-Tag `meilenstein-1` markiert den ursprünglichen Stand vom 20. November 2026. Die Sprechblase und die gepufferte Uhrzeit sind ein Nachtrag zu Meilenstein 1 auf `main`.
