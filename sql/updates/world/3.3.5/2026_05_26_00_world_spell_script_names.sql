-- 这些法术脚本已通过 C++ 代码添加，需要在数据库中注册才能生效
-- 适用版本：TrinityCore 3.3.5 with NPCBots

DELETE FROM `spell_script_names` WHERE `spell_id` IN (8913, 19512, 34665, 44936, 50546, 49587, 52090, 43874, 43882);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
-- Quest 55: 摩本特·费尔（暮色森林） - 神圣净化弱化莫本特
(8913, 'spell_q55_sacred_cleansing'),
-- Quest 6124/6129: 消除疾病（贫瘠之地） - 使用动物医疗药膏
(19512, 'spell_q6124_6129_apply_salve'),
-- Quest 10255: 测试解药（地狱火半岛） - 地狱野猪变恐牙
(34665, 'spell_q10255_administer_antidote'),
-- Quest 11515: 血换血（影月谷） - 削弱魔血精英
(44936, 'spell_q11515_fel_siphon_dummy'),
-- Quest 12066: 海岸上的魔法焦点（北风苔原 - 龙骨荒野） - 魔网能量焦点控制指环效果
(50546, 'spell_dragonblight_focus_on_the_beach_control_ring'),
-- Quest 12459: That Which Creates Can Also Destroy（龙骨荒野） - 自然愤怒之种弱化精英
(49587, 'spell_dragonblight_seeds_of_natures_wrath'),
-- Quest 12659: Scalps!（灰熊丘陵） - 埃霍奈的小刀
(52090, 'spell_grizzly_hills_ahunaes_knife'),
-- Quest 11396/11399: Bring Down Those Shields（嚎风峡湾） - 法力护盾光环
(43874, 'spell_fjord_force_shield_arcane_purple_x3'),
-- Quest 11396/11399: Bring Down Those Shields（嚎风峡湾） - 净化水晶控制器虚影
(43882, 'spell_fjord_scourging_crystal_controller_dummy');
