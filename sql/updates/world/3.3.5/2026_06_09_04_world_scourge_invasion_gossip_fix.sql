-- 修复 Argent Emissary (16285) 对话系统：补全 gossip_menu + npc_text 中文化 + 删除不兼容的 WorldState conditions
--
-- 问题1: gossip_menu 记录缺失（7164→8434 等未插入），NPC对话空白
-- 问题2: npc_text 英文未中文化
-- 问题3: conditions 使用了 CONDITION_WORLD_STATE(11) 做区域状态动态切换，
--        但 TC 启动时验证 world state 是否存在，未注册的 state 会被跳过并报错
-- 修复: 补全所有 gossip_menu + gossip_menu_option，npc_text 中文化，
--       删除 WorldState conditions，每个区域只保留一个静态文本

-- =====================================================================
-- 1. npc_text 中文化（银色特使对话 + 区域状态 + Cultist Engineer）
-- =====================================================================
UPDATE `npc_text` SET `text0_0` = '部落与联盟必须将目光转向诺森德，面对巫妖王的入侵。近日来，卡利姆多和东部王国多处领土遭到攻击。你愿意拿起武器，拯救你的家园免遭毁灭吗？', `text0_1` = '部落与联盟必须将目光转向诺森德，面对巫妖王的入侵。近日来，卡利姆多和东部王国多处领土遭到攻击。你愿意拿起武器，拯救你的家园免遭毁灭吗？', `BroadcastTextID0` = 0 WHERE `ID` = 8434;
UPDATE `npc_text` SET `text0_0` = '战争的浪潮再次袭来。从寒冷的北方，巫妖王的浮空城已经降临我们的土地。他派出飞行要塞入侵整个世界。', `text0_1` = '战争的浪潮再次袭来。从寒冷的北方，巫妖王的浮空城已经降临我们的土地。他派出飞行要塞入侵整个世界。', `BroadcastTextID0` = 0 WHERE `ID` = 8471;
UPDATE `npc_text` SET `text0_0` = '目前该区域暂时没有天灾的威胁。但我担心他们不久就会卷土重来。', `text0_1` = '目前该区域暂时没有天灾的威胁。但我担心他们不久就会卷土重来。', `BroadcastTextID0` = 0 WHERE `ID` = 8481;
UPDATE `npc_text` SET `text0_0` = '没错，冬泉谷的山丘正在抵御新一轮的天灾进攻。你的帮助将大大缓解他们的压力。', `text0_1` = '没错，冬泉谷的山丘正在抵御新一轮的天灾进攻。你的帮助将大大缓解他们的压力。', `BroadcastTextID0` = 0 WHERE `ID` = 8480;
UPDATE `npc_text` SET `text0_0` = '天灾军团似乎已经来到了塔纳利斯。大量浮空城和其他部队已被派往那里。', `text0_1` = '天灾军团似乎已经来到了塔纳利斯。大量浮空城和其他部队已被派往那里。', `BroadcastTextID0` = 0 WHERE `ID` = 8482;
UPDATE `npc_text` SET `text0_0` = '诅咒之地的天灾威胁正在加剧。每一位有战斗能力的勇士都需要加入抵御入侵的行列。', `text0_1` = '诅咒之地的天灾威胁正在加剧。每一位有战斗能力的勇士都需要加入抵御入侵的行列。', `BroadcastTextID0` = 0 WHERE `ID` = 8483;
UPDATE `npc_text` SET `text0_0` = '如果再不向燃烧平原派遣增援，恐怕天灾将在那里建立行动基地。如果你能去，就去帮助那儿的守军吧。', `text0_1` = '如果再不向燃烧平原派遣增援，恐怕天灾将在那里建立行动基地。如果你能去，就去帮助那儿的守军吧。', `BroadcastTextID0` = 0 WHERE `ID` = 8484;
UPDATE `npc_text` SET `text0_0` = '天灾正在被攻击的地点建立小型据点，并通过头顶飞过的浮空城接收通讯和其他支援。据我们所知，根除他们的唯一方法就是消灭据点周围的地面部队。', `text0_1` = '天灾正在被攻击的地点建立小型据点，并通过头顶飞过的浮空城接收通讯和其他支援。据我们所知，根除他们的唯一方法就是消灭据点周围的地面部队。', `BroadcastTextID0` = 0 WHERE `ID` = 8486;
UPDATE `npc_text` SET `text0_0` = '我们已经赢得了 \$2219W 场对抗天灾的胜利。做好准备，\$n，这场战争远未结束。', `text0_1` = '我们已经赢得了 \$2219W 场对抗天灾的胜利。做好准备，\$n，这场战争远未结束。', `BroadcastTextID0` = 0 WHERE `ID` = 8551;
UPDATE `npc_text` SET `text0_0` = '我们已经赢得了 \$2219W 场对抗天灾的胜利。坚持下去，\$n！我们必须坚持不懈！', `text0_1` = '我们已经赢得了 \$2219W 场对抗天灾的胜利。坚持下去，\$n！我们必须坚持不懈！', `BroadcastTextID0` = 0 WHERE `ID` = 8555;
UPDATE `npc_text` SET `text0_0` = '巫妖王只派出了少量部队攻击艾泽拉斯的各主城，而他们的主力部队则会定期攻击以下区域：艾萨拉、诅咒之地、燃烧平原、塔纳利斯沙漠、东瘟疫之地和冬泉谷。', `text0_1` = '巫妖王只派出了少量部队攻击艾泽拉斯的各主城，而他们的主力部队则会定期攻击以下区域：艾萨拉、诅咒之地、燃烧平原、塔纳利斯沙漠、东瘟疫之地和冬泉谷。', `BroadcastTextID0` = 0 WHERE `ID` = 8573;
UPDATE `npc_text` SET `text0_0` = '艾萨拉的天灾威胁正在加剧。每一位有战斗能力的勇士都需要加入抵御入侵的行列。', `text0_1` = '艾萨拉的天灾威胁正在加剧。每一位有战斗能力的勇士都需要加入抵御入侵的行列。', `BroadcastTextID0` = 0 WHERE `ID` = 8593;
UPDATE `npc_text` SET `text0_0` = '东瘟疫之地的天灾威胁正在加剧。每一位有战斗能力的勇士都需要加入抵御入侵的行列。', `text0_1` = '东瘟疫之地的天灾威胁正在加剧。每一位有战斗能力的勇士都需要加入抵御入侵的行列。', `BroadcastTextID0` = 0 WHERE `ID` = 8594;

