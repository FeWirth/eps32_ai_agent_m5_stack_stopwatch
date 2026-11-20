# UI-Komponenten und Designreferenzen

Diese Sammlung hilft bei der Auswahl von Komponenten für Weboberflächen und bei der Gestaltung der nativen StopWatch-Oberfläche.

## Quellen nach Aufgabe

| Einsatz | Quelle |
| --- | --- |
| Anpassbare React-Komponenten als Ausgangspunkt für ein konsistentes Web-UI | [shadcn/ui](https://ui.shadcn.com) |
| Ungestylte React-Grundbausteine für Dialoge, Menüs, Popover und andere interaktive Elemente | [Base UI](https://base-ui.com) |
| React-Primitives für barrierearme Interaktionen, insbesondere wenn die Oberfläche bereits Radix verwendet | [Radix](https://www.radix-ui.com) |
| Kurze Statusmeldungen und Benachrichtigungen als Toasts | [Sonner](https://sonner.emilkowal.ski) |
| Drawer und Bottom Sheets mit Touch-Gesten | [Vaul](https://vaul.emilkowal.ski) |
| Durchsuchbare Befehlspalette, etwa für Cmd+K | [cmdk](https://cmdk.paco.me) |
| Animierte Zahlenwechsel für Zähler und Messwerte | [NumberFlow](https://number-flow.barvian.me) |
| Kuratierte Auswahlhilfe für konkrete Frontend-Aufgaben | [Emil Kowalskis Skills](https://github.com/emilkowalski/skills) · [pick-ui-library](https://github.com/emilkowalski/skills/blob/main/skills/pick-ui-library/SKILL.md) |
| Visuelle Referenzen und Beispiele für Komponenten und Layouts | [Beautiful UI](https://beautifului.dev), [RareUI](https://rareui.in) |
| Referenzen für animierte Komponenten und visuelle Effekte | [Magic UI](https://magicui.design), [Aceternity UI](https://ui.aceternity.com) |
| Community-Komponenten und Beispiele für konkrete UI-Muster | [21st.dev](https://21st.dev) |

## Welche Angaben bei einer UI-Aufgabe helfen

- **Zieloberfläche und Einbauort** – Weboberfläche, Mac-Vorschau oder native StopWatch-Firmware. Wenn bekannt, den Screen, die Route oder die Datei nennen.
- **Aufgabe und Verhalten** – Was soll die Person tun können? Welche Zustände sind wichtig, etwa Laden, Fehler, leere Ansicht oder Erfolg? Welche Eingaben werden benötigt, etwa Touch, Maus oder Tastatur?
- **Gestaltung** – Ein Screenshot, ein Komponentenlink oder ein kurzer Hinweis zum gewünschten Stil reicht als Referenz. Bei Animationen helfen Angaben zu Auslöser und gewünschtem Ablauf.
- **Technische Vorgaben** – Framework und Styling, vorhandene Komponenten, erlaubte Abhängigkeiten sowie Anforderungen an Barrierefreiheit oder Leistung. Was sich aus dem Projekt ergibt, kann dort nachgelesen werden.

## Vorgehen bei der Umsetzung

1. Zielplattform, vorhandene Komponenten und Styling im relevanten Projektteil prüfen. Bei Webcode auch Paketliste und Lockfile ansehen und vorhandene Bausteine bevorzugt wiederverwenden.
2. Nach Aufgabe in der Tabelle auswählen. Vor Einbau oder Übernahme die konkrete Komponentendokumentation, Versionskompatibilität, Abhängigkeiten und Lizenz prüfen. Bei Community-Beispielen gilt das für den jeweiligen Eintrag.
3. Für die native ESP32-Oberfläche die Quellen als Referenz für Layout, Zustände, Gesten und Bewegung nutzen. Die Umsetzung erfolgt im vorhandenen nativen Display-Code. Für das runde 466 × 466-Display Platzierung und Touch-Ziele in der Vorschau prüfen.
