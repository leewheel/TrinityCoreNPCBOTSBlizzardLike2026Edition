-- 移植自 Acore: 53111 Devour Humanoid aura (quest 12779 "An End to All Things")
DELETE FROM `spell_script_names` WHERE `spell_id` = 53111;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(53111, 'spell_death_knight_devour_humanoid_aura');
