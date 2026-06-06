-- 天灾入侵系统：补全任务 NPC 刷怪 + quest 绑定 + Cultist Engineer 对话修复
-- 参照 Acore 2025_07_12_01.sql + 2025_08_08_01.sql

-- =====================================================================
-- 消除 Argent Emissary 自带的卡拉赞任务（12616 Chamber of Secrets），避免干扰
DELETE FROM world.creature_queststarter WHERE id = 16285 AND quest = 12616;
DELETE FROM world.creature_questender WHERE id = 16285 AND quest = 12616;

-- 1. NPC 刷怪 (通过变量动态分配 GUID 避免冲突)
-- =====================================================================
SET @NEXT_GUID = (SELECT COALESCE(MAX(guid), 0) + 1 FROM world.creature);

-- 先清除所有相关 NPC 的旧数据
DELETE FROM world.creature WHERE id IN (16285, 16359, 16361, 16484, 16490, 16493, 16495);

-- Commander Thomas Helleran (16361) — 东瘟疫光明大教堂，任务 9085/9153
INSERT INTO world.creature (guid, id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, spawntimesecs, MovementType) VALUES
(@NEXT_GUID, 16361, 0, 1, 1, 2240.87, -5317.26, 82.2506, 1.67552, 120, 0);
SET @NEXT_GUID = @NEXT_GUID + 1;

-- Lieutenant Nevell (16484) — 暴风城/铁炉堡区域
INSERT INTO world.creature (guid, id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, spawntimesecs, MovementType) VALUES
(@NEXT_GUID, 16484, 0, 1, 1, -5027.18, -806.981, 496.484, 3.00197, 120, 0);
SET @NEXT_GUID = @NEXT_GUID + 1;

-- Lieutenant Lisande (16490) — 幽暗城区域
INSERT INTO world.creature (guid, id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, spawntimesecs, MovementType) VALUES
(@NEXT_GUID, 16490, 1, 1, 1, -1346.69, 192.28, 61.5735, 4.38078, 120, 0);
SET @NEXT_GUID = @NEXT_GUID + 1;

-- Lieutenant Dagel (16493) — 奥格瑞玛区域
INSERT INTO world.creature (guid, id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, spawntimesecs, MovementType) VALUES
(@NEXT_GUID, 16493, 1, 1, 1, 1512.34, -4403.79, 19.7539, 5.77704, 120, 0);
SET @NEXT_GUID = @NEXT_GUID + 1;

-- Lieutenant Beitha (16495) — 雷霆崖区域
INSERT INTO world.creature (guid, id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, spawntimesecs, MovementType) VALUES
(@NEXT_GUID, 16495, 1, 1, 1, 9939.37, 2114.4, 1328.61, 6.10865, 120, 0);
SET @NEXT_GUID = @NEXT_GUID + 1;

-- Argent Emissary (16285) — 白銀使者，事件期间出现在各主城
INSERT INTO world.creature (guid, id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, spawntimesecs, MovementType) VALUES
(@NEXT_GUID, 16285, 0, 1, 1, 2247.79, -5317.31, 82.1935, 1.5708, 120, 0); SET @NEXT_GUID = @NEXT_GUID + 1;
INSERT INTO world.creature (guid, id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, spawntimesecs, MovementType) VALUES
(@NEXT_GUID, 16285, 0, 1, 1, -8830.83, 640.758, 94.528, 4.2237, 120, 0); SET @NEXT_GUID = @NEXT_GUID + 1;
INSERT INTO world.creature (guid, id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, spawntimesecs, MovementType) VALUES
(@NEXT_GUID, 16285, 0, 1, 1, -4935.48, -990.115, 501.539, 2.23402, 120, 0); SET @NEXT_GUID = @NEXT_GUID + 1;
INSERT INTO world.creature (guid, id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, spawntimesecs, MovementType) VALUES
(@NEXT_GUID, 16285, 1, 1, 1, 9918.94, 2518.57, 1317.64, 3.47321, 120, 0); SET @NEXT_GUID = @NEXT_GUID + 1;

