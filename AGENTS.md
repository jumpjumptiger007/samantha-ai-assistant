# AGENTS.md

## Project scope

- This repository packages the Samantha interface with the XiaoZhi voice-assistant firmware for the FoloToy AI Passport.
- The current supported target is `folotoy/ai-passport`: ESP32-C3, 8 MB flash, no PSRAM, and a 240 × 320 ST7789 display.
- Read `README.md` for the supported product behavior and canonical build. Keep this file focused on stable agent instructions.

## Architecture and change boundaries

- Keep Passport pins, hardware initialization, and board-specific UI behavior in `main/boards/folotoy/ai-passport/`. Keep shared behavior in the existing application, audio, display, and protocol layers.
- The current repository and CI intentionally support only the Passport board. If the project scope expands to another board, update the README scope, board/configuration chain, and CI single-board assertion together; follow docs/custom-board.md.
- Do not repurpose the existing Passport board identity, pin mapping, or flash layout to support different hardware. Treat changes to them as compatibility-sensitive and verify them against the Passport BSP and, when applicable, physical hardware. Use a distinct board identity or release variant for different hardware.
- Change runtime state through `Application::SetDeviceState()` and the device state machine. Callbacks that may run outside the main task should schedule application mutations with `Application::Schedule()` or the existing event mechanisms.
- Do not block the main event loop or audio tasks. Keep audio queues bounded and avoid repeated large allocations in real-time paths.
- Keep shared message semantics in `Protocol`; verify WebSocket and MQTT/UDP behavior when changing their shared contract.
- Validate network input, preserve `cJSON` ownership, and treat persistent NVS keys as an API that needs migration when changed.
- Account for the Passport's lack of PSRAM and its asset partition limits in UI, audio, and animation changes.

## Generated assets and build state

- Samantha GIFs under `assets/samantha/` are generated outputs. Use `scripts/samantha_preview/` and follow `assets/samantha/README.md` when changing animation sources or exporting assets; preserve the documented attribution.
- Do not manually edit generated or vendor output such as `build/`, `releases/`, `managed_components/`, `components/`, `main/assets/lang_config.h`, or generated `mmap_generate_*.h` files.
- `sdkconfig` is local build state. Make intentional configuration changes in tracked defaults or board configuration, not by treating a generated `sdkconfig` as the source of truth. The build script changes `sdkconfig` and build output; confirm the selected target when reusing a build directory.
- Flashing is separate from build verification. `build/merged-binary.bin` written at `0x0` can overwrite NVS or device configuration; inspect `build/flash_args` and use segmented addresses when that data must be preserved.

## Build and validation

Initialize the intended ESP-IDF environment before building. The README documents the supported SDK versions and canonical command:

```sh
python scripts/build.py folotoy/ai-passport --name ai-passport --language zh-CN
```

- Run `python3 -m unittest discover -s scripts/tests -v` for changes to build selection, build tooling, or asset packaging.
- Build the Passport target for firmware, board configuration, Kconfig, CMake, or UI/asset changes; check that packaged assets still fit their partition.
- Format only touched C/C++ files with the repository `.clang-format`; use `clang-format --dry-run -Werror <files>` to check them.
- A successful software build does not verify physical display, audio, or runtime behavior. Report hardware checks separately.

## References

- Board details: `main/boards/folotoy/ai-passport/README.md`
- Audio architecture: `main/audio/README.md`
- Code style: `docs/code_style.md`
- Board configuration guide: `docs/custom-board.md`
- Protocols: `docs/websocket.md`, `docs/mqtt-udp.md`, `docs/mcp-protocol.md`
- Samantha animation workflow and asset notes: `scripts/samantha_preview/README.md`, `assets/samantha/README.md`
- CI target selection: `.github/workflows/build.yml`
