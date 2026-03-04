# AutoBootWake - Controller-basierte Boot-Auswahl

## Projekt-Status

### ✅ Fertig

**IPX-Plugin (v6 final):**
- Kompiliert zu `90ab_w.ipx` (~178KB)
- Für klassischen Wii U Plugin Loader / ISFSHax ios_plugins
- Controller-Erkennung (GamePad vs WiiMote)
- Config von SD-Karte
- Logging

**Source-Code:**
- `v6 final/autoboot_wake.c` - Hauptcode
- `source/autoboot_wake.c` - Backup

### ⚠️ Nicht vollständig

**Fehlende Funktionen:**
- Wake-Source Erkennung (welcher Controller bootete) - NICHT der aktuelle Controller
- Die `boot_to_target()` Funktionen sind eingebaut aber ungetestet

## Aktuelle限制en

1. **ISFSHax:** Das Plugin startet zu spät - nach IOSU. Die Wake-Source Info ist zu diesem Zeitpunkt nicht mehr verfügbar.

2. **Aroma:** Für Aroma wird ein anderes Format (RPX/WUHB) und ein komplexeres Build-System benötigt. Das AutobootModule verwendet C++ undWUMS.

## Nächste Schritte für echte Wake-Source-Erkennung

Basierend auf der GBAtemp-Diskussion:

1. **PRSH/MCP Flags lesen** - Die Wake-Info ist in PRSH verfügbar ("You should be able to get the boot info from PRSH at any point")

2. **AutobootModule modifizieren** - Das Aroma AutobootModule erweitern statt ein neues Modul zu schreiben

3. **IOSU Plugin (optional)** - Ein frühes Stroopwafel-Plugin das die Wake-Info speichert, welche dann von Aroma gelesen wird

## Installations-Pfade

### IPX (Plugin Loader)
```
sd:/wiiu/plugins/90ab_w.ipx
```

### Config
```
sd:/wiiu/abw.cfg
```

### Log
```
sd:/wiiu/abw.log
```

## Config-Optionen

```
timeout=5           # Wartezeit in Sekunden
log=1               # Logging aktivieren
wiimote_enable=1   # WiiMote-Boot aktivieren
gamepad_enable=1   # GamePad-Boot aktivieren
boot_default=wiiu_menu
boot_gamepad=wiiu_menu
boot_wiimote=vwii
```

## Kompilierung

```bash
# IPX-Version
cd v6\ final
make clean && make

# Ausgabe: 90ab_w.ipx
```

## Technische Details

- Verwendet VPAD für GamePad-Erkennung
- Verwendet WPAD/KPAD für WiiMote-Erkennung  
- Titel-Wechsel via SYSLaunchMenu() / CMPTLaunchTitle()
- Konfiguration via abw.cfg auf SD

## Was funktioniert

- ✅ Code kompiliert
- ✅ Controller-Erkennung
- ✅ Config-Parsing
- ✅ Logging
- ✅ Timeout-Warteschleife
- ⚠️ Boot-Funktion (theoretisch, ungetestet)

## Was NICHT funktioniert / nicht geht

- ❌ Wake-Source Erkennung (nur aktueller Controller)
- ❌ ISFSHax-kompatibel (startet zu spät)
- ❌ Aroma-kompatibel (falsches Format)
