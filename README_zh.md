# Samantha for FoloToy AI Passport

(中文 | [English](README.md))

Samantha 是面向 FoloToy AI Passport 的语音优先 AI 助手与陪伴体验。界面视觉语言受《Her》中的 OS1 / Samantha 启发，并支持实时语音交互；这是独立实现，并非电影官方软件。

![Samantha UI 演示](docs/samantha-ui-demo.gif)

## 项目概览

本仓库专注于 FoloToy AI Passport 上的 Samantha 界面。语音交互沿用 XiaoZhi 现有的语音链路，显示部分则在 Passport 板级实现中集成，并使用预渲染 GIF 动画。助手回复字幕复用 XiaoZhi 现有的 TTS 句子文本通路；用户语音转写内容不会显示在对话界面中。

## 功能

- 为 Passport 独立实现的 OS1 / Samantha 风格界面。
- 待机螺旋动画、聆听动画、由 STT 触发的思考过渡，以及说话时的环形动画。
- 显示助手回复字幕；不显示用户语音转写内容。
- 通过现有 XiaoZhi 语音链路实现实时语音交互。
- 沿用 XiaoZhi 核心提供的设备端与云端 MCP 能力。
- FoloToy AI Passport 板级集成，目标硬件为 ESP32-C3 和 240×320 ST7789 屏幕。

## 交互流程

`待机 → 聆听 → 用户说话 → 思考（STT 识别后触发）→ 播报并显示助手逐句字幕 → 聆听 / 待机`

当前状态使用针对 ESP32-C3 优化的预渲染 GIF；固件动画为 128×128、20 FPS。上方较大的演示 GIF 仅用于 README 展示。

## 硬件

本仓库目前仅面向 FoloToy AI Passport：

- ESP32-C3，8 MB Flash，无 PSRAM
- ST7789 / ST7789P3 竖屏，240×320
- ES8311 音频编解码器
- CW2017 电量计

## 构建

仓库要求 ESP-IDF 6.0.1 或更高版本。Samantha Passport 的标准构建已使用 ESP-IDF 6.1 验证通过：

```sh
python scripts/build.py folotoy/ai-passport --name ai-passport --language zh-CN
```

构建会检查固件和打包资源是否能放入 Passport 对应的 Flash 分区，其中资源分区为 2 MB。软件构建通过不代表已验证真机播放或运行时行为。

## 烧录

构建生成的 `build/merged-binary.bin` 从 Flash 地址 `0x0` 开始写入。合并镜像会覆盖各段之间的空隙，可能覆盖设备已有的 NVS 或配置。若需要保留设备身份或配置，请先检查 `build/flash_args` 中的分段地址，并使用分段烧录。构建过程不会备份已有固件或配置。

## 开发状态

已实现并通过软件验证：

- Samantha 动画及 GIF 资源打包流程
- 助手逐句字幕集成与 Passport 板级显示实现
- ESP-IDF 6.1 标准构建；固件和资源均符合已配置的分区容量

仍待真机验证：

- Passport 上的 GIF 播放与运行时行为
- 设备实际 RAM 占用、字幕布局及动画过渡流畅度
- 最终硬件版本发布

## 开发参考

- [Passport 板级实现](main/boards/folotoy/ai-passport)
- [MCP 用法](docs/mcp-usage_zh.md)与 [MCP 协议](docs/mcp-protocol_zh.md)
- [MQTT + UDP](docs/mqtt-udp_zh.md)与 [WebSocket 协议](docs/websocket_zh.md)
- [Samantha 预览与导出工具](scripts/samantha_preview/README.md)
- [Samantha GIF 资源说明](assets/samantha/README.md)

## 开源基础与致谢

本仓库基于 [XiaoZhi ESP32](https://github.com/78/xiaozhi-esp32) v2.5.0 演进。XiaoZhi 提供底层语音、网络、协议、应用及 MCP 能力；这里的 Samantha 交互与视觉层是面向 FoloToy AI Passport 的独立实现，板级支持参考了 [FoloToy AI Passport BSP](https://github.com/FoloToy/ai-passport)。

OS1 动画的几何形状与运动参考并改编自 [`nickvasilescu/hermes-desktop-os1`](https://github.com/nickvasilescu/hermes-desktop-os1/tree/bea7d24aa03c1834b80390345c8875eaf9ae6502)，具体版本为提交 `bea7d24aa03c1834b80390345c8875eaf9ae6502`；[`callbacked/os1`](https://github.com/callbacked/os1) 也作为交叉参考。Samantha 并非《Her》或 OS1 的官方软件。

动画来源署名及上游许可证信息另见 [GIF 资源说明](assets/samantha/README.md)和[预览工具文档](scripts/samantha_preview/README.md)。

## 许可证

本项目遵循仓库中的 MIT 许可证。保留的上游版权声明、许可证条款及来源署名仍然适用；OS1 动画的具体说明见上方链接。
