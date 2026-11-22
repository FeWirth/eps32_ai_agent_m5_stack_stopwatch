# Projektübergabe

Stand vom 21. November 2026. Bei einer Übernahme zuerst den [Projektstand](../README.md), die [Projektziele](../goal.md) und [AGENTS.md](../AGENTS.md) lesen. Das [Entwicklungslog](entwicklungslog.md) sammelt Hardware-Erkenntnisse, Messwerte und offene Gerätetests. Bei UI-Arbeit gilt zusätzlich [ui_elements/README.md](../ui_elements/README.md).

## Architektur und aktueller Stand

- Die M5Stack StopWatch verwendet einen ESP32-S3, ein rundes 466 × 466 AMOLED mit CO5300 und Touch mit CST820. Der Renderer benötigt den vollständigen Framebuffer und berücksichtigt dessen RGB565-Byte-Reihenfolge.
- Himmelsgravur (`display/alter-ego`) und Mary (`display/alter_ego_mary`) besitzen je einen C++-Renderer in `avatar.hpp`. `preview.mm` verwendet denselben Renderer in einer AppKit-Vorschau. Ziehen dreht das Modell und Drücken erzeugt einen Lichtimpuls.
- Beide ESP32-Buildskripte setzen `HG_ALTER_EGO_DIR`. Das SDK bindet den jeweiligen Header über `firmware/esp32/main/CMakeLists.txt` ein. Himmelsgravur rendert in der regulären SDK-Ereignisschleife auf den AMOLED-Framebuffer. Die Standard-UI zeichnet dabei nicht auf das Display. Mary nutzt weiterhin die separate Demo-Schleife.
- Mary wurde auf der Uhr geflasht und vom Benutzer als funktionierend bestätigt. Danach wurde auf Wunsch wieder Himmelsgravur verifiziert geflasht und das Gerät neu gestartet.
- Der Himmelsgravur-Hintergrund verwendet die aus Mary übernommenen Ringparameter und den Vollmond. Die Änderung wurde in der Mac-Vorschau kontrolliert und als ESP32-Firmware gebaut. Am 21. November wurde dieser Build auf die StopWatch geflasht. Esptool bestätigte die geschriebenen Daten per Prüfsumme und löste einen Neustart aus.
- Die Himmelsgravur-Sprechblase trennt Text und Tipperkennung (`speech_bubble.hpp`) vom RX8130CE-Zugriff (`stopwatch_clock.hpp`). Die Mac-Vorschau liefert ihre lokale Systemzeit. Die Firmware mit Sprechblase wurde am 21. November geflasht. Der anschließende Abgleich über `display/alter-ego/set-clock.py` wurde vom RTC mit Berliner Ortszeit bestätigt. Kurze Tipps wechseln zyklisch zwischen dem zuletzt gesetzten Text, der laufenden Uhrzeit und dem WLAN-Status. Die Firmware aktiviert die RTC-Pufferung (CHGEN und INIEN). Nach dem Setzen wurde die Zeit nach einem USB-Hard-Reset mit `set-clock.py --read` zurückgelesen. Felix bestätigte anschließend den Tipp-Wechsel und den Erhalt der Uhrzeit nach einem Neustart am Gerät. Neue Funktionen können über `avatar.speech().show_message(text)` Text anzeigen.
- Der SDK-Quellcode liegt inklusive Lizenz und Hinweisen unter `inspiration/hermes-gadget-sdk`. Upstream ist [allinnb/hermes-gadget-sdk](https://github.com/allinnb/hermes-gadget-sdk) auf Commit `82c079a06cbe1f778a06c0b0f6e95c2c7af0bf4c`. Die Avatar-Anbindung liegt in `firmware/esp32/main`. Ergänzungen für Gerätebetrieb, Konsole und Telemetrie liegen in den zugehörigen Port-Dateien und unter `firmware/core`.

## Bauen und aufs Gerät bringen

1. Vom Projektstamm `bash display/alter-ego/build-preview.sh` ausführen. Die App und `preview.png` liegen anschließend im jeweiligen `build/`-Ordner. Für Mary `alter_ego_mary` als Ordner wählen.
2. ESP-IDF aktivieren und das entsprechende `build-esp32.sh` ausführen. Der bestätigte Build verwendete ESP-IDF 6.1.0 aus der vorhandenen PlatformIO-Installation. Lokale Ersatzpakete lagen auf diesem Mac in `display/alter-ego/build/local-python`. Eine regulär eingerichtete ESP-IDF-Umgebung bringt ihre Python-Abhängigkeiten mit.
3. Zum Flashen in den gewünschten `build/esp32`-Ordner wechseln und den aktuell erkannten USB-Anschluss einsetzen. Auf diesem Mac war die StopWatch beim Test `/dev/cu.usbmodem21401` und enumerierte als Espressif USB Serial/JTAG. Der andere USB-Modemanschluss gehörte zum LG-Monitor.

```sh
python3 -m esptool --chip esp32s3 --port /dev/cu.usbmodem21401 --baud 460800 \
  --before default-reset --after hard-reset write-flash @flash_args
```

`flash_args` stammt aus dem Build und enthält Bootloader, Partitionstabelle, OTA-Initialdaten und App. Beim erfolgreichen Flash wurden alle Schreibvorgänge per Hash bestätigt und anschließend ein Hard-Reset ausgelöst. Für Aussagen über das Bild und den Touch auf der Uhr braucht es zusätzlich eine Beobachtung am Gerät.

## Repo-Pflege

- Die ursprünglichen Git-Metadaten des SDK-Clones und der Patch der lokalen SDK-Änderungen liegen auf dieser SSD unter `.local/`. Im Projekt-Repository wird der SDK-Quellcode direkt versioniert.
- Große generierte Build-Dateien, Python-Umgebungen, ESP-IDF-Komponentencaches und vollständige Flash-Backups bleiben lokal. Das Backup vor der ersten Avatar-Demo liegt auf dieser SSD unter `display/alter-ego/backups/`.
- Nach einer Sitzung mit neuen Erkenntnissen diese Übergabe ergänzen. Bei verändertem Projektstand auch die Status-Tabelle der Haupt-README aktualisieren. Gerätebezogene Tests getrennt von Build- und Flash-Ergebnissen dokumentieren.
- Der nächste geplante Integrationsschritt ist ein Gespräch mit Hermes über lokales WLAN. Dafür am normalen SDK-Profil und dessen [StopWatch-Dokumentation](../inspiration/hermes-gadget-sdk/STOPWATCH.md) ansetzen.

## WLAN für Himmelsgravur

Die vorhandene SDK-WLAN-Verwaltung bleibt für NVS-Zugangsdaten, DHCP und Wiederverbindung zuständig. Die Sprechblase bezieht ihren Status aus dem aktuellen Netz und dem laufenden Verbindungsversuch. Der Start zeigt die Begrüßung, dann den WLAN-Status und nach erfolgreicher Verbindung wieder die Begrüßung. Manuelle Tipps wechseln zwischen Text, Uhrzeit und WLAN.

`display/alter-ego/provision-wifi.py` richtet das Heimnetz über die SDK-Seriellkonsole ein. Das Passwort wird verdeckt am lokalen Terminal eingegeben. Der RTC-Befehl läuft auf derselben Konsole wie die WLAN-Befehle. Das Heimnetz wurde eingerichtet. Nach einem USB-Hard-Reset meldete die StopWatch das gespeicherte Netz erneut als aktiv und lieferte weiterhin die Berliner Ortszeit. Der Offline-Zustand wurde im lokalen Test der Sprechblase geprüft.

## Akkuansicht

`display/alter-ego/battery_drawer.hpp` enthält Geste, Bewegung und Darstellung. Der Avatar leitet eine erfasste Randgeste an die Akkuansicht weiter. Die Firmware reicht den vorhandenen, periodisch aktualisierten `hg::App::power_status()` weiter. Die StopWatch liefert über ihren M5PM1 die Akkuspannung und externe Versorgung. Die Anzeige nutzt die Li-Ion-Kurve des SDK und markiert die Schätzung mit `~`. Die Mac-Vorschau exportiert zusätzlich eine geöffnete Ansicht mit Beispieldaten. Der Build wurde am 21. November verifiziert geflasht. Nach dem Neustart meldete die Uhr das Heimnetz als aktiv und lieferte weiterhin die RTC-Zeit. Felix bestätigte die Akkuansicht am Gerät.

## Energiesparen

`display/alter-ego/idle.hpp` entscheidet anhand der letzten Interaktion zwischen Animation, Standbild nach 5 s und AMOLED aus nach 10 s. Im Standbild werden Statusinhalte weiterhin aktualisiert. Der erste Tipp bei dunklem Display wird als Aufweckgeste verbraucht. Die rote Taste schaltet die Anzeige manuell. Die Himmelsgravur-spezifische `sdkconfig.defaults` aktiviert Tickless Idle, ESP32-S3-Powermanagement und Light-Sleep-Zähler.

Der ESP32 darf bei dunklem Display auf Akku in automatischen Light Sleep wechseln. USB-Verbindung verhindert Light Sleep, damit die Konsole erreichbar bleibt. `avatar-power` meldet Anzeigestufe, Doze-Status und gemessene Schlafphasen. Die drei Anzeigestufen wurden am 21. November nach verifiziertem USB-Flash über die Konsole beobachtet.

Die Reihenfolge der Audiopause und die Ergebnisse der drei Akkutests stehen im [Entwicklungslog](entwicklungslog.md#21-november-2026--energiesparen-und-aufwachen).

Die Löffel haben einen separaten Übergang von 760 ms, ausgelöst durch die Inaktivitätszustände. Während sie nach 5 s aus dem Bild sinken, bleibt der restliche Avatar bereits stehen. Während des Übergangs rendert die Firmware mit rund 20 Bildern pro Sekunde. Beim Aufwecken beginnt der erste Frame mit den Löffeln außerhalb des sichtbaren Bereichs. Die Mac-Vorschau verwendet dieselbe Inaktivitätssteuerung. Die Animation wurde gebaut, mit gerenderten Zwischenbildern geprüft und verifiziert auf die StopWatch geflasht. Die Darstellung auf der Uhr wurde noch nicht visuell geprüft.

Nach vollständigem Ausschalten ohne USB kann die StopWatch ihre RTC-Zeit verlieren. Sobald WLAN eine IP-Adresse liefert, stellt SNTP die Berliner Ortszeit automatisch wieder her. Der [Gerätetest im Entwicklungslog](entwicklungslog.md#21-november-2026--zeitsynchronisation-nach-vollständigem-ausschalten) prüft den Rückschreibvorgang bei zuvor ungültiger RTC.
