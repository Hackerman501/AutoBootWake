# AutoBootWake - Controller-basierte Boot-Auswahl für Wii U

## Projekt-Übersicht

Dieses Projekt ermöglicht das automatische Booten in unterschiedliche Ziele (vWii / Wii U Menu) basierend auf dem verwendeten Controller.

### Unterstützte Funktionen

- **Controller-Erkennung:** Erkennt ob GamePad oder WiiMote verwendet wird
- **Autoboot:** Bootet automatisch nach konfigurierbarer Zeit
- **vWii-Boot:** Bootet bei WiiMote in den Virtual Wii Modus
- **Wii U Menu:** Bootet bei GamePad in das normale Wii U Menu
- **SD-Karten-Config:** Lädt Einstellungen von `abw.cfg`
- **Logging:** Protokolliert Aktionen in `abw.log`

### Aktueller Status

- ✅ C-Code geschrieben und getestet
- ✅ Kompiliert zu `.ipx` (für klassischen Plugin Loader)
- ❌ Boot-Funktionen noch nicht vollständig getestet
- ❌ Nicht ISFSHax-kompatibel (braucht laufendes OS)

## Dateien

```
code/
├── v6 final/           # Kompilierte Version
│   ├── 90ab_w.ipx     # Kompiliertes Plugin
│   ├── autoboot_wake.c
│   └── Makefile
├── source/
│   └── autoboot_wake.c
├── v1/ bis v5/         # Ältere Versionen
└── README.md           # Diese Datei
```

## Konfiguration

Erstelle `abw.cfg` auf der SD-Karte:

```
timeout=5
log=1
wiimote_enable=1
gamepad_enable=1
boot_default=wiiu_menu
boot_gamepad=wiiu_menu
boot_wiimote=vwii
fallback=wiiu_menu
```

### Optionen

- `timeout` - Wartezeit in Sekunden (Standard: 5)
- `log` - Logging aktivieren (0/1, Standard: 1)
- `wiimote_enable` - WiiMote-Boot aktivieren (Standard: 1)
- `gamepad_enable` - GamePad-Boot aktivieren (Standard: 1)
- `boot_default` - Standard Boot-Ziel (Standard: wiiu_menu)
- `boot_gamepad` - Boot-Ziel für GamePad (Standard: wiiu_menu)
- `boot_wiimote` - Boot-Ziel für WiiMote (Standard: vwii)
- `fallback` - Fallback-Ziel (Standard: wiiu_menu)

### Boot-Ziele

- `wiiu_menu` - Wii U System Menu
- `vwii` - Virtual Wii
- `stay` - Im aktuellen Titel bleiben

## Kompilierung

```bash
cd v6\ final
make clean
make
```

## Installation

### Klassischer Plugin Loader

Kopiere `90ab_w.ipx` nach:
- `sd:/wiiu/plugins/` (Plugin Loader)

### Aroma/Tiramisu

Für Aroma wird ein anderes Format benötigt (.rpx/.wuhb). Siehe unten.

## Technische Details

### Verwendete APIs

- `coreinit/filesystem.h` - Dateisystem-Zugriff
- `coreinit/time.h` - Zeitfunktionen
- `coreinit/launch.h` - Titel-Wechsel
- `vpad/input.h` - GamePad-Erkennung
- `padscore/wpad.h` - WiiMote-Erkennung
- `nn/cmpt.h` - vWii-Boot

### Bekannte Einschränkungen

1. Das Plugin startet nach IOSU - nicht für ISFSHax geeignet
2. Wake-Source (welcher Controller bootete) wird NICHT direkt erkannt
3. Es wird der aktuell verbundene Controller geprüft, nicht der Boot-Trigger

## Zukünftige Entwicklung

Für echte Wake-Source-Erkennung (welcher Controller bootete):

1. **ISFSHax-Erweiterung:** MCP-Wake-Flags früh im Boot lesen
2. **Aroma-Modul:** AutobootModule erweitern mit Controller-Erkennung
3. **PRSH-Hook:** Boot-Info aus PRSH auslesen

Siehe Diskussion: https://gbatemp.net/threads/

## Credits

- Basierend auf Wii U Homebrew SDK (devkitPRO/wut)
- Controller-Erkennung via VPAD/WPAD
- Titel-Launch via SYSLaunchTitle/CMPTLaunchTitle
