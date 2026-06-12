-- Sunken Temple (109) creature_text zhCN + spell_script for Awaken Soulflayer
-- Date: 2025-06-12

-- Shade of Eranikus (5709)
UPDATE `creature_text` SET `Text`='这股邪恶绝不能进入这个世界！来吧，我的孩子们！' WHERE `CreatureID`=5709 AND `GroupID`=0 AND `ID`=0;

-- Jammal'an the Prophet (5710)
UPDATE `creature_text` SET `Text`='护盾已破！起来吧，阿塔莱！起来！' WHERE `CreatureID`=5710 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='加入我们！' WHERE `CreatureID`=5710 AND `GroupID`=1 AND `ID`=0;
UPDATE `creature_text` SET `Text`='夺魂者来了！' WHERE `CreatureID`=5710 AND `GroupID`=2 AND `ID`=0;
UPDATE `creature_text` SET `Text`='哈卡将再次复活！' WHERE `CreatureID`=5710 AND `GroupID`=3 AND `ID`=0;

-- Register Awaken the Soulflayer spell script (port from AzerothCore)
INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (12346, 'spell_sunken_temple_awaken_soulflayer');
