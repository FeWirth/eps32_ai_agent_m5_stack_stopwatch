# Mary · Piepi als interaktives Alter Ego

Die Piepi-Silhouette ist nach der Kartenillustration als räumliches Punktmodell aufgebaut. Die erhobenen Arme, die dunklen Ohrspitzen und die Stirnlocke stehen vor einem Sternenkreis mit Vollmond. Das C++-Modell in `avatar.hpp` rendert für beide Plattformen in 466 × 466 Pixeln mit derselben RGB565-Ausgabe.

## Mac-Vorschau

Aus dem Projektstamm:

```sh
bash display/alter_ego_mary/build-preview.sh
open display/alter_ego_mary/build/Alter-Ego-Mary.app
```

Ein Klick löst einen Lichtimpuls aus. Beim Ziehen dreht sich das Modell zum Zeiger. Nach dem Loslassen kehrt es in die Ruhebewegung zurück. Der Build exportiert auch `build/preview.png`.

## ESP32-S3 mit CO5300-AMOLED und CST820-Touch

Aus dem Projektstamm mit aktiviertem ESP-IDF:

```sh
bash display/alter_ego_mary/build-esp32.sh
```

Das Skript wählt über `HG_ALTER_EGO_DIR` den Mary-Renderer für die vorhandene StopWatch-Demo aus. Der Build schreibt die Firmware nach `build/esp32` im Mary-Ordner. Das Skript führt ausschließlich den Build aus.