-- gossip_menu（主菜单 + 子菜单 + 区域状态文本）
DELETE FROM `gossip_menu` WHERE `MenuID` IN (7164, 7193, 7203, 7246, 7254, 7266, 7201, 7202, 7267, 7200, 7199);
INSERT INTO `gossip_menu` (`MenuID`, `TextID`) VALUES
(7164, 8434),
(7193, 8471),
(7203, 8486),
(7246, 8551),
(7246, 8554),
(7246, 8555),
(7254, 8573),
(7266, 8481),
(7201, 8481),
(7202, 8481),
(7267, 8481),
(7200, 8481),
(7199, 8481);

-- gossip_menu_option（主菜单4个选项）
DELETE FROM `gossip_menu_option` WHERE `MenuID` = 7164;
INSERT INTO `gossip_menu_option` (`MenuID`, `OptionID`, `OptionIcon`, `OptionText`, `OptionBroadcastTextID`, `OptionType`, `OptionNpcFlag`, `ActionMenuID`) VALUES
(7164, 0, 0, '发生了什么事？', 0, 1, 1, 7193),
(7164, 1, 0, '我能做什么？', 0, 1, 1, 7203),
(7164, 2, 0, '我们在哪里与天灾作战？', 0, 1, 1, 7254),
(7164, 3, 0, '我们赢得了多少场战斗？', 0, 1, 1, 7246);

-- gossip_menu_option（子菜单"返回"按钮）
DELETE FROM `gossip_menu_option` WHERE `MenuID` IN (7193, 7203, 7246);
INSERT INTO `gossip_menu_option` (`MenuID`, `OptionID`, `OptionIcon`, `OptionText`, `OptionType`, `OptionNpcFlag`, `ActionMenuID`) VALUES
(7193, 0, 0, '我还有问题。', 1, 1, 7164),
(7203, 0, 0, '我还有问题。', 1, 1, 7164),
(7246, 0, 0, '我还有问题。', 1, 1, 7164);

-- gossip_menu_option（区域查询子菜单7个选项）
DELETE FROM `gossip_menu_option` WHERE `MenuID` = 7254;
INSERT INTO `gossip_menu_option` (`MenuID`, `OptionID`, `OptionIcon`, `OptionText`, `OptionType`, `OptionNpcFlag`, `ActionMenuID`) VALUES
(7254, 0, 0, '艾萨拉目前正遭受攻击吗？', 1, 1, 7266),
(7254, 1, 0, '诅咒之地目前正遭受攻击吗？', 1, 1, 7201),
(7254, 2, 0, '燃烧平原目前正遭受攻击吗？', 1, 1, 7202),
(7254, 3, 0, '东瘟疫之地目前正遭受攻击吗？', 1, 1, 7267),
(7254, 4, 0, '塔纳利斯目前正遭受攻击吗？', 1, 1, 7200),
(7254, 5, 0, '冬泉谷目前正遭受攻击吗？', 1, 1, 7199),
(7254, 6, 0, '我还有问题。', 1, 1, 7164);

-- 补全缺失的 npc_text 8554（7246菜单第三段）
DELETE FROM `npc_text` WHERE `ID` = 8554;
INSERT INTO `npc_text` (`ID`, `text0_0`, `text0_1`, `BroadcastTextID0`, `lang0`, `Probability0`) VALUES
(8554, '我们已经赢得了 $2219W 场对抗天灾的胜利。打起精神，$n。虽然还有很多战斗在前方，但来自各个领域的英雄们已经挺身而出与之战斗。', '我们已经赢得了 $2219W 场对抗天灾的胜利。打起精神，$n。虽然还有很多战斗在前方，但来自各个领域的英雄们已经挺身而出与之战斗。', 0, 0, 100);

-- 删除不兼容的 WorldState conditions（TC 启动时验证 world state 是否存在，未注册则报错跳过）
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 14 AND `SourceGroup` IN (7199, 7200, 7201, 7202, 7266, 7267);

-- 更新 Argent Emissary npcflag（添加 gossip 标志）
UPDATE `creature_template` SET `npcflag` = `npcflag` | 1 WHERE `entry` = 16285;
