# Samantha for FoloToy AI Passport

(English | [中文](README_zh.md))

Samantha is a voice-first AI assistant and companion experience for the FoloToy AI Passport. Its interface takes inspiration from the OS1 / Samantha visual language in *Her* and supports real-time voice interaction. This is an independent implementation, not official *Her* software.

![Samantha UI demo](docs/samantha-ui-demo.gif)

## Overview

This repository packages the Samantha interface for one target: the FoloToy AI Passport. It uses the existing XiaoZhi voice pipeline, with a board-local display implementation and pre-rendered GIF animations. Assistant sentence subtitles reuse XiaoZhi's existing TTS sentence text path; user speech transcripts are intentionally hidden from the conversation UI.

## Features

- OS1 / Samantha-inspired visual interface, independently implemented for the Passport.
- Idle helix, Listening helix, STT-triggered Thinking transition, and Speaking ring animations.
- Assistant response subtitles; user speech transcripts are not shown.
- Real-time voice interaction through the existing XiaoZhi voice pipeline.
- Device and cloud MCP capabilities inherited from the XiaoZhi core.
- Board-local FoloToy AI Passport integration, targeting its ESP32-C3 and 240×320 ST7789 display.

## Interaction

`Idle → Listening → user speaks → Thinking (after STT) → Speaking + assistant sentence subtitles → Listening / Idle`

The current states use pre-rendered GIFs optimized for the ESP32-C3. The firmware animations are 128×128 at 20 FPS; the larger demo above is for the README only.

## Hardware

The supported hardware target is the FoloToy AI Passport:

- ESP32-C3 with 8 MB flash and no PSRAM
- ST7789 / ST7789P3 portrait display, 240×320
- ES8311 audio codec
- CW2017 fuel gauge

## Build

The repository requires ESP-IDF 6.0.1 or later. ESP-IDF 6.1 is the validated SDK for the canonical Samantha Passport build, which passes with the command below:

```sh
python scripts/build.py folotoy/ai-passport --name ai-passport --language zh-CN
```

The build checks that the firmware and packaged assets fit their Passport flash partitions, including the 2 MB assets partition. A successful software build does not validate playback or runtime behavior on physical hardware.

## Flashing

The build produces `build/merged-binary.bin` for flashing at offset `0x0`. A merged image writes across the space between segments and can overwrite existing NVS or device configuration. For a device whose identity or configuration must be preserved, inspect and use the segmented addresses in `build/flash_args` instead. The build does not back up existing firmware or configuration.

## Development Status

Implemented and validated in software:

- Samantha animations and GIF asset packaging pipeline
- Assistant sentence subtitle integration and board-local Passport display behavior
- Canonical ESP-IDF 6.1 build, with firmware and assets fitting their configured partitions

Still awaiting physical hardware validation:

- Passport GIF playback and runtime behavior
- Runtime RAM measurement, subtitle layout, and transition smoothness on device
- Final hardware release

## Developer References

- [Passport board implementation](main/boards/folotoy/ai-passport)
- [MCP usage](docs/mcp-usage.md) and [MCP protocol](docs/mcp-protocol.md)
- [MQTT + UDP](docs/mqtt-udp.md) and [WebSocket protocol](docs/websocket.md)
- [Samantha preview and export tooling](scripts/samantha_preview/README.md)
- [Samantha GIF asset notes](assets/samantha/README.md)

## Open-source Foundation & Credits

This repository derives from [XiaoZhi ESP32](https://github.com/78/xiaozhi-esp32) v2.5.0. XiaoZhi provides the underlying voice, network, protocol, application, and MCP foundation. The Samantha interaction and visual layer here is an independent implementation for the FoloToy AI Passport, whose board support follows the [FoloToy AI Passport BSP](https://github.com/FoloToy/ai-passport).

The OS1 animation geometry and motion are adapted with reference to [`nickvasilescu/hermes-desktop-os1`](https://github.com/nickvasilescu/hermes-desktop-os1/tree/bea7d24aa03c1834b80390345c8875eaf9ae6502), commit `bea7d24aa03c1834b80390345c8875eaf9ae6502`. The related [`callbacked/os1`](https://github.com/callbacked/os1) implementation is also a reference. Samantha is not official *Her* or official OS1 software.

Source-specific animation attribution and upstream license details are recorded in the [asset notes](assets/samantha/README.md) and [preview tooling documentation](scripts/samantha_preview/README.md).

## License

This project is distributed under the MIT license in this repository. Retained upstream copyright notices, license terms, and attributions continue to apply; see the linked source-specific notes above for the adapted OS1 animation.
