# Himmelsgravur als interaktives Alter Ego

Das Punktmodell übernimmt die goldene Silhouette, den geteilten Schnurrbart, die zwei schwebenden Löffel und den Sternenkreis aus der [gewählten Himmelsgravur](../references/07-ausgewaehlte-himmelsgravur.png). Die Punkte liegen auf modellierten Flächen mit unterschiedlichen Tiefen. Bei einer Drehung verschieben sich Stirn, Nase und Bart dadurch unterschiedlich stark.

Der Hintergrund übernimmt den dichteren goldenen Außenring, den helleren blauen Innenring und den Vollmond aus [Mary](../alter_ego_mary/README.md).

## Vorschau auf dem Mac

Aus dem Projektstamm:

```sh
bash display/alter-ego/build-preview.sh
open display/alter-ego/build/Alter-Ego.app
```

Klicken löst einen Lichtimpuls aus. Beim Klicken und Ziehen folgt das Gesicht dem Zeiger. Nach dem Loslassen kehrt es in die Ruhebewegung zurück. Die App zeigt denselben C++-Renderer in 466 × 466 Pixeln und mit derselben RGB565-Byte-Reihenfolge wie der CO5300-Treiber.

## ESP32-S3

In einer eingerichteten ESP-IDF-Shell im Projektstamm:

```sh
bash display/alter-ego/build-esp32.sh
```

Dieser Build aktiviert die Alter-Ego-Demo für die StopWatch. Sie zeigt das Modell dauerhaft und verarbeitet Berührungen vom CST820. Die Demo übernimmt nach Display- und Touch-Initialisierung die Anzeige. Zum üblichen Hermes-Betrieb dient das normale StopWatch-Profil. Der Build-Befehl überträgt keine Firmware auf die Uhr.

Build-Ausgaben, der Komponenten-Cache und gegebenenfalls lokal installierte Python-Pakete liegen in `display/alter-ego/build` auf der EXT_SSD.
