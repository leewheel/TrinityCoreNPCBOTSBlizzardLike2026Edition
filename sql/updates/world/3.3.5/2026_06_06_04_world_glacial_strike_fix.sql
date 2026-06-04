-- 修正 Glacial Strike aura 脚本注册：使用核心通用版替代自定义版
DELETE FROM `spell_script_names` WHERE `spell_id` IN (70292, 71316, 71317) AND `ScriptName` = 'spell_pos_glacial_strike_aura';
DELETE FROM `spell_script_names` WHERE `spell_id` IN (70292, 71316, 71317) AND `ScriptName` = 'spell_gen_remove_on_full_health_pct';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(70292, 'spell_gen_remove_on_full_health_pct'),
(71316, 'spell_gen_remove_on_full_health_pct'),
(71317, 'spell_gen_remove_on_full_health_pct');
