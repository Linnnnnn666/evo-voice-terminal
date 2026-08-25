# EvoAgent — 自进化的 AI 硬件开发系统

> **一句话**：用语音或文字指挥 AI 智能体，为 ESP32 板卡完成「写固件 → 编译 → OTA 部署 → 遥测验收 → 经验沉淀」的完整开发闭环，系统自主迭代，人在环兜底。

```
  用户语音「你好小安」              用户文字（DSH 会话）
        │                                │
        ▼                                ▼
 ┌───────────────┐              ┌──────────────────┐
 │  语音板        │   WS/opus    │  DSH              │
 │ (evo-voice-   │ ───────────► │ (DeepSeek Harness)│
 │  terminal)    │              │  AI 智能体         │
 └───────▲───────┘              └────────┬─────────┘
         │ TTS 播报                      │ MCP 协议
         │                               ▼
         │                      ┌──────────────────┐   MQTT / HTTP    ┌──────────────┐
         └──────────────────────│  能力中枢         │ ───────────────► │ ESP32 板卡    │
                                │  (fall-mcp)      │ ◄─────────────── │ 跌倒检测/烧录 │
                                │  47 个工具        │   遥测/事件回流   │ /业务板      │
                                └────────┬─────────┘                  └──────────────┘
                                         │
                                         ▼
                            工具工厂 · 经验库 · 人在环验收
                            （同样的坑，系统不犯第二次）
```

## 系统的灵魂：三个设计信念

1. **自进化（能力容器，而非模型）**
   系统每解决一个问题，就把经验沉淀进可检索的知识库（RAG + 置信度）；AI 可以自造新工具（工具工厂）——模型不变，但系统越用越熟练。进化的能力容器（工具/经验/资产），不碰模型，可控且可解释。

2. **人在环验收（AI 可靠性的底座）**
   系统先自己验：遥测字段断言（如 `led_color=#0000FF`），验不了/验不过才语音问你。修复循环最多 3 轮，再不行求助人类。AI 全自动不可信，人在环是信任底座。

3. **AI 协作开发（工程师的进化方向）**
   AI 写代码、编译、归档、部署；人类负责架构、硬件驱动、全链路排障、最终验收。这个仓库群就是这套协作模式的完整实践。

## 仓库地图（三件套）

