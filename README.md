# Imager.Plugins

Hardware plugins for the [Imager](https://github.com/ImagerMicroscopy/Imager.Core) microscopy backend.

Each plugin is built as a self-contained `.imagerplugin` library that the Imager backend loads at runtime to drive a specific piece of equipment — a camera, a motorized microscope body, a stage, a filter wheel, a laser combiner, and so on.

## Contents

- [Available plugins](#available-plugins)
- [Building from source](#building-from-source)
- [Prebuilt binaries (Releases)](#prebuilt-binaries-releases)
- [Installing a plugin](#installing-a-plugin)

## Available plugins

| Plugin | Type | Hardware |
| --- | --- | --- |
| `DummyCamera` | Camera | Simulated camera (testing) |
| `NikonTi2` | Microscope body | Nikon Ti2 |
| `NikonTiE` | Microscope body | Nikon Ti-E |
| `OlympusIX83` | Microscope body | Olympus IX83 |
| `OlympusRTC` | Microscope body | Olympus Real-Time Controller |
| `MarzhauserTango` | Stage | Märzhäuser Tango |
| `PriorProScanIII` | Stage | Prior ProScan III |
| `Lambda10B` | Filter wheel | Sutter Lambda 10-B |
| `OmicronLedHub` | Illumination | Omicron LED Hub |
| `OxxiusCombiner` | Laser | Oxxius laser combiner |
| `VisiTIRF` | Illumination | Visitron TIRF |
| `DummyEquipment` | Equipment | Simulated equipment (testing) |

## Building from source

**Requirements:** [Meson](https://mesonbuild.com/) + [Ninja](https://ninja-build.org/), and a 64-bit C++20 toolchain (MSVC on Windows). Subproject dependencies are fetched automatically by Meson.

```sh
meson setup build --buildtype=release
cd build
meson compile
```

Each plugin is emitted next to its name, e.g. `build/OlympusIX83/OlympusIX83.imagerplugin`.

> **Note:** the Windows-only plugins (`NikonTi2`, `NikonTiE`, `OlympusIX83`, `OlympusRTC`, `VisiTIRF`) are only built on Windows.

## Prebuilt binaries (Releases)

Every push to `main` builds all plugins and uploads them as a CI artifact. Tagged versions (`v*`) additionally publish the `.imagerplugin` files to [Releases](../../releases), where each one can be downloaded individually.

## Installing a plugin

Drop the desired `.imagerplugin` file into the Imager backend's plugins folder and restart it. See the [Imager.Core](https://github.com/ImagerMicroscopy/Imager.Core) documentation for the plugin directory location.
