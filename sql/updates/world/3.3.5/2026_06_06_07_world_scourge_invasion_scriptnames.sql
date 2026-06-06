-- 注册天灾入侵系统 ScriptNames（补遗）
-- 核心已实现 12 个 CreatureScript + 1 个 GameObjectScript + 3 个 SpellScript，
-- 但 2026_06_06_06_world_scourge_invasion.sql 未注册数据库映射，导致启动警告。
-- 参考 Acore 2025_07_12_01.sql 补全

-- 1. GameObject 脚本注册
UPDATE `gameobject_template` SET `ScriptName` = 'go_necropolis' WHERE `entry` IN (181154, 181215, 181223, 181374, 181373);

-- 2. Creature 脚本注册
UPDATE `creature_template` SET `ScriptName` = 'npc_herald_of_the_lich_king' WHERE `entry` = 16995;
UPDATE `creature_template` SET `ScriptName` = 'npc_necropolis' WHERE `entry` = 16401;
UPDATE `creature_template` SET `ScriptName` = 'npc_necropolis_health' WHERE `entry` = 16421;
UPDATE `creature_template` SET `ScriptName` = 'npc_necropolis_proxy' WHERE `entry` = 16398;
UPDATE `creature_template` SET `ScriptName` = 'npc_necropolis_relay' WHERE `entry` = 16386;
UPDATE `creature_template` SET `ScriptName` = 'npc_necrotic_shard' WHERE `entry` IN (16136, 16172);
UPDATE `creature_template` SET `ScriptName` = 'npc_minion_spawner' WHERE `entry` IN (16306, 16336, 16338);
UPDATE `creature_template` SET `ScriptName` = 'npc_cultist_engineer' WHERE `entry` = 16230;
UPDATE `creature_template` SET `ScriptName` = 'npc_flameshocker' WHERE `entry` = 16383;
UPDATE `creature_template` SET `ScriptName` = 'npc_pallid_horror' WHERE `entry` IN (16382, 16394);
UPDATE `creature_template` SET `ScriptName` = 'npc_si_controller' WHERE `entry` = 16356;

-- 3. Spell 脚本注册
DELETE FROM `spell_script_names` WHERE `spell_id` IN (28345, 28091, 28265);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(28345, 'spell_communique_trigger'),
(28091, 'spell_despawner_self'),
(28265, 'spell_scourge_invasion_scourge_strike');
