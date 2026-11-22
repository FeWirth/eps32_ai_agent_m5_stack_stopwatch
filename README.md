# Hermes-Agent auf der M5Stack StopWatch

Ziel ist ein kleiner Avatar auf der ESP32-S3-Uhr, über den man mit einem Hermes-Agenten sprechen kann. Die [ursprünglichen Projektziele](goal.md) beschreiben lokales WLAN, Bluetooth LE als Rückfall und optionalen Fernzugriff.

## Projektstand

**21. November 2026 · Meilenstein 1** — Auf der StopWatch ist Himmelsgravur mit hellerem Sternenkreis, Vollmond, WLAN-Status, Akkuansicht und Energiesparmodus installiert. Die Löffel-Animation ist ebenfalls installiert. Beide interaktiven Avatare sind für das runde 466 × 466 AMOLED verfügbar.

| Bereich | Status | Nachweis oder offene Arbeit |
| --- | --- | --- |
| Himmelsgravur | Auf der StopWatch installiert | Aktualisierte Firmware per Prüfsumme verifiziert und die Uhr neu gestartet. Die Darstellung wurde in der Mac-Vorschau geprüft. [Renderer und Build](display/alter-ego/) |
| Mary / Piepi | Auf der StopWatch erprobt | Mac-Vorschau gebaut, ESP32-S3-Firmware verifiziert und Funktion auf dem Gerät bestätigt. [Renderer und Build](display/alter_ego_mary/) |
| Sprechblase für Himmelsgravur | Auf der StopWatch installiert | Tipp-Loop mit Berliner Ortszeit. Automatische Wiederherstellung einer ungültigen Hardware-Uhr über WLAN auf der StopWatch bestätigt. |
| Automatische WLAN-Verbindung | Auf der StopWatch installiert | Heimnetz über USB eingerichtet. Automatische Verbindung nach einem Neustart der StopWatch bestätigt. |
| Akkuansicht für Himmelsgravur | Auf der StopWatch installiert | Herunterziehen und Schließen am Gerät bestätigt. |
| Energiesparen | Auf der StopWatch installiert | Animation hält nach 5 s an, AMOLED schaltet nach 10 s ab. Aufwecken per Tipp und leiser Akkubetrieb bestätigt. 20,7 s tatsächlicher Light Sleep während einer rund 27 s langen Doze-Periode gemessen. |
| Löffel-Animation beim Einschlafen | Auf der StopWatch installiert | Firmware per Prüfsumme verifiziert. Die Animation wurde in der Mac-Vorschau geprüft. |
| Hermes über lokales WLAN | Offen | Agent und StopWatch verbinden und den vollständigen Gesprächsablauf testen. |
| Bluetooth LE als Rückfall | Offen | Automatischen Wechsel zu Bluetooth und zurück zu WLAN umsetzen und testen. |
| Optionaler Fernzugriff | Offen | Tailscale-Anbindung mit Einstellung zum Ein- und Ausschalten planen. |

Himmelsgravur rendert die Avatar-Oberfläche in der SDK-Ereignisschleife. Das SDK verwaltet WLAN und Gerätestatus. Der Hermes-Gesprächsablauf steht noch aus.

## Einstieg

- [Übergabe für Codex und neue Mitwirkende](codex/README.md)
- [Entwicklungslog mit Hardware-Erkenntnissen und Messwerten](codex/entwicklungslog.md)
- [Himmelsgravur auf Mac und ESP32-S3 bauen](display/alter-ego/README.md)
- [Mary auf Mac und ESP32-S3 bauen](display/alter_ego_mary/README.md)
- [Hinweise für UI-Arbeit](ui_elements/README.md)

Bei Änderungen an Firmware, Gerät oder Projektzielen den Status in dieser README aktualisieren. Reproduzierbare Abläufe und Erkenntnisse stehen in [codex/](codex/README.md). Der Git-Tag `meilenstein-1` markiert den ursprünglichen Stand vom 20. November 2026. Sprechblase, WLAN, Akkuansicht, Energiesparen und Löffel-Animation sind Nachträge zu Meilenstein 1 auf `main`.
