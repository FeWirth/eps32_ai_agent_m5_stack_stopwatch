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
- Mac-Vorschau, gerenderte Zwischenbilder und ESP32-S3-Build sind geprüft. Die Firmware mit Animation wurde verifiziert auf die Uhr geflasht. Nach dem Neustart waren Heimnetz und RTC-Zeit verfügbar. Die Animation wurde auf der Uhr noch nicht visuell geprüft.

## 21. November 2026 · Zeitsynchronisation nach vollständigem Ausschalten

Nach einer längeren Ausschaltphase ohne USB meldete die RX8130CE im USB-Test „unset“, obwohl sie frühere USB-Hard-Resets überstanden hatte. Die Ursache des Zeitverlusts bei abgeschaltetem Gerät wurde offen gelassen.

Die Firmware startet SNTP nach dem Erhalt einer WLAN-IP-Adresse und bei Wiederverbindung erneut. Sie liefert Berliner Ortszeit aus der synchronisierten Systemuhr und schreibt diese in die Hardware-Uhr zurück. Bei einem Offline-Start bleibt der RX8130CE die Zeitquelle, solange seine Daten gültig sind.

Test auf derselben StopWatch ohne manuelles Stellen: RTC vor dem Flash ungültig. Nach dem Neustart protokollierte die Firmware nach etwa 6,7 s `SNTP Berlin 02:12:46; RTC verified`. Eine nachfolgende USB-Abfrage las `02:13` direkt aus der Hardware-Uhr.

## 22. November 2026 · iPhone-Hotspot als WLAN-Rückfall

Himmelsgravur behält die gespeicherte WLAN-Reihenfolge bei. Nach der Verbindung mit einem Rückfallnetz bleibt das erste Netz das bevorzugte Heimnetz. Im Rückfallnetz sucht die Uhr außerhalb eines laufenden Gesprächs im Abstand von 20 s nach dem Heimnetz. Bei zwei aufeinanderfolgenden Sichtungen mit mindestens −75 dBm kehrt sie dorthin zurück. Die übrigen SDK-Profile behalten ihre bisherige WLAN-Auswahl.

Der Host-Test für gespeicherte WLANs und der ESP32-S3-Build liefen durch. Auf der StopWatch wurde die Firmware per Flash-Prüfsumme verifiziert. Nach dem Neustart bestätigte die USB-Konsole das gespeicherte Heimnetz als verbunden. Zum Gerätetest mit dem iPhone fehlen noch dessen lokal eingegebene Hotspot-Daten.

Der Hotspot wurde mit verdeckter Passworteingabe über USB gespeichert. Beim manuellen Verbindungstest meldete die Uhr den iPhone-Hotspot erstmals als geprüft und aktiv. Danach wurden Heimnetz-Priorität und Heimnetz-Verbindung wiederhergestellt. Der automatische Wechsel bei Reichweitenverlust und Rückkehr bleibt als eigener Gerätetest offen.

## 22. November 2026 · Manuelle WLAN-Auswahl

Die zweite Himmelsgravur-Ansicht nutzt den nativen Canvas. Ein horizontaler Wisch öffnet oder schließt die Ansicht. Die Schalter fixieren exklusiv das erste beziehungsweise zweite gespeicherte Netz bis zum erneuten Antippen oder Neustart. Die Schalterstellung zeigt auch in der Automatik das aktive Netz (rechts, grün). Ein goldener Rahmen markiert die manuelle Auswahl. Der tatsächliche Linkstatus wird erst nach der IP-Meldung als verbunden angezeigt. Die Sprechblase nennt den Netztyp.

Mac-Vorschau, Skript-Gestentest und ESP32-S3-Build waren erfolgreich. Die neue Firmware wurde per Flash-Prüfsumme verifiziert. Nach Neustart bestätigte die USB-Konsole Heim-WLAN und beide gespeicherten Netze. Der Touch-Test auf der Uhr steht aus.

Felix bestätigte auf der StopWatch die korrigierte Schalteranzeige mit grünem Schalter rechts für das aktive und grauem Schalter links für das inaktive Netz.

## 23. November 2026 · StopWatch → Hermes → Obsidian

