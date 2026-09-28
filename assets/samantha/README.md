# Samantha GIF assets

The four firmware candidates are generated at **128 × 128 pixels, 20 FPS**:

| File | Motion | Playback metadata |
| --- | --- | --- |
| `idle.gif` | Open OS1 helix, slow rotation | Infinite loop |
| `listening.gif` | Open OS1 helix, normal rotation | Infinite loop |
| `thinking.gif` | Open helix moving toward the ring, ending at the approved 0.70 transition target | Finite; no GIF loop extension |
| `speaking.gif` | Near-circular ring with restrained source-derived motion | Infinite ping-pong loop |

`docs/samantha-ui-demo.gif` is a separate **240 × 320** README showcase, not a firmware asset. The export command is `python3 scripts/samantha_preview/export_gifs.py`; open the URL it prints and choose **Export all GIFs**. Rendering uses a fixed 60 FPS simulation clock and samples output at 20 FPS.

The curve geometry and motion equations are adapted from `nickvasilescu/hermes-desktop-os1`, commit [`bea7d24aa03c1834b80390345c8875eaf9ae6502`](https://github.com/nickvasilescu/hermes-desktop-os1/tree/bea7d24aa03c1834b80390345c8875eaf9ae6502), especially `Sources/OS1/Resources/Boot/infinity-loader.js`. The upstream project is MIT-licensed; retain its attribution and license notice when redistributing substantial adapted code ([upstream license](https://github.com/nickvasilescu/hermes-desktop-os1/blob/bea7d24aa03c1834b80390345c8875eaf9ae6502/LICENSE)).

These GIFs are software-rendered adaptations of the prototype, not film frames or official *Her* UI assets. The README showcase is not included in the firmware asset collection. The one-pass Thinking candidate may need explicit completion/loop handling when integrated with the device GIF player.
