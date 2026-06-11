-- Uldaman (70) creature_text zhCN localization
-- Source: broadcast_text_locale (official WotLK 3.3.5a zhCN client data)
-- Date: 2025-06-11

-- Archaedas (2748)
UPDATE `creature_text` SET `Text`='谁胆敢唤醒阿扎达斯？谁胆敢触怒造物之神？' WHERE `CreatureID`=2748 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='醒来吧！我的奴仆！保护圆盘！' WHERE `CreatureID`=2748 AND `GroupID`=1 AND `ID`=0;
UPDATE `creature_text` SET `Text`='兄弟们，到我这儿来！为创世者而战！' WHERE `CreatureID`=2748 AND `GroupID`=2 AND `ID`=0;
UPDATE `creature_text` SET `Text`='鲁莽的蠢货！' WHERE `CreatureID`=2748 AND `GroupID`=3 AND `ID`=0;
UPDATE `creature_text` SET `Text`='%s从他的岩石外壳中解脱了出来！' WHERE `CreatureID`=2748 AND `GroupID`=4 AND `ID`=0;

-- Ironaya (7228) — broadcast_text_locale 3261 is empty
UPDATE `creature_text` SET `Text`='没人能盗取造物之神的秘密！' WHERE `CreatureID`=7228 AND `GroupID`=0 AND `ID`=0;

-- Galgann Firehammer (7291)
UPDATE `creature_text` SET `Text`='以索瑞森胡子的名义！杀死他们！' WHERE `CreatureID`=7291 AND `GroupID`=0 AND `ID`=0;

-- Grimlok (4854)
UPDATE `creature_text` SET `Text`='我，格雷姆洛克，是国王！' WHERE `CreatureID`=4854 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='死吧！死吧！' WHERE `CreatureID`=4854 AND `GroupID`=2 AND `ID`=0;

-- Frenzy texts (shared broadcast_text 38630)
UPDATE `creature_text` SET `Text`='%s进入狂暴状态！' WHERE `CreatureID` IN (4851,4855,7175,7320) AND `GroupID`=0 AND `ID`=0;
