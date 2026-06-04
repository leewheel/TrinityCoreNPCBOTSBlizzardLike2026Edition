-- 移植自 Acore: Pit of Saron (萨隆矿坑) 事件系统注册
-- 
-- 注册 Creature ScriptName：
--   36788 Deathwhisper Necrolyte → npc_pos_deathwhisper_necrolyte
--   36794 Scourgelord Tyrannus    → npc_pos_tyrannus_events
--   36848 Icicle Trigger          → npc_pos_icicle_trigger
--   36990 Sylvanas Part1          → npc_pos_leader
--   36993 Jaina Part1             → npc_pos_leader
--   37580 Martin Victus 2         → npc_pos_martin_or_gorkun_second
--   37581 Gorkun Ironskull 2      → npc_pos_martin_or_gorkun_second
--   37591 Martin Victus 1         → npc_pos_after_first_boss
--   37592 Gorkun Ironskull 1      → npc_pos_after_first_boss
--   36888/36889 Freed Slave       → npc_pos_freed_slave
--   38188 Jaina Part2             → npc_pos_leader_second
--   38189 Sylvanas Part2          → npc_pos_leader_second
--
-- 注册 SpellScript：
--   70132 (Empowered Blizzard)    → spell_pos_empowered_blizzard_aura
--   71281 (Slave Trigger)         → spell_pos_slave_trigger_closest
--   68198 (Rimefang Frost Nova)   → spell_pos_rimefang_frost_nova
--   69603/69604/70285/70286 (Blight) → spell_pos_blight_aura
--   70292/71316/71317 (Glacial Strike) → spell_pos_glacial_strike_aura

-- Creature ScriptName
UPDATE `creature_template` SET `ScriptName` = 'npc_pos_deathwhisper_necrolyte' WHERE `entry` = 36788;
UPDATE `creature_template` SET `ScriptName` = 'npc_pos_tyrannus_events' WHERE `entry` = 36794;
UPDATE `creature_template` SET `ScriptName` = 'npc_pos_icicle_trigger' WHERE `entry` = 36848;
UPDATE `creature_template` SET `ScriptName` = 'npc_pos_leader' WHERE `entry` IN (36990, 36993);
UPDATE `creature_template` SET `ScriptName` = 'npc_pos_martin_or_gorkun_second' WHERE `entry` IN (37580, 37581);
UPDATE `creature_template` SET `ScriptName` = 'npc_pos_after_first_boss' WHERE `entry` IN (37591, 37592);
UPDATE `creature_template` SET `ScriptName` = 'npc_pos_freed_slave' WHERE `entry` IN (36888, 36889, 37572, 37575, 37576, 37577, 37578, 37579);
UPDATE `creature_template` SET `ScriptName` = 'npc_pos_leader_second' WHERE `entry` IN (38188, 38189);

-- Spell ScriptNames
DELETE FROM `spell_script_names` WHERE `spell_id` IN (70132, 71281, 68198, 69603, 69604, 70285, 70286, 70292, 71316, 71317);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(70132, 'spell_pos_empowered_blizzard_aura'),
(71281, 'spell_pos_slave_trigger_closest'),
(68198, 'spell_pos_rimefang_frost_nova'),
(69603, 'spell_pos_blight_aura'),
(69604, 'spell_pos_blight_aura'),
(70285, 'spell_pos_blight_aura'),
(70286, 'spell_pos_blight_aura'),
(70292, 'spell_pos_glacial_strike_aura'),
(71316, 'spell_pos_glacial_strike_aura'),
(71317, 'spell_pos_glacial_strike_aura');
