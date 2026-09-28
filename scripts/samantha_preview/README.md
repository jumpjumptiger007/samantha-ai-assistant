# Samantha OS1 motion preview

This local browser prototype and GIF exporter share one adapted Her / OS1
infinity-loader animation. All states use the same white tube, white torus,
camera, and coral background. The normalized transition runs from the open
helix at `0.0` to the circular ring at `1.0`, including a reversible crossfade.
The model advances on a fixed 60 FPS reference clock, independent of browser
`requestAnimationFrame` frequency.

## Default motion values

| State | Rotation | Target transition | Ring motion / wobble |
| --- | ---: | ---: | ---: |
| Idle | 0.015 | 0.00 | 0.020 |
| Listening | 0.035 | 0.00 | 0.025 |
| Thinking | 0.070 | 0.70 | 0.050 |
| Speaking | 0.120 | 0.98 | 0.080 |

Transition speed defaults to `0.45 progress/s`. Reverse transitions run at 1.4
times that speed, following the alternate OS1 implementation. State buttons
animate toward their target; dragging Transition progress directly controls the
shape until another state is selected. The ring uses the upstream torus size
and wobble equations, with the wobble scaled by the Ring motion control.

## Run locally

From this directory, start a static server and open the shown local URL:

```sh
python3 -m http.server 8000
```

The page imports Three.js 0.182.0 from unpkg, matching the version vendored by
the upstream reference, so the browser needs network access on first load.

## Export GIFs

From the repository root, run:

```sh
python3 scripts/samantha_preview/export_gifs.py
```

Open the local URL printed by the script and choose **Export all GIFs**. The
exporter renders at fixed timesteps, samples at 20 FPS, and writes only the four
128 × 128 firmware candidates under `assets/samantha/` and the separate
240 × 320 showcase at `docs/samantha-ui-demo.gif`. It uses Python's standard
library and a local palette/LZW GIF encoder; Three.js 0.182.0 is loaded from
the pinned unpkg URL.

## Attribution

- Visual reference: Her / OS1 infinity-loader.
- Upstream repository: `nickvasilescu/hermes-desktop-os1`.
- Upstream commit: `bea7d24aa03c1834b80390345c8875eaf9ae6502`.
- Implementation reference:
  `Sources/OS1/Resources/Boot/infinity-loader.js`.
- Cross-check: `callbacked/os1`, `src/components/OS1Animation.tsx`.
- Idle, Listening, Thinking, and Speaking are parameter presets/adaptations
  for this prototype, not four upstream-defined states.

The upstream repository is MIT licensed. Its LICENSE identifies:

```text
Copyright (c) 2026 Element Software
Copyright (c) 2026 Orgo, Inc.
Copyright (c) 2026 dodo-reach (original Hermes Desktop)
```

Any distributed substantial adaptation should retain the upstream MIT notice
and license text.
