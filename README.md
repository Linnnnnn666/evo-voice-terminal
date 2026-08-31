# EvoAgent — 自进化的 AI 硬件开发系统

> **一句话**：用语音或文字指挥 AI 智能体，为 ESP32 板卡完成「写固件 → 编译 → OTA 部署 → 遥测验收 → 经验沉淀」的完整开发闭环——**核心链路已验证、架构完整、持续迭代中的系统原型**，双层自进化，人在环兜底。

> **English abstract**: Custom board package (`evo-voice-v1`) for **xiaozhi-esp32 v2.4.2** on **ESP32-S3** — a clean bring-up with two bare I2S peripherals: an **INMP441** microphone and a **MAX98357A** amplifier (no I2C codec). Wake word "你好小智" via esp-sr, plus a WebSocket **keep-alive patch** that lets the server push TTS broadcasts (alarms/notifications) to an idle board. Includes pinout, build and debugging notes. MIT licensed. Part of the self-evolving EvoAgent system — see [evo-firmware](https://github.com/Linnnnnn666/evo-firmware) and [evo-fall-mcp](https://github.com/Linnnnnn666/evo-fall-mcp).

```
                              ┌──────────────┐
                              │     用户     │
                              └──────┬───────┘
                语音「你好小智」        │       文字（DSH 会话）
                     │               │              │
                     ▼               ▼              ▼
        ┌──────────────────┐  ┌────────────────────────────┐
        │ 语音链路         │  │ 智能体层                    │
        │                  │  │ ┌──────────────────────┐   │
        │ 语音板           │  │ │ DSH-1 干活者         │    │
        │ (evo-voice-      │  │ │ 写代码/编译/部署/排障 ◄── ┼── 插件装入
        │  terminal)       │  │ └──────────┬───────────┘    │
        │   │ WS/opus      │  │            │ 能力缺口       │
        │   ▼              │  │            ▼                │
        │ xiaozhi-server   │  │ ┌──────────────────────┐    │
        │ ASR→LLM→TTS      │  │ │ DSH-2 进化者          │   │
        └────────┬─────────┘  │ │ (隔离环境开发插件)     │──┼── req_*.json
                 │            │ └──────────────────────┘    │
                 │ 工具调用    └────────────────────────────┘
                 ▼
        ┌───────────────────────────────────────────────────┐
        │ 能力中枢 (fall-mcp) —— 47 工具                    │
        │ 部署/烧录/播报/自验收/门控/工具工厂/经验库/插件轮询│
        └───────┬──────────────────────────┬────────────────┘
                │ MQTT / HTTP / OTA        │ 遥测 · 事件回流
                ▼                          ▲
        ┌───────────────────────────────────────────────┐
        │ 硬件层 —— ESP32 板卡                          │
        │ 跌倒检测板 · 云端烧录板 · 业务板（OTA 双分区） │
        └───────────────────────────────────────────────┘

   进化回流：工具工厂/经验库 → 注入下一次任务 · DSH-2 插件 → 装入 DSH-1
   人在环：关键决策经语音板播报确认（confirm 队列）——AI 全自动不可信
```

## 系统的灵魂：双层自进化

**这不是一个"用 AI 写固件"的项目，而是一个"AI 自己给自己升级能力"的系统。**
系统不仅越用越熟练（经验沉淀），还能**自己发现自己缺什么能力、自己把能力造出来装上**——两层进化闭环。

### 第一层 · 系统自进化 —— 进化"手"（工具与经验）

每干完一次活自动复盘：需求值得固化？→ 工具工厂生成 MCP 工具（编译+验证才注册）；
任务结果沉淀进经验库（bigram 索引），下次相似任务自动注入参考经验。
真实案例："查银价" → 系统自造 `query_silver_price` 工具 → 之后语音直接调用。

### 第二层 · 智能体自进化 —— 进化"大脑"（DSH 插件）

DSH 双角色：**DSH-1 干活，DSH-2 进化**。DSH-1 干完活复盘自己的能力缺口 →
写插件需求文件 → DSH-2 在**隔离环境**（headless-builder profile）开发插件 →
装入 DSH-1 → 健康检查；装坏了 DSH-2 修复，坏插件移入 `quarantine/` 隔离区。
真实案例：DSH-2 造出 `base64-codec` / `reverse-string` / `text-stats` 插件给 DSH-1 装上。

### 闭环与保险

```
DSH-1 干活 → 复盘① 固化工具（手变强）/ 复盘② 发现缺口 → req_*.json
   → DSH-2 隔离造插件 → 装入 DSH-1（大脑变强）→ 干得更好 → 经验更多 → 螺旋上升
保险：DSH-2 隔离开发 · 工具编译验证 · 插件健康检查 · quarantine 可回滚 · 人在环兜底
```

**进化的是能力容器（工具/插件/经验），不碰模型**——可控、可解释、可回滚。
而你面前的这块语音板，就是这套系统的**人类入口**：喊一声「你好小智」，指挥整个自进化系统干活。

## 两个支撑信念

1. **人在环验收**：系统先自己验（遥测断言），验不了/验不过才语音问你——AI 全自动不可信，人在环是信任底座。
2. **AI 协作开发**：AI 写代码、编译、部署；人类负责架构、硬件驱动、排障、最终验收。

## 仓库地图（三件套）

| 仓库 | 角色 | 一句话 |
|------|------|--------|
| **[evo-firmware](https://github.com/Linnnnnn666/evo-firmware)** | 硬件端 | ESP32-S3 固件集合：跌倒检测板（端侧 AI）、云端烧录板、配置化引导固件 |
| **[evo-fall-mcp](https://github.com/Linnnnnn666/evo-fall-mcp)** | 能力中枢 | MCP 服务器（47 工具）：部署/烧录/播报/自验收/自进化，连接 AI 与硬件 |
| **[evo-voice-terminal](https://github.com/Linnnnnn666/evo-voice-terminal)** | 语音入口 | 语音板板卡包：唤醒「你好小智」→ 语音对话 → TTS 播报 |

**本仓库是其中的「语音入口」**——让用户用最自然的方式（说话）指挥整个系统。

---

# EvoAgent Voice Terminal — 小智语音板（evo-voice-v1）

基于 [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) **v2.4.2** 的**板卡定制包**（board package），不是完整固件。

> 本仓库只含 EvoAgent 自研的板卡代码与配置；固件本体请克隆上游 xiaozhi-esp32（MIT）后按下方说明合并。

## 它在系统中的角色

- **入口**：用户喊「你好小智」，唤醒词在本地 esp-sr 完成（离线、低延迟）
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

> 分层入口：**[0 层 · 先看效果](https://github.com/Linnnnnn666/evo-fall-mcp)**（系统架构图）· **[1 层 · 纯软件 5 分钟](https://github.com/Linnnnnn666/evo-fall-mcp#快速开始)**（能力中枢，无需硬件）·
> **[2 层 · 单板体验](https://github.com/Linnnnnn666/evo-firmware)**（跌倒板/业务板）· **[3 层 · 完整系统](https://github.com/Linnnnnn666/evo-fall-mcp/blob/main/docs/QUICK_START.md)**（本板全链路搭建）

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
| `CONFIG_SR_WN_WN9_NIHAOXIAOZHI_TTS=y` | 唤醒词 | 「你好小智」（与服务器人设一致） |
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

## 空闲保活（keep-alive）补丁

上游 v2.4.2 的 WebSocket 是**懒连接**——只有对话时才连服务器，说完即断。板子空闲时服务器找不到它，`/api/push` 会返回 `404 no active device`，闹钟/主动播报在空闲时全部失效。

`docs/patches/websocket-keepalive.patch` 让固件**开机即连、每 30s 心跳、断线自动重连**，空闲时也能被服务器随时喊话（本系统已用于 23:30 闹钟补播闭环验证）：

```bash
git apply docs/patches/websocket-keepalive.patch   # 在 xiaozhi 仓库根目录执行
```

配套服务器配置（`data/.config.yaml`，重启生效）：

```yaml
enable_websocket_ping: true             # 默认 false！不开启则服务器直接忽略心跳
close_connection_no_voice_time: 86400   # 空闲超时 600→86400（双保险）
```

实现要点：心跳用 JSON `{"type":"ping"}`（服务器回 `pong` 并刷新活动时间戳），而不是 WS 控制帧 Ping——后者不会更新服务器的 `last_activity_time`；连接对象改 `shared_ptr` + 互斥锁，避免后台心跳与关闭连接的任务竞态。

## 服务器集成

- WS 连接：`ws://YOUR_SERVER/xiaozhi/v1/`（Caddy 反代到 xiaozhi-server 8001）
- OTA：`http://YOUR_SERVER/xiaozhi/ota/`（xiaozhi-server 8003，按设备型号查固件目录）
- **主动播报**（跌倒事件等）：fall-mcp `dev_speak` → POST `/api/push` → 服务器 `notify()` → tts/start + chat → 板子 TTS 播报。多语音板并存时请显式指定 `device_id`，避免播报到错误的板。

## 迭代历史

- `feat(board)` 板卡骨架：引脚定义与注册配置（08-18）
- `feat(audio)` I2S 音频 codec：16-bit 槽位 + 显式通道 enable + 输入增益 ×16 + mono→stereo 播放修复（08-20）
- `docs` README：接线/构建/踩坑记录/服务器集成（08-25）
- `feat(keepalive)` WebSocket 空闲保活：开机即连 + 30s JSON 心跳 + 断线自动重连，空闲可被服务器主动喊话（08-28）

## License

MIT © 2026 EvoAgent
