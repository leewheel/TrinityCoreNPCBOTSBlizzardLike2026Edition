-- Gnomeregan (90) creature_text zhCN localization
-- Source: broadcast_text_locale (official WotLK 3.3.5a zhCN client data)
-- Date: 2025-06-11

-- Irradiated texts (shared across multiple creatures)
UPDATE `creature_text` SET `Text`='%s浑身是血，变得异常狂暴！' WHERE `CreatureID` IN (6206,6207,6211,6212,6222,6223,6224) AND `GroupID`=0 AND `ID`=0;

-- Dark Iron Agent (6212) aggro texts
UPDATE `creature_text` SET `Text`='不要穿过黑铁矮人的地盘，$C。' WHERE `CreatureID`=6212 AND `GroupID`=1 AND `ID`=0;
UPDATE `creature_text` SET `Text`='去死吧，$C。' WHERE `CreatureID`=6212 AND `GroupID`=1 AND `ID`=1;
UPDATE `creature_text` SET `Text`='感受一下黑铁矮人的力量吧！' WHERE `CreatureID`=6212 AND `GroupID`=1 AND `ID`=2;

-- Irradiated Gnomes (6222,6223,6224) aggro texts
UPDATE `creature_text` SET `Text`='我们不能放弃任何一个侏儒！' WHERE `CreatureID` IN (6222,6223,6224) AND `GroupID`=1 AND `ID`=0;
UPDATE `creature_text` SET `Text`='穴居人……好像永远也杀不光。该死的穴居人！去死吧！' WHERE `CreatureID` IN (6222,6223,6224) AND `GroupID`=1 AND `ID`=1;
UPDATE `creature_text` SET `Text`='疾病让我的视觉受损，但我知道你一定是穴居人。去死吧，该死的入侵者！' WHERE `CreatureID` IN (6222,6223,6224) AND `GroupID`=1 AND `ID`=2;
UPDATE `creature_text` SET `Text`='又是穴居人，去死吧！' WHERE `CreatureID` IN (6222,6223,6224) AND `GroupID`=1 AND `ID`=3;

-- Electrocutioner 6000 (6235)
UPDATE `creature_text` SET `Text`='电刑！' WHERE `CreatureID`=6235 AND `GroupID`=0 AND `ID`=0;

-- Grubbis (7361)
UPDATE `creature_text` SET `Text`='我们从地底而来！你们无法阻止我们！' WHERE `CreatureID`=7361 AND `GroupID`=0 AND `ID`=0;

-- Mekgineer Thermaplugg (7800)
UPDATE `creature_text` SET `Text`='篡位者！诺莫瑞根是我的！' WHERE `CreatureID`=7800 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='我的机器代表未来！它们会将你们统统摧毁的！' WHERE `CreatureID`=7800 AND `GroupID`=1 AND `ID`=0;
UPDATE `creature_text` SET `Text`='爆炸！更多爆炸！我需要更多爆炸！' WHERE `CreatureID`=7800 AND `GroupID`=2 AND `ID`=0;
UPDATE `creature_text` SET `Text`='站着别动！' WHERE `CreatureID`=7800 AND `GroupID`=3 AND `ID`=0;
