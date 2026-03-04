# AutoBootWake - Controller-based Boot Selection for Wii U

## Projekt-Übersicht

Dieses Projekt ermöglicht das automatische Booten in unterschiedliche Ziele (vWii / Wii U Menu) basierend auf dem verwendeten Controller.

### Verfügbare Dateien

```
code/
├── v6 final/           # Kompiliertes IPX-Plugin
│   ├── 90ab_w.ipx     # Plugin für klassischen Loader
│   ├── autoboot_wake.c
│   └── Makefile
├── source/             # Quellcode
├── patch/              # Erweiterungen
│   ├── WakeSource.h   # Wake-Source Erkennung für Aroma
│   └── stroopwafel_plugin.c  # ISFShax Plugin (Pseudocode)
└── README.md
```

### Aktueller Status

| Komponente | Status | Beschreibung |
|------------|--------|--------------|
| IPX Plugin | ✅ Fertig | Kompiliert, für klassischen Plugin Loader |
| Wake-Source Erkennung | 🔄 In Arbeit | Code für PRSH/MCP Flags |
| Aroma Integration | 🔄 In Arbeit | WakeSource.h als Erweiterung |
| ISFShax Plugin | ⚠️ Pseudocode | Benötigt weitere Entwicklung |

## Wake-Source Erkennung

### Option 1: Aroma AutobootModule

Die Datei `patch/WakeSource.h` enthält Code zur Wake-Source Erkennung,
der in das AutobootModule von Aroma integriert werden kann.

**Integration:**
1. Klone das AutobootModule: `git clone https://github.com/wiiu-env/AutobootModule`
2. Kopiere `WakeSource.h` in das `source/` Verzeichnis
3. Füge in `main.cpp` hinzu:
   ```cpp
   #include "WakeSource.h"
   
   // In der main() Funktion, nach dem Laden:
   if (isWakeSourceWiimote()) {
       bootSelection = BOOT_OPTION_VWII_SYSTEM_MENU;
   }
   ```

### Option 2: ISFShax Stroopwafel Plugin

Die Datei `patch/stroopwafel_plugin.c` enthält Pseudocode für ein
frühes IOSU-Plugin, das die Wake-Source speichert.

**Hinweis:** Dies erfordert tiefes Wissen über IOSU-Hacking und ist
für die meisten Benutzer nicht empfehlenswert.

## Technische Details

### Wake-Source Speicher-Adressen

- **PRSH Boot Info:** `0x10000000` (MEM2)
- **OSPlatformInfo:** `0x1FFF000` (MEM1)

### Power Flags

| Flag | Bedeutung |
|------|-----------|
| `0x80000000` | Power Button |
| `0x40000000` | Eject Button |
| `0x08000000` | Wake Request 1 (Bluetooth) |
| `0x00000001` | Bluetooth Wake (WiiMote) |

## Kompilierung

```bash
# IPX-Plugin
cd v6\ final
make clean && make

# Ausgabe: 90ab_w.ipx
```

## Installation

### IPX Plugin (Klassisch)
```
sd:/wiiu/plugins/90ab_w.ipx
```

### Config
```
sd:/wiiu/abw.cfg
```

## Nächste Schritte

1. ✅ IPX Plugin kompilieren und testen
2. 🔄 Aroma AutobootModule erweitern
3. 🔄 Wake-Source Erkennung verfeinern
4. ⚠️ ISFShax Plugin entwickeln (optional)

## Danksagung

- devkitPRO/wut für die Wii U Toolchain
- AutobootModule von wiiu-env
- GBAtemp Community für technische Informationen
