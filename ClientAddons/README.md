# NPCBotInventory 客户端插件

与 `bot_manager_addon.cpp`（BMU 协议）配套，需放入魔兽客户端：

`World of Warcraft\Interface\Addons\NPCBotInventory\`

## 版本

- **1.2**（2026-05-28）：支持服务端 `U;OPEN;entry`，右键「让我看看你的装备」自动打开界面；名称与场上机器人一致。

## 服务端要求

- `AddonChannel = 1`（worldserver.conf）
- 重新编译后的 worldserver（含 BMU 名称修复与 `SendBotSnapshotAndOpenUI`）
