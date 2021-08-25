# Changelog
All notable changes will be documented in this file (generated).

## 0.20.1.114
2021-08-25 - 76bd26f404afbaf798a6f0416a4e1568cf5c7ba8
### features
vinders (20):
* psemu: add PSEmu dynamic library interface functions
* display: font-map manager + character descriptor
* display: create font descriptor generation tool (CSV to binary) - useful to regenerate for big-endian systems
* display: font-map: add resource loader (descriptor / image files)
* config: add config types/enums + config/profile data containers + profile labels/tiles
* config: add JSON config serializer/deserializer + config dir detection + game binding reader/writer
* config: config profile preset loader
* display: viewport size calculator (based on mode/stretch/crop/source)
* display: output window builder & config/resource container
* config: emulator info detection (process name + emulator type)
* psemu: system logger (for error messages & warnings)
* psemu: sync timer (framerate limit + skipping) + speed modes
* psemu: read game ID + load config profile associated with game
* psemu: plugin 'about' message box
* config: detect emulator 'widescreen hack' settings (epsxe/pcsxr)
* display: status/control register & rendering attributes management + DMA status + display state + texture page/window state + light-cursor status
* psemu: add delay-to-start-limiter to speedup game startup
* psemu: emulator save-state management: load/save/select
* psemu: toggle framerate limit from emulator
* display: DMA-chain iterator (with endless loop protection)
### fixes
vinders (2):
* config: refactor file management: create file/path utils
* config: refactor emulator options detection + detect if cursor is auto-hidden

---