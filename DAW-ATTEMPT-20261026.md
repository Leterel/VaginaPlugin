# Isolierter REAPER-Versuch — 26.10.2026

Vorhandenes REAPER 7.80 Windows x64 mit frischem eigenen Testprofil und
vajayjay 0.1.3 gestartet. Nur eine synthetische 30-Sekunden-Fünf-Ton-WAV,
keine Nutzerprojekte, Wiedergabe oder Aufnahme. Der frühere Fehler beim
Starten wegen System-DLL-Relocation trat in diesem Versuch nicht auf.

Der eigene Prozess blieb jedoch ohne MainWindowHandle und ohne über Computer
Use targetbares REAPER-Fenster aktiv. Auch nach dem begrenzten Startversuch
entstand kein host-results.txt. Kein Rendervergleich oder Editorpass behauptet.
Nur die selbst gestarteten Prozesse wurden nach gespeicherter PID und passendem
Programmpfad beendet. Bestehende DAW-Profile und Projekte blieben unangetastet.

Das ist kein Nachweis für einen Pluginfehler oder eine Behebung des Hoststarts.
Die Ursache der fehlenden Testausführung ist weiter offen. Native SDK-/UI-Tests
von 0.1.3 behalten ihren bisherigen Umfang; sie ersetzen diesen DAW-Test nicht.
Lokaler maschinenlesbarer Befund liegt im aktuellen Chat unter
outputs/20261026/vajayjay/REAPER-attempt.json. Keine neuen Binärdateien gebaut.
