# Himmelsgravur als interaktives Alter Ego

Das Punktmodell übernimmt die goldene Silhouette, den geteilten Schnurrbart, die zwei schwebenden Löffel und den Sternenkreis aus der [gewählten Himmelsgravur](../references/07-ausgewaehlte-himmelsgravur.png). Die Punkte liegen auf modellierten Flächen mit unterschiedlichen Tiefen. Bei einer Drehung verschieben sich Stirn, Nase und Bart dadurch unterschiedlich stark.

Der Hintergrund übernimmt den dichteren goldenen Außenring, den helleren blauen Innenring und den Vollmond aus [Mary](../alter_ego_mary/README.md).

## Vorschau auf dem Mac

Aus dem Projektstamm:

```sh
bash display/alter-ego/build-preview.sh
open display/alter-ego/build/Alter-Ego.app
```

Beim Start steht „Hi, Felix!“ in der Sprechblase. Kurze Klicks lösen einen Lichtimpuls aus und wechseln zwischen Text, lokaler Mac-Uhrzeit und simuliertem WLAN-Status. Beim Klicken und Ziehen folgt das Gesicht dem Zeiger. Nach dem Loslassen kehrt es in die Ruhebewegung zurück. Die App zeigt denselben C++-Renderer in 466 × 466 Pixeln und mit derselben RGB565-Byte-Reihenfolge wie der CO5300-Treiber.

## ESP32-S3

In einer eingerichteten ESP-IDF-Shell im Projektstamm:

```sh
bash display/alter-ego/build-esp32.sh
```

Dieser Build aktiviert die Alter-Ego-Demo für die StopWatch. Sie zeigt das Modell dauerhaft und verarbeitet Berührungen vom CST820. Die Demo übernimmt nach Display- und Touch-Initialisierung die Anzeige. Zum üblichen Hermes-Betrieb dient das normale StopWatch-Profil. Der Build-Befehl überträgt keine Firmware auf die Uhr.

Build-Ausgaben, der Komponenten-Cache und gegebenenfalls lokal installierte Python-Pakete liegen in `display/alter-ego/build` auf der EXT_SSD.

## Sprechblase

Kurze Tipps wechseln zwischen Textausgabe, Uhrzeit und WLAN-Status. Beim Start erscheint „Hi, Felix!“, gefolgt vom WLAN-Status. Sobald die Uhr eine IP-Adresse hat, erscheint kurz „WLAN: Verbunden“ und anschließend wieder der Begrüßungstext. Ein Tipp während der Startfolge wechselt direkt in den normalen Tipp-Loop. Mit WLAN synchronisiert die StopWatch die Zeit per SNTP über `pool.ntp.org`. Die Firmware rechnet auf Berliner Ortszeit mit Sommerzeit um und schreibt sie in die RX8130CE-Echtzeituhr zurück. Beim Start ohne Netz dient die Hardware-Uhr als Zeitquelle. Falls sie ihre Zeit verloren hat, erscheint bis zum nächsten Abgleich „Uhr nicht gestellt“.

Für weitere Funktionen setzt `avatar.speech().show_message(text)` den Inhalt. Die Sprechblase bricht Text auf bis zu zwei Zeilen um und kürzt Überlänge mit Auslassungspunkten. Die Plattform liefert die Zeitquelle separat über `SpeechBubble::update`.

### Ortszeit über USB einstellen

Mit laufender Himmelsgravur-Firmware und installiertem `pyserial` vom Projektstamm ausführen:

```sh
python3 display/alter-ego/set-clock.py /dev/cu.usbmodem21401 --zone Europe/Berlin
```

Den tatsächlichen USB-Port einsetzen. Mit `--read` lässt sich der RTC auslesen. Die Firmware aktiviert die RTC-Pufferung der StopWatch. Das Skript verwendet die aktuelle Host-Zeit, rechnet sie in die gewählte Zeitzone um und wartet auf die Bestätigung der Uhr. Der RTC speichert Ortszeit. Bei dauerhaftem Offline-Betrieb kann die Ortszeit nach einem Sommerzeitwechsel über USB neu gesetzt werden.

### Heimnetz über USB einrichten

Mit laufender Firmware und installiertem `pyserial`:

```sh
python3 display/alter-ego/provision-wifi.py /dev/cu.usbmodem21401 Zweibahnstrasse
```

Das Skript fragt das Passwort verdeckt am lokalen Terminal ab. Das SDK speichert die Zugangsdaten im NVS der StopWatch und nutzt sie für automatische Verbindungsversuche beim Start und nach einem Abbruch. `--status` gibt die gespeicherten Netzwerke und die aktive Verbindung aus.

## Akkustand

Am goldenen Griff oben nach unten wischen. Die Akkuansicht folgt der Geste und rastet nach dem Loslassen ein. Nach oben wischen schließt sie. In der Mac-Vorschau dieselbe Bewegung mit gedrückter Maustaste ausführen. Der Build exportiert `build/battery-preview.png` mit Beispieldaten.

Der Regler des M5PM1 liefert die Akkuspannung. Aus ihr schätzt das SDK den Ladestand, gekennzeichnet durch `~`. Bei erkannter externer Versorgung erscheint „USB verbunden“.

## Inaktivität und Light Sleep

Nach 5 Sekunden ohne Eingabe hält die Avatar-Animation an und Simsala senkt die Löffel in einem 760-ms-Übergang aus dem runden Bild. Beim Aufwecken hebt er die Löffel wieder an. Die Mac-Vorschau bildet die gleichen Zeitgrenzen und Übergänge ab. Uhrzeit und WLAN-Status können sich weiter ändern. Nach 10 Sekunden schaltet der CO5300 das Display aus. Ein Tipp weckt die Anzeige, ohne gleichzeitig die Sprechblase umzuschalten. Die rote Taste schaltet die Anzeige auch manuell aus oder ein.

Bei dunklem Display und im Akkubetrieb darf der ESP32 automatisch in Light Sleep gehen. USB hält die Konsole erreichbar. Über die USB-Konsole zeigt `avatar-power` den Anzeigemodus sowie Dauer und Anzahl der Light-Sleep-Phasen der letzten Schlafperiode.
