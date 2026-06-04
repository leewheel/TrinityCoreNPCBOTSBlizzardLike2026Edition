-- 移植自 Acore: DK 新手区第二章法术脚本注册
-- 
-- 注册 SpellScript：
--   52781 (Persuasive Strike) → spell_chapter2_persuasive_strike
--   53098 (Portal Effect Acherus) → spell_portal_effect_acherus

DELETE FROM `spell_script_names` WHERE `spell_id` IN (52781, 53098);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(52781, 'spell_chapter2_persuasive_strike'),
(53098, 'spell_portal_effect_acherus');

-- 注册 Creature ScriptName
UPDATE `creature_template` SET `ScriptName` = 'npc_acherus_necromancer' WHERE `entry` = 28889;
UPDATE `creature_template` SET `ScriptName` = 'npc_gothik_the_harvester' WHERE `entry` = 28890;