| 仓库 | 角色 | 一句话 |
|------|------|--------|
| **[evo-firmware](https://github.com/Linnnnnn666/evo-firmware)** | 硬件端 | ESP32-S3 固件集合：跌倒检测板（端侧 AI）、云端烧录板、配置化引导固件 |
| **[evo-fall-mcp](https://github.com/Linnnnnn666/evo-fall-mcp)** | 能力中枢 | MCP 服务器（47 工具）：部署/烧录/播报/自验收/自进化，连接 AI 与硬件 |
| **[evo-voice-terminal](https://github.com/Linnnnnn666/evo-voice-terminal)** | 语音入口 | 语音板板卡包：唤醒「你好小安」→ 语音对话 → TTS 播报 |

**本仓库是其中的「语音入口」**——让用户用最自然的方式（说话）指挥整个系统。

---

# EvoAgent Voice Terminal — 小安语音板（evo-voice-v1）

基于 [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) **v2.4.2** 的**板卡定制包**（board package），不是完整固件。

> 本仓库只含 EvoAgent 自研的板卡代码与配置；固件本体请克隆上游 xiaozhi-esp32（MIT）后按下方说明合并。

## 它在系统中的角色

- **入口**：用户喊「你好小安」，唤醒词在本地 esp-sr 完成（离线、低延迟）
- **对话**：WS/opus 上送服务器 → ASR(SherpaParaformer) → LLM(deepseek function_call) → TTS(Edge) → 板子播放
- **播报**：系统主动开口（跌倒事件/部署结果）——fall-mcp `dev_speak` → `/api/push` → 板子 TTS 原样播报
- **OTA**：固件升级走自家服务器 `/xiaozhi/ota/`，与部署闭环无缝衔接

## 硬件

ESP32-S3 N16R8（8MB OCT PSRAM）+ 两个纯 I2S 外设，无 I2C codec 芯片：

| 模块 | 引脚 | 说明 |
|------|------|------|
| INMP441 麦克风 | BCLK=GPIO12, WS=GPIO13, DOUT=GPIO21 | I2S 输入，L/R 接 GND（左声道） |
| MAX98357A 功放 | BCLK=GPIO4, LRCLK=GPIO14, DIN=GPIO18, SD=GPIO5 | I2S 输出，VDD 接 3.3V |
| 板载 LED | GPIO2 | 状态指示 |
| BOOT 键 | GPIO0 | 点击切换对话 |

> ⚠️ **供电**：功放从 3.3V 取电，播放瞬间电流会拉垮 3.3V 轨。**必须使用充电器/独立 5V 电源供电**（电脑 USB 口在播放时会触发欠压复位）。改善方案：功放 VDD 并联 ≥470µF 电容，或改接 5V 供电轨。
> ⚠️ **引脚避让**：GPIO33-37 是 PSRAM 总线（OCT 模式），GPIO26-32 是 flash 总线，麦克风/功放引脚均不得占用；GPIO15/16 实测异常，勿用于 I2S。

## 使用方式

```bash
git clone https://github.com/78/xiaozhi-esp32.git -b v2.4.2 xiaozhi
cd xiaozhi
# 1. 复制板卡
cp -r <本仓库>/boards/evo-voice-v1 main/boards/
# 2. 注册板卡（main/Kconfig.projbuild 增加 BOARD_TYPE_EVO_VOICE_V1，main/CMakeLists.txt 增加分支）
# 3. 配置 sdkconfig（见下方清单）
# 4. 修改 main/protocols/websocket_protocol.cc 的默认 WS 地址（或保持 config.h 注入）
# 5. 构建烧录
idf.py build
idf.py -p COMx flash monitor
```

## 关键配置清单（sdkconfig）

| 配置 | 值 | 原因 |
|------|-----|------|
| `CONFIG_BOARD_TYPE_EVO_VOICE_V1=y` | 板卡 | 注册本板 |
| `CONFIG_SR_WN_WN9_NIHAOXIAOAN_TTS2=y` | 唤醒词 | 「你好小安」（与服务器人设一致） |
| `CONFIG_SPIRAM_MODE_OCT=y` | PSRAM | 本板为 OCT 嵌入式 PSRAM（QUAD 会启动失败） |
| `CONFIG_SPIRAM_SPEED_40M=y` | PSRAM 频率 | 40M 更稳定（80M 偶发随机崩溃） |
| `CONFIG_OTA_URL="http://YOUR_SERVER/xiaozhi/ota/"` | OTA | 指向自家 xiaozhi-server（避开官方激活流程） |

## 板卡实现要点（踩坑记录）

1. **I2S 通道必须显式 `i2s_channel_enable()`**：上游部分板卡（如 t-circle-s3）没调，`i2s_channel_read/write` 会直接返回 `ESP_ERR_INVALID_STATE`，外设不启动（无时钟、无数据）。
2. **INMP441 用 16-bit 槽位**（BCLK=32×fs=512kHz）：实测 32-bit 槽位（1.024MHz）下该模块输出 8192 步长的错乱数据；16-bit 槽位正常。24-bit 数据跨槽：L 槽=高 16 位（有效），R 槽=LSB 残留（丢弃）。
3. **输入增益 ×16**：INMP441 输出幅度仅 ~1.5% 满幅，直接喂唤醒模型几乎无法触发；读取后左移 4 位并限幅。
4. **播放必须 mono→stereo 复制**：I2S TX 是 16-bit 立体声槽，mono PCM 直接写入会以 2 倍速播放（声音沙哑/语速怪）；Write 中复制 L=R。
5. **默认音量 30**：3.3V 供电的 MAX98357A 大音量会拉垮电源（见供电注意）。
6. **唤醒词与服务器人设对齐**：esp-sr 自带 `wn9_nihaoxiaoan_tts2` 预训练模型，切换 sdkconfig 即可，无需训练。

## 服务器集成

- WS 连接：`ws://YOUR_SERVER/xiaozhi/v1/`（Caddy 反代到 xiaozhi-server 8001）
- OTA：`http://YOUR_SERVER/xiaozhi/ota/`（xiaozhi-server 8003，按设备型号查固件目录）
- **主动播报**（跌倒事件等）：fall-mcp `dev_speak` → POST `/api/push` → 服务器 `notify()` → tts/start + chat → 板子 TTS 播报。多语音板并存时请显式指定 `device_id`，避免播报到错误的板。

## 迭代历史

- `feat(board)` 板卡骨架：引脚定义与注册配置（08-18）
- `feat(audio)` I2S 音频 codec：16-bit 槽位 + 显式通道 enable + 输入增益 ×16 + mono→stereo 播放修复（08-20）
- `docs` README：接线/构建/踩坑记录/服务器集成（08-25）

## License

MIT © 2026 EvoAgent
