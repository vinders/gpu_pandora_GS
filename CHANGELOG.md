# Changelog
All notable changes will be documented in this file (generated).

## 0.12.1.79
2021-08-13 - 384ae772aa4ed126a80de23d3b24f20893ba9631
### features
vinders (12):
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
### fixes
vinders (1):
* config: refactor file management: create file/path utils

---