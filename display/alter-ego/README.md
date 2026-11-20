# Himmelsgravur als interaktives Alter Ego

Das Punktmodell übernimmt die goldene Silhouette, den geteilten Schnurrbart, die zwei schwebenden Löffel und den Sternenkreis aus der [gewählten Himmelsgravur](../references/07-ausgewaehlte-himmelsgravur.png). Die Punkte liegen auf modellierten Flächen mit unterschiedlichen Tiefen. Bei einer Drehung verschieben sich Stirn, Nase und Bart dadurch unterschiedlich stark.

Der Hintergrund übernimmt den dichteren goldenen Außenring, den helleren blauen Innenring und den Vollmond aus [Mary](../alter_ego_mary/README.md).

## Vorschau auf dem Mac

Aus dem Projektstamm:

```sh
bash display/alter-ego/build-preview.sh
open display/alter-ego/build/Alter-Ego.app
```

Beim Start steht „Hi, Felix!“ in der Sprechblase. Ein kurzer Klick löst einen Lichtimpuls aus und zeigt anschließend die lokale Mac-Uhrzeit im 24-Stunden-Format. Beim Klicken und Ziehen folgt das Gesicht dem Zeiger. Nach dem Loslassen kehrt es in die Ruhebewegung zurück. Die App zeigt denselben C++-Renderer in 466 × 466 Pixeln und mit derselben RGB565-Byte-Reihenfolge wie der CO5300-Treiber.

## ESP32-S3

In einer eingerichteten ESP-IDF-Shell im Projektstamm:

```sh
bash display/alter-ego/build-esp32.sh
```

Dieser Build aktiviert die Alter-Ego-Demo für die StopWatch. Sie zeigt das Modell dauerhaft und verarbeitet Berührungen vom CST820. Die Demo übernimmt nach Display- und Touch-Initialisierung die Anzeige. Zum üblichen Hermes-Betrieb dient das normale StopWatch-Profil. Der Build-Befehl überträgt keine Firmware auf die Uhr.

Build-Ausgaben, der Komponenten-Cache und gegebenenfalls lokal installierte Python-Pakete liegen in `display/alter-ego/build` auf der EXT_SSD.

## Sprechblase

Kurze Tipps wechseln zwischen der allgemeinen Textausgabe und der Uhrzeit. Beim Start ist die Textausgabe mit „Hi, Felix!“ aktiv. Die Anzeige aktualisiert sich jede Sekunde aus der vorhandenen RX8130CE-Echtzeituhr. Sie übernimmt deren gespeicherte Uhrzeit ohne Zeitzonenumrechnung. Ist die Uhr gestoppt, ungültig oder nicht lesbar, erscheint „Uhr nicht gestellt“.

Für weitere Funktionen setzt `avatar.speech().show_message(text)` den Inhalt. Die Sprechblase bricht Text auf bis zu zwei Zeilen um und kürzt Überlänge mit Auslassungspunkten. Die Plattform liefert die Zeitquelle separat über `SpeechBubble::update`.

### Ortszeit über USB einstellen

Mit laufender Himmelsgravur-Firmware und installiertem `pyserial` vom Projektstamm ausführen:

```sh
python3 display/alter-ego/set-clock.py /dev/cu.usbmodem21401 --zone Europe/Berlin
```

Den tatsächlichen USB-Port einsetzen. Mit `--read` lässt sich der RTC auslesen. Die Firmware aktiviert die RTC-Pufferung der StopWatch. Das Skript verwendet die aktuelle Host-Zeit, rechnet sie in die gewählte Zeitzone um und wartet auf die Bestätigung der Uhr. Der RTC speichert Ortszeit. Nach einem Sommerzeitwechsel den Abgleich erneut ausführen.