-- Argent Messenger (16359) — 银色传令官
INSERT INTO world.creature (guid, id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, spawntimesecs, MovementType) VALUES
(@NEXT_GUID, 16359, 1, 1, 1, 1585.22, -4420.51, 8.32929, 3.45575, 120, 0); SET @NEXT_GUID = @NEXT_GUID + 1;
INSERT INTO world.creature (guid, id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, spawntimesecs, MovementType) VALUES
(@NEXT_GUID, 16359, 1, 1, 1, -1272.73, 75.3866, 128.153, 0.401426, 120, 0); SET @NEXT_GUID = @NEXT_GUID + 1;
INSERT INTO world.creature (guid, id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, spawntimesecs, MovementType) VALUES
(@NEXT_GUID, 16359, 0, 1, 1, 1578.8, 241.731, -61.994, 2.93215, 120, 0); SET @NEXT_GUID = @NEXT_GUID + 1;
INSERT INTO world.creature (guid, id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, spawntimesecs, MovementType) VALUES
(@NEXT_GUID, 16359, 530, 1, 1, 9525.33, -7347.89, 14.4149, 1.71042, 120, 0);

-- =====================================================================
-- 2. game_event_creature 绑定到事件 17（天灾入侵）
-- =====================================================================
DELETE FROM world.game_event_creature WHERE eventEntry = 17;

INSERT INTO world.game_event_creature (eventEntry, guid)
SELECT 17, guid FROM world.creature WHERE id IN (16361, 16484, 16490, 16493, 16495, 16285, 16359);

-- =====================================================================
-- 3. 补全 queststarter / questender 绑定
-- =====================================================================
DELETE FROM world.creature_queststarter WHERE id IN (16484, 16490, 16493, 16495, 16431);
INSERT INTO world.creature_queststarter (id, quest) VALUES
(16484, 9261), -- Lieutenant Nevell -> Investigate the Scourge of Ironforge
(16495, 9262), -- Lieutenant Beitha -> Investigate the Scourge of Darnassus
(16493, 9263), -- Lieutenant Dagel -> Investigate the Scourge of Orgrimmar
(16490, 9264), -- Lieutenant Lisande -> Investigate the Scourge of Thunder Bluff
(16431, 9292); -- Cracked Necrotic Crystal -> quest 9292

DELETE FROM world.creature_questender WHERE id IN (16484, 16490, 16493, 16495);
INSERT INTO world.creature_questender (id, quest) VALUES
(16484, 9261),
(16495, 9262),
(16493, 9263),
(16490, 9264);

-- =====================================================================
-- 4. creature_template 更新 — npcflag、gossip_menu_id 等
-- =====================================================================
UPDATE world.creature_template SET npcflag = npcflag | 1, unit_flags = unit_flags | (256 | 512), ScriptName = 'npc_cultist_engineer'
WHERE entry = 16230;
UPDATE world.creature_template SET gossip_menu_id = 7166 WHERE entry = 16230;

UPDATE world.creature_template SET npcflag = npcflag | 2, gossip_menu_id = 7164 WHERE entry IN (16285, 16359);
UPDATE world.creature_template SET npcflag = npcflag | 2 WHERE entry IN (16484, 16490, 16493, 16495);
UPDATE world.creature_template SET npcflag = npcflag | 1, unit_flags = unit_flags | 256 WHERE entry IN (16136, 16172);
UPDATE world.creature_template SET minlevel = 70, maxlevel = 70 WHERE entry IN (16230, 16172, 16298, 16141, 16299, 16143);
UPDATE world.creature_template SET minlevel = 69, maxlevel = 70 WHERE entry IN (16298, 16141, 16299);
UPDATE world.creature_template SET npcflag = npcflag | 1, unit_flags = unit_flags | 256, ArmorModifier = 0 WHERE entry IN (16136, 16172);
UPDATE world.creature_template SET minlevel = 71, maxlevel = 71 WHERE entry = 16379;

