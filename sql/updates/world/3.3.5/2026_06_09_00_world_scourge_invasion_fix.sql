-- 天灾入侵系统：补遗 game_event + 初始状态 + npc_si_controller 刷怪
-- 移植时遗漏了三个关键部分，导致系统无法工作
-- 参考 Acore 2025_07_12_01.sql

-- 1. 创建缺失的 game_event 条目 120-130（内部子事件，world_event=5）
DELETE FROM `game_event` WHERE `eventEntry` BETWEEN 120 AND 130;
INSERT INTO `game_event` (`eventEntry`, `description`, `world_event`) VALUES
(120, 'Scourge Invasion - Boss in instance activation', 5),
(121, 'Scourge Invasion - Attacking Winterspring', 5),
(122, 'Scourge Invasion - Attacking Tanaris', 5),
(123, 'Scourge Invasion - Attacking Azshara', 5),
(124, 'Scourge Invasion - Attacking Blasted Lands', 5),
(125, 'Scourge Invasion - Attacking Eastern Plaguelands', 5),
(126, 'Scourge Invasion - Attacking Burning Steppes', 5),
(127, 'Scourge Invasion - 50 Invasions Done', 5),
(128, 'Scourge Invasion - 100 Invasions Done', 5),
(129, 'Scourge Invasion - 150 Invasions Done', 5),
(130, 'Scourge Invasion - Invasions Done', 5);

-- 2. 更新事件 17 为内部事件（world_event=5）
UPDATE `game_event` SET `world_event` = 5 WHERE `eventEntry` = 17;

-- 3. 插入初始状态数据，启用系统
--    state=1 (SI_STATE_ENABLED) 使 LoadFromDB() 自动调用 StartEvents()
--    其他字段=0，StartEvents() 会填入初始计时器
DELETE FROM `scourge_invasion_state`;
INSERT INTO `scourge_invasion_state` (`zoneId`, `attackTimer`, `remainingNecropoli`, `battlesWon`, `lastAttackZone`, `state`) VALUES
(618, 0, 0, 0, 0, 1),   -- Winterspring
(440, 0, 0, 0, 0, 1),   -- Tanaris
(16,  0, 0, 0, 0, 1),   -- Azshara
(4,   0, 0, 0, 0, 1),   -- Blasted Lands
(139, 0, 0, 0, 0, 1),   -- Eastern Plaguelands
(46,  0, 0, 0, 0, 1),   -- Burning Steppes
(1497, 0, 0, 0, 0, 1),  -- Undercity
(1519, 0, 0, 0, 0, 1);  -- Stormwind

-- 4. 刷怪 npc_si_controller (16356) — 驱动更新循环的不可见 NPC
--    位置：东瘟疫之地（同 Acore），setActive=true 即使用户不在附近也会 tick
--    GUID: 使用一个高值避免冲突，如需调整请替换为 MAX(guid)+1
SET @SI_CONTROLLER_GUID = (SELECT COALESCE(MAX(guid), 0) + 1 FROM creature);
DELETE FROM `creature` WHERE `id` = 16356;
INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `MovementType`, `VerifiedBuild`) VALUES
(@SI_CONTROLLER_GUID, 16356, 0, 1, 1, -8939.1, -2319.45, 132.649, 2.75762, 300, 0, 0, 0);
