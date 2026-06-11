# ![logo](https://community.trinitycore.org/public/style_images/1_trinitycore.png) TrinityCore 3.3.5 — BlizzardLike 2026 Edition

---

## Introduction

基于 [TrinityCore](https://www.trinitycore.org) 3.3.5 分支深度定制的怀旧服服务端，集成 NPCBot 玩家机器人、LLM 智能对话、天灾入侵世界事件等大量扩展功能，致力于还原巫妖王之怒 (Wrath of the Lich King) 的原版游戏体验。

核心框架源自 TrinityCore 及 [Trinity-Bots](https://github.com/trickerer/Trinity-Bots) NPCBot 模组，在此基础上进行了大量二次开发和系统改造。

---

## 主要特性

### 🤖 NPCBot 玩家机器人
- 可组队、可养成、可装备的 AI 队友
- 支持坦克/治疗/输出职责，自动走位、打断、驱散
- 数据库持久化 Bot 装备、天赋、外观

### 🧠 Bot LLM 智能对话
- 接入本地 LLM（llama.cpp），Bot 可根据上下文生成自然语言回复
- 支持中文对话，带队伍/副本/任务语境感知

### ⚔️ 天灾入侵世界事件
- 从 AzerothCore 移植的巫妖王之怒前夕天灾入侵系统
- 六大野外战区 + 两大主城袭击，浮空城通信链、死灵水晶、小兵无限刷新
- `.si` 命令查看入侵状态，`.si enable/disable` 开关事件

### 🌐 世界频道
- 服务端启动时自动创建"世界"频道，全体玩家自动加入

### 🛡️ 反作弊 & 游戏体验优化
- Warden 反作弊模块、反加速/穿墙检测
- 位面系统（Phasing）、动态刷新、弹性副本难度

### 🧰 管理工具 & 客户端插件
- **超级菜单 (SuperMenu)** — 多功能 GM 面板，支持传送、召唤、附魔、主题切换（`.gm menu`）
- **Bot 装备管理 (NPCBotInventory)** — 可视化界面管理 Bot 背包、装备、宝石附魔，一键换装
- **idTip** — 鼠标提示显示 NPC/物品/法术 ID，调试必备

---

## Requirements

编译环境要求与 TrinityCore 3.3.5 官方一致，详见 [TrinityCore Wiki](https://trinitycore.info/en/install/requirements)。

额外依赖：
- **llama.cpp** — Bot LLM 对话功能（可选，通过 CMake 选项控制）

---

## Install

```bash
git clone <repo-url>
cd TrinityCoreNPCBotsBlizzardLike2026EditionSource
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build . --config RelWithDebInfo --target worldserver
```

数据库安装请参考 [TrinityCore Wiki](https://trinitycore.info/en/home)，NPCBots 安装指南见 [Trinity-Bots](https://github.com/trickerer/Trinity-Bots#npcbot-mod-installation)。

客户端插件（idTip、SuperMenu、NPCBotInventory）位于 `ClientAddons/` 目录，复制到 `World of Warcraft/Interface/AddOns/` 即可。

---

## Reporting Issues

请通过项目 Issue 追踪器提交问题。提交前请先查阅已有 issue 避免重复。

---

## Copyright

License: GPL 2.0

本项目基于以下开源项目：
- [TrinityCore](https://github.com/TrinityCore/TrinityCore) — GPL 2.0
- [Trinity-Bots (NPCBots)](https://github.com/trickerer/Trinity-Bots) — GPL 2.0
- [AzerothCore](https://github.com/azerothcore/azerothcore-wotlk) — 天灾入侵系统参考实现 (GPL 2.0)
- [llama.cpp](https://github.com/ggml-org/llama.cpp) — MIT

详见 [COPYING](COPYING) 及 [AUTHORS](AUTHORS)。

---

## Links

- [TrinityCore 官网](https://www.trinitycore.org)
- [TrinityCore Wiki](https://www.trinitycore.info)
- [Trinity-Bots (NPCBots)](https://github.com/trickerer/Trinity-Bots)
- [AzerothCore](https://github.com/azerothcore/azerothcore-wotlk)
- [llama.cpp](https://github.com/ggml-org/llama.cpp)