-- =====================================================================
-- 5. creature_template_addon — 添加缺失的 aura
-- =====================================================================
DELETE FROM world.creature_template_addon WHERE entry IN (14697, 16136, 16172, 16230, 16379, 16380, 16382, 16394, 16422, 16423, 16437, 16438);
INSERT INTO world.creature_template_addon (entry, mount, emote, visibilityDistanceType, auras) VALUES
(14697, 0, 0, 0, '28292 28126'),
(16136, 0, 0, 0, '28346'),
(16172, 0, 0, 0, '28346'),
(16230, 0, 0, 0, '29826'),
(16379, 0, 0, 0, '28292 28126'),
(16380, 0, 0, 0, '32900 28292 28126'),
(16382, 0, 0, 0, '28126'),
(16394, 0, 0, 0, '28126'),
(16422, 0, 0, 0, '28126 674'),
(16423, 0, 0, 0, '28126'),
(16437, 0, 0, 0, '28126'),
(16438, 0, 0, 0, '28126');

-- =====================================================================
-- 6. Cultist Engineer 对话菜单 + 条件
-- =====================================================================
DELETE FROM world.gossip_menu WHERE MenuID = 7166;
INSERT INTO world.gossip_menu (MenuID, TextID) VALUES (7166, 8436);

DELETE FROM world.npc_text WHERE ID = 8436;
INSERT INTO world.npc_text (ID, text0_0, text0_1, BroadcastTextID0, lang0, Probability0) VALUES
(8436, '这个教徒正处于深度入迷状态...', '这个教徒正处于深度入迷状态...', 0, 0, 100);

DELETE FROM world.gossip_menu_option WHERE MenuID = 7166;
INSERT INTO world.gossip_menu_option (MenuID, OptionID, OptionIcon, OptionText, OptionBroadcastTextID, OptionType, OptionNpcFlag) VALUES
(7166, 0, 0, '使用8个死亡符文打断他的仪式', 0, 1, 1);

DELETE FROM world.conditions WHERE SourceTypeOrReferenceId = 15 AND SourceGroup = 7166;
INSERT INTO world.conditions (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorType, ErrorTextId) VALUES
(15, 7166, 0, 0, 0, 2, 0, 22484, 8, 0, 0, 0, 0);

-- =====================================================================
-- 7. Shadow of Doom — 死亡符文掉落
-- =====================================================================
UPDATE world.creature_loot_template SET MinCount = 30, MaxCount = 30, Chance = 100 WHERE Entry = 16143 AND Item = 22484;

-- =====================================================================
-- 8. 符文圈(GameObject 181136) 初始化施法 SmartAI
-- =====================================================================
UPDATE world.gameobject_template SET AIName = 'SmartGameObjectAI' WHERE entry = 181136;
DELETE FROM world.smart_scripts WHERE entryorguid = 181136 AND source_type = 1;
INSERT INTO world.smart_scripts (entryorguid, source_type, id, link, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, action_type, action_param1, action_param2, target_type, comment) VALUES
(181136, 1, 0, 0, 37, 0, 100, 0, 0, 0, 0, 0, 11, 28344, 0, 1, 'Circle - On Initialize - Cast Create Crystal');

-- =====================================================================
-- 9. 添加部分 SmartAI 给 Spectral Soldier 等掉落 ZAP 水晶的 AI
-- =====================================================================
DELETE FROM world.smart_scripts WHERE entryorguid IN (16298, 16380, 14697) AND source_type = 0;
UPDATE world.creature_template SET AIName = 'SmartAI' WHERE entry IN (16298, 16380, 14697);
INSERT INTO world.smart_scripts (entryorguid, source_type, id, link, event_type, event_phase_mask, event_chance, event_flags, event_param1, event_param2, event_param3, event_param4, action_type, action_param1, action_param2, target_type, comment) VALUES
(16298, 0, 0, 0, 6, 0, 100, 0, 0, 0, 0, 0, 11, 28032, 2, 1, 'Spectral Soldier - On Just Died - Cast Zap Crystal'),
(16380, 0, 0, 0, 6, 0, 100, 0, 0, 0, 0, 0, 11, 28032, 2, 1, 'Bone Witch - On Just Died - Cast Zap Crystal'),
(14697, 0, 0, 0, 6, 0, 100, 0, 0, 0, 0, 0, 11, 28032, 2, 1, 'Lumbering Horror - On Just Died - Cast Zap Crystal');
