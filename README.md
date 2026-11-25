# Hermes-Agent auf der M5Stack StopWatch

Ziel ist ein kleiner Avatar auf der ESP32-S3-Uhr, über den man mit einem Hermes-Agenten sprechen kann. Die [ursprünglichen Projektziele](goal.md) beschreiben lokales WLAN, Bluetooth LE als Rückfall und optionalen Fernzugriff.

## Projektstand

**25. November 2026 · Meilenstein 4** — Das Avatar-Zifferblatt zeigt lange Hermes-Nachrichten in einer wachsenden, scrollbaren Sprechblase. Die Bedienung ist auf der StopWatch bestätigt. Das Karussell enthält außerdem Pomodoro und Einstellungen.

| Bereich | Status | Nachweis oder offene Arbeit |
| --- | --- | --- |
| Himmelsgravur | Auf der StopWatch installiert | Aktualisierte Firmware per Prüfsumme verifiziert und die Uhr neu gestartet. Die Darstellung wurde in der Mac-Vorschau geprüft. [Renderer und Build](display/alter-ego/) |
| Mary / Piepi | Auf der StopWatch erprobt | Mac-Vorschau gebaut, ESP32-S3-Firmware verifiziert und Funktion auf dem Gerät bestätigt. [Renderer und Build](display/alter_ego_mary/) |
| Sprechblase für Himmelsgravur | Auf der StopWatch installiert | Tipp-Loop und automatische Wiederherstellung der Hardware-Uhr am Gerät bestätigt. Die wachsende, scrollbare Sprechblase wurde per USB aufgespielt und per Prüfsumme verifiziert. Bedienung von Felix am Gerät bestätigt. |
| Automatische WLAN-Verbindung | Auf der StopWatch installiert | Heimnetz über USB eingerichtet. Automatische Verbindung nach einem Neustart der StopWatch bestätigt. |
| Akkuansicht für Himmelsgravur | Auf der StopWatch installiert | Herunterziehen und Schließen am Gerät bestätigt. |
| Energiesparen | Auf der StopWatch installiert | Animation hält nach 5 s an, AMOLED schaltet nach 10 s ab. Aufwecken per Tipp und leiser Akkubetrieb bestätigt. 20,7 s tatsächlicher Light Sleep während einer rund 27 s langen Doze-Periode gemessen. |
| Löffel-Animation beim Einschlafen | Auf der StopWatch installiert | Firmware per Prüfsumme verifiziert. Die Animation wurde in der Mac-Vorschau geprüft. |
| iPhone-Hotspot als WLAN-Rückfall | Firmware auf der StopWatch installiert | Heim-WLAN und iPhone-Hotspot am Gerät bestätigt. Der automatische Wechsel bei Reichweitenverlust und Rückkehr steht als Gerätetest aus. |
| Verbindungsansicht für Himmelsgravur | Auf der StopWatch installiert | Swipe nach links, zwei Netzschalter mit grünem Verbindungsstatus und Netztyp in der Sprechblase. Darstellung des aktiven und inaktiven Schalters auf der Uhr bestätigt. |
| Hermes über lokales WLAN | Texttest auf der StopWatch bestätigt | Festen Testtext im Vault gespeichert, gleiche ID mehrfach bestätigt und genau eine Zeile bis zur Mac-Kopie synchronisiert. Push-to-Talk auf der StopWatch bestätigt. Das Uhrenmikrofon liefert ein Transkript und Hermes antwortet als Text. |
| Pomodoro-Zifferblatt und Karussell | UI-Prototyp auf der StopWatch installiert | 25 und 50 Minuten mit einem umlaufenden Farbverlauf am Displayrand. Die Mac-Vorschau zeigt Projekt und Aufgabe mit Beispieldaten. Flash per Prüfsumme bestätigt. Der Sprachtest für den Hermes-Start steht aus. [Vorschau](display/alter-ego/README.md#pomodoro-und-zifferblatt-karussell) |
| Pomodoro per Sprache | Firmware und Gateway-Plugin installiert | ESP32-Flash und Vault-Schreibpfad geprüft. Der Sprachtest am Gerät steht aus. [Ablauf](codex/README.md#pomodoro-per-sprache) |
| Pomodoro-Abschluss und Pause | Firmware und Hermes-Adapter installiert | Kurzer Dreiklang, 5-/10-Minuten-Pause, Regenerationstipp und von Hermes gesprochener Wiedereinstieg am Pausenende. Countdown-Logik und Vorschau geprüft. [Ablauf](codex/README.md#fokusende-und-regenerationspause) |
| Timeralarm quittieren | Firmware auf der StopWatch installiert | Bei einem SDK-Timeralarm zeigt das Zifferblatt „Timer fertig“ und „Stopp“. Ein Tipp oder die Sprechtaste quittiert den Alarm. Flash per Prüfsumme bestätigt. |
| Lautstärkeregler | Auf der StopWatch installiert | 0–100 %. Auf der Uhr auf 30 % gestellt und gespeicherten Wert per USB bestätigt. Im Karussell-Prototyp liegen die Einstellungen auf Seite 3. |
| Gesprochene Hermes-Antwort | Lautsprecher auf der StopWatch bestätigt | Vollständige Gespräche über Uhrenmikrofon und Lautsprecher auf Deutsch und Englisch bestätigt. |
| Bluetooth LE als Rückfall | Offen | Automatischen Wechsel zu Bluetooth und zurück zu WLAN umsetzen und testen. |
| Gateway-Autostart auf dem Pi | Offen | Dauerhaften Dienst einrichten und Wiederverbindung nach Pi-Neustart testen. |
| Optionaler Fernzugriff | Offen | Hermes über den iPhone-Hotspot und einen verschlüsselten Fernendpunkt erreichbar machen. |

Himmelsgravur rendert die Avatar-Oberfläche in der SDK-Ereignisschleife. Das SDK verwaltet WLAN und Gerätestatus. Die lokale Hermes-Verbindung ist mit einem Obsidian-Texttest bestätigt. Push-to-Talk mit lokaler Transkription und Textantwort ist am Gerät bestätigt. Der Pi-Transkriptionsanbieter wählt zwischen Deutsch und Englisch.

## Einstieg

- [Übergabe für Codex und neue Mitwirkende](codex/README.md)
- [Entwicklungslog mit Hardware-Erkenntnissen und Messwerten](codex/entwicklungslog.md)
- [Himmelsgravur auf Mac und ESP32-S3 bauen](display/alter-ego/README.md)
- [Mary auf Mac und ESP32-S3 bauen](display/alter_ego_mary/README.md)
- [Hinweise für UI-Arbeit](ui_elements/README.md)

Bei Änderungen an Firmware, Gerät oder Projektzielen den Status in dieser README aktualisieren. Reproduzierbare Abläufe und Erkenntnisse stehen in [codex/](codex/README.md). Der Git-Tag `meilenstein-1` markiert den ursprünglichen Stand vom 20. November 2026. Sprechblase, WLAN, Akkuansicht, Energiesparen und Löffel-Animation sind Nachträge zu Meilenstein 1 auf `main`.
