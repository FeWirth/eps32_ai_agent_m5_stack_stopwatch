# Entwicklungslog der M5Stack StopWatch

## 20.–21. November 2026 · Hardware, Build und Flash

| Bauteil | Erkenntnis für die Firmware |
| --- | --- |
| ESP32-S3R8 | Die StopWatch nutzt 16 MB Flash und 8 MB OPI-PSRAM. Das StopWatch-Boardprofil und die zugehörige Partitionstabelle beim Build beibehalten. |
| CO5300-AMOLED | Der runde Ausschnitt hat 466 × 466 Pixel. Der Avatar braucht einen vollständigen RGB565-Framebuffer und übernimmt die Byte-Reihenfolge vom Treiber. Die AppKit-Vorschau rendert denselben C++-Code. Der Treiber schaltet mit `set_backlight(0)` den Panel-Ausgang aus und behält den Bildspeicher. |
| CST820-Touch | Der aktuelle StopWatch-Treiber pollt den Controller. Beim Loslassen können die Koordinaten null sein. Für Tipp und Wisch deshalb die Bewegung aus den Samples bei gedrücktem Finger ableiten. |
| M5PM1 | Der PMIC liefert Akkuspannung und externe Versorgung. Der Akkubildschirm berechnet mit der Li-Ion-Kurve des SDK eine Schätzung (`~`). „USB verbunden“ zeigt erkannte externe Versorgung an. |
| M5IOE1 und Audio | Der I/O-Expander steuert unter anderem den AW8737A-Verstärker über PYG10. Der ES8311 verwendet gemeinsame I²S-Takte für Mikrofon und Lautsprecher. |
| RX8130CE | Die Hardware-Uhr liegt bei I²C-Adresse `0x32`. Sie speichert die über USB gesetzte Berliner Ortszeit. |

- ESP-IDF 6.1.0 mit der vorhandenen PlatformIO-Toolchain hat die StopWatch-Builds erzeugt. `flash_args` aus dem jeweiligen Build-Verzeichnis enthält die Adressen für Bootloader, Partitionstabelle, OTA-Initialdaten und App.
- Der USB-Serial/JTAG-Port war auf diesem Mac `/dev/cu.usbmodem21401`. Ein weiterer `usbmodem`-Port gehörte zum LG-Monitor. Vor dem Flashen das Espressif-Gerät zuordnen.
- Esptool hat die geschriebenen Flash-Abschnitte per Hash geprüft und anschließend einen Hard-Reset ausgelöst. Die Mac-Vorschau eignet sich zur Layout-Kontrolle. Display und Touch auf der Uhr werden separat am Gerät geprüft.

## 21. November 2026 · Uhrzeit, WLAN und Akkuansicht

- Die frisch gesetzte Uhrzeit ging zunächst bei einem Neustart verloren. Seit die Firmware Pufferumschaltung und Ladung des RTC-Backups aktiviert (INIEN und CHGEN), konnte die Berliner Ortszeit nach einem USB-Hard-Reset ausgelesen werden. Felix bestätigte sie auch nach seinem Neustart. `display/alter-ego/set-clock.py --read` prüft die Uhrzeit ohne Schreibvorgang.
- Die ursprüngliche Avatar-Schleife verarbeitete nur Touch. Himmelsgravur rendert jetzt in der regulären SDK-Ereignisschleife. Dadurch verarbeitet das SDK auch WLAN-Events, DHCP und Wiederverbindungsversuche. Der Avatar zeigt „WLAN: Verbunden“, nachdem die Station eine IP-Adresse erhalten hat.
- Das Heimnetz wurde einmalig über die USB-Konsole eingerichtet. Nach einem USB-Hard-Reset meldete die Uhr das gespeicherte Netz automatisch wieder als aktiv. Der Offline-Zustand der Sprechblase wurde lokal simuliert. Ein Wiederverbindungstest nach Verlassen der Funkreichweite steht aus.
- Felix bestätigte die Akkuansicht mit Wisch vom oberen Rand auf der StopWatch. Die Mac-Vorschau zeigt dafür Beispieldaten. Ab Firmware-Build liefert das Gerät seine tatsächliche Akkuspannung und die erkannte externe Versorgung.

## 21. November 2026 · Energiesparen und Aufwachen

- Die Gerätekonsole meldete nach dem Flashen die Zustandsfolge `animated → still → dark`. Die Animation hält nach 5 s ohne Eingabe an. Nach 10 s schaltet das AMOLED ab. Der erste Tipp weckt die Uhr und lässt den Inhalt der Sprechblase stehen.
- Automatischer Light Sleep wird bei abgeschaltetem Display auf Akku freigegeben. Der USB-Serial/JTAG-Schutz hält die Konsole erreichbar, solange der Mac per USB verbunden ist. Zum Testen USB abziehen, ca. 30 s warten, per Tipp aufwecken, USB wieder anschließen und `avatar-power` abfragen.
- Erster Akkutest mit aktiven I²S-Kanälen: Der ESP32 meldete Doze, die Konsole zählte null Schlafphasen. Die dauerhaft aktiven I²S-Kanäle hielten eine Power-Management-Sperre.
- Zweiter Akkutest mit pausiertem I²S: 18,1 s Light Sleep in 1.198 Phasen. Die Uhr wachte auf, surrte aber bei dunklem Display. Im Log versuchte der Mikrofontask wiederholt, den abgeschalteten Kanal zu lesen.
- Korrigierter Build: Vor I²S-Aus den AW8737A-Verstärker deaktivieren und den Mikrofontask parken. Beim Aufwachen zuerst die I²S-Takte starten, dann Mikrofontask und Verstärker freigeben.
- Abschließender Akkutest: Felix bestätigte leisen Betrieb und Aufwecken per Tipp. Per USB wurden **20.659 ms Light Sleep in 1.601 Phasen** ausgelesen, rund **77 % der Doze-Periode**. WLAN und RTC waren danach verfügbar. Für die tatsächliche Stromersparnis fehlt eine separate Verbrauchsmessung.

## 21. November 2026 · Löffel-Animation

- Nach 5 s Inaktivität sinken beide Löffel über 760 ms aus dem runden Ausschnitt. Der übrige Avatar steht währenddessen bereits still. Beim Aufwecken beginnt der erste Frame mit abgesenkten Löffeln und hebt sie wieder an.
- Mac-Vorschau, gerenderte Zwischenbilder und ESP32-S3-Build sind geprüft. Die Animation wurde noch nicht auf die Uhr geflasht. Dort läuft der bestätigte Energiespar-Build.
