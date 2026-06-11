-- 天灾入侵数据热修复
-- 
-- 1. gameobject_template 190610 缺失（Acore 有，TC base 无）
--    这个 GO 是 "Orders from the Lich King"，由天灾怪物掉落
-- 2. SmartAI 修复：Ghoul Berserker(16141) event_type=2 缺少 Repeat flag
-- 3. SmartAI 修复：Shadow of Doom(16143) 使用了 Acore 特有但 TC 不支持的 action_type

-- ===================================================
-- 1. 补全缺失的 gameobject_template
-- ===================================================
DELETE FROM `gameobject_template` WHERE `entry` = 190610;
INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `size`, `Data0`, `Data1`, `Data2`, `Data3`, `Data6`, `AIName`, `VerifiedBuild`) VALUES
(190610, 1, 220, '巫妖王的命令', 1, 0, 1690, 1000, 190611, 33041, 'SmartGameObjectAI', 49345);

-- ===================================================
-- 2. Ghoul Berserker SmartAI 修复
-- ===================================================
-- event_type=2 (SMART_EVENT_RANGE) 需要 event_flags=1 才能重复触发
UPDATE `smart_scripts` SET `event_flags` = 1 WHERE `entryorguid` = 16141 AND `id` = 3;

-- ===================================================
-- 3. Shadow of Doom SmartAI 替换（移除 Acore 特有指令）
-- ===================================================
DELETE FROM `smart_scripts` WHERE `entryorguid` = 16143;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`) VALUES
(16143, 0, 0, 0, 54, 0, 100, 0, 0, 0, 0, 0, 11, 10389, 0, 1, 'Shadow of Doom - On Summon - Cast Spawn Smoke'),
(16143, 0, 1, 0, 0, 0, 100, 0, 2000, 2000, 2000, 2000, 11, 16568, 0, 2, 'Shadow of Doom - In Combat - Cast Mind Flay'),
(16143, 0, 2, 0, 0, 0, 100, 1, 3000, 3000, 3000, 3000, 11, 12542, 0, 2, 'Shadow of Doom - In Combat - Cast Fear'),
(16143, 0, 3, 0, 6, 0, 100, 0, 0, 0, 0, 0, 11, 28056, 0, 1, 'Shadow of Doom - On Death - Cast Zap Crystal Corpse'),
(16143, 0, 4, 0, 8, 0, 100, 0, 17680, 0, 0, 0, 41, 3000, 0, 1, 'Shadow of Doom - On Spellhit Spirit Spawn - Despawn');

-- ===================================================
-- 4. 军需官难度1模板与主模板 npcflag / gossip 同步
-- ===================================================
UPDATE `creature_template` SET `npcflag` = `npcflag` | 129, `gossip_menu_id` = 7230 WHERE `entry` = 29379;
UPDATE `creature_template` SET `npcflag` = `npcflag` | 129, `gossip_menu_id` = 7165 WHERE `entry` = 29360;
