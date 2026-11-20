# Projektübergabe

Stand vom 20. November 2026. Bei einer Übernahme zuerst den [Projektstand](../README.md), die [Projektziele](../goal.md) und [AGENTS.md](../AGENTS.md) lesen. Bei UI-Arbeit gilt zusätzlich [ui_elements/README.md](../ui_elements/README.md).

## Architektur und aktueller Stand

- Die M5Stack StopWatch verwendet einen ESP32-S3, ein rundes 466 × 466 AMOLED mit CO5300 und Touch mit CST820. Der Renderer benötigt den vollständigen Framebuffer und berücksichtigt dessen RGB565-Byte-Reihenfolge.
- Himmelsgravur (`display/alter-ego`) und Mary (`display/alter_ego_mary`) besitzen je einen C++-Renderer in `avatar.hpp`. `preview.mm` verwendet denselben Renderer in einer AppKit-Vorschau. Ziehen dreht das Modell und Drücken erzeugt einen Lichtimpuls.
- Beide ESP32-Buildskripte setzen `HG_ALTER_EGO_DIR`. Das SDK bindet den jeweiligen Header über `firmware/esp32/main/CMakeLists.txt` ein. In `main.cpp` startet die Demo nach Display- und Touch-Initialisierung und übernimmt die Anzeige und Touch-Ereignisse.
- Mary wurde auf der Uhr geflasht und vom Benutzer als funktionierend bestätigt. Danach wurde auf Wunsch wieder Himmelsgravur verifiziert geflasht und das Gerät neu gestartet.
- Der SDK-Quellcode liegt inklusive Lizenz und Hinweisen unter `inspiration/hermes-gadget-sdk`. Upstream ist [allinnb/hermes-gadget-sdk](https://github.com/allinnb/hermes-gadget-sdk) auf Commit `82c079a06cbe1f778a06c0b0f6e95c2c7af0bf4c`. Die projektspezifischen Änderungen befinden sich in den beiden genannten Dateien unter `firmware/esp32/main`.

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
