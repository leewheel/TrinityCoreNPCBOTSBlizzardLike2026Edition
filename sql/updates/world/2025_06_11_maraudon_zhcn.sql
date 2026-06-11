-- Maraudon (349) creature_text zhCN localization
-- Source: broadcast_text_locale (official WotLK 3.3.5a zhCN client data)
-- Some broadcast_text_locale rows are empty; translated from verified sources
-- Date: 2025-06-11

-- Cavern Shambler (11793) / Corruptor (11794) — shared texts, broadcast_text_locale empty
UPDATE `creature_text` SET `Text`='你不属于这片花园。你的躯体将滋养我们可爱的造物！' WHERE `CreatureID` IN (11793,11794) AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='绝不能让花园被玷污！你必须被摧毁！' WHERE `CreatureID` IN (11793,11794) AND `GroupID`=0 AND `ID`=1;

-- Gizlock the Rare (13601)
UPDATE `creature_text` SET `Text`='我的！我的！我的！吉兹洛克是这片领域的统治者！你绝对不能泄露我的行踪！' WHERE `CreatureID`=13601 AND `GroupID`=0 AND `ID`=0;