Auf der geflashten StopWatch wurde „Vault gespeichert“ nach Druck auf den gelb-orangen Knopf bestätigt. Die Bestätigung wird separat behandelt, damit sie keinen normalen Gesprächszug belegt. Nach dem erneuten Flash bestätigte die echte StopWatch zwei direkt aufeinanderfolgende USB-Auslösungen mit derselben ID. Der Git-Diff des privaten Vaults enthielt genau eine zusätzliche Logzeile mit Berliner Uhrzeit 04:18, Projektlink und Herkunft. Vault-Commit `df0e803` wurde vom Pi gepusht und auf dem Mac per Fast-Forward synchronisiert. Mikrofonaufnahme und Transkription folgen später.

## 23. November 2026 · Push-to-Talk und lokale Transkription

Der ESP32-S3-Build wurde per Flash-Prüfsumme verifiziert. Danach war die StopWatch wieder online und gepaart. Der Pi transkribierte die synthetische deutsche Testaufnahme vollständig mit faster-whisper Base. Der physische Tastentest über GPIO2 bestätigte 3,9 s Aufnahme vom Uhrenmikrofon. Die StopWatch meldete das empfangene Transkript 5,1 s nach Loslassen und die finale Hermes-Textantwort nach 10,9 s. Die USB-Diagnose protokollierte die Ereignisse und Textlängen.

Felix bestätigte die angezeigte Begrüßung auf der Uhr. Der lokale Whisper-Anbieter wählt jetzt pro Aufnahme zwischen Deutsch und Englisch. Beide Sprachen wurden auf dem Pi mit synthetischen Testaufnahmen erfolgreich erkannt.

## 23. November 2026 · Piper auf dem Uhrenlautsprecher

Beide lokalen Piper-Stimmen erzeugten gültiges Mono-WAV mit 22,05 kHz. Das SDK spielte die deutsche Hermes-Testantwort auf der Uhr ab. Die Codec-Diagnose meldete 126 720 abgespielte Bytes, keine Underruns und keine verworfenen Bytes. Felix hörte die Antwort, meldete die Qualität aber als zu leise oder abgehackt. Der Gerätepegel wurde von der Standardstufe 70 auf 80 erhöht. Eine englische Ausgabe wurde als WAV auf dem Pi geprüft und steht am Uhrenlautsprecher noch aus.

## 23. November 2026 · Lautstärkeregler auf der StopWatch

Die zweite Watchface-Seite trägt den Titel Einstellungen und zeigt unter den WLAN-Schaltern den Lautstärkeregler. Die Ausgabe ändert sich beim Ziehen. NVS wird erst beim Loslassen beschrieben. Der Regler verwendet dieselbe Lautstärke wie USB-Konsole und Hermes-Aktion. Mac-Vorschau und Host-Gestentest prüften die Endwerte 0 und 100, Loslassen außerhalb der Leiste und den Wisch zurück. Der ESP32-S3-Build wurde per Flash-Prüfsumme verifiziert.

Felix zog den Regler am Gerät auf 30 %. Die USB-Konsole las anschließend denselben Wert aus der gespeicherten Einstellung.

## 23. November 2026 · Jarvis High

Piper nutzt auf dem Pi die deutsche Jarvis-High-Variante von ufozone und die britisch-englische Variante von jgkawell. Beide Modelle erzeugten gültiges Mono-WAV mit 22,05 kHz. Die StopWatch spielte zwei Hermes-Testantworten ab. Der Codec meldete 132 480 beziehungsweise 144 000 abgespielte Bytes ohne Underruns oder verworfene Audiodaten. Der Wechsel erfolgte im vorhandenen Piper-Anbieter auf dem Pi.

Felix bestätigte beide Jarvis-High-Stimmen am Uhrenlautsprecher.


## 23. November 2026 · Meilenstein 2

Felix bestätigte ein vollständiges englisches Gespräch über das Uhrenmikrofon. Damit sind Sprachaufnahme, Whisper-Transkription, Hermes-Antwort und Ausgabe mit Jarvis High im Heim-WLAN auf Deutsch und Englisch am Gerät geprüft. Der Lautstärkeregler speichert den gewählten Pegel.

Offen für den Alltag:

- **Pi-Autostart** – Den temporären Hermes-Gateway durch einen dauerhaft eingerichteten Dienst ersetzen und die erneute Verbindung der Uhr nach einem Pi-Neustart prüfen.
- **Fernzugriff unterwegs** – Der iPhone-Hotspot wurde als WLAN getestet. Die Uhr braucht noch einen verschlüsselten Hermes-Fernendpunkt. Danach ein vollständiges Gespräch über Mobilfunk prüfen.
