# Vagina Squirt — VST3 Synth (Meme Edition)

Absurdes, aber **echtes** Audio-Plugin für Ableton / jede DAW mit VST3.

## Was es macht

- **Synth/Generator** mit kurzen „Splash“-Bursts (Noise → Resonanzfilter → Envelope)
- Regler: **WETNESS**, **PRESSURE**, **SPRAY**, **RATE**
- Großer **SQUIRT**-Button + MIDI-Noten triggern Bursts
- Ausgabe: `Vagina Squirt.vst3`

## Voraussetzungen (Windows)

1. **Visual Studio 2022** — Workload „Desktop development with C++“
2. **CMake** 3.22+ — https://cmake.org/download/
3. **Git** — https://git-scm.com/

JUCE wird beim ersten Build automatisch heruntergeladen (FetchContent).

## Bauen

```powershell
cd C:\Users\User\Projects\VaginaPlugin
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Das `.vst3` liegt danach unter:

```
build\VaginaPlugin_artefacts\Release\VST3\Vagina Squirt.vst3
```

## In Ableton einbinden

1. Ordner `Vagina Squirt.vst3` kopieren nach:
   `C:\Program Files\Common Files\VST3\`
   (oder einen eigenen Ordner, den du in Ableton unter **Preferences → Plug-Ins** hinzufügst)
2. Ableton neu starten oder **Rescan**
3. Unter **Plug-Ins** → **Vagina Squirt** auf eine MIDI-Spur legen

## Bedienung

| Regler   | Wirkung                                      |
|----------|----------------------------------------------|
| Wetness  | Lautstärke / Intensität der Bursts           |
| Pressure | Härte & Länge der Envelope                   |
| Spray    | Filter-/Resonanz-Frequenz (höher = schärfer) |
| Rate     | Auto-Squirts pro Sekunde (0 = aus)           |
| SQUIRT   | Manueller Trigger                            |

MIDI-Noten triggern ebenfalls Bursts — praktisch für Drumpads.

## Standalone testen

Nach dem Build auch verfügbar als:

```
build\VaginaPlugin_artefacts\Release\Standalone\Vagina Squirt.exe
```
