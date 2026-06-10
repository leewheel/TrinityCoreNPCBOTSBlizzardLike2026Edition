-- Scourge Invasion: loot, minion addons, quest mail rewards (ported from Acore 2025_08_29_00 / 2025_07_12_01)

-- Necrotic Runes (22484)
DELETE FROM `creature_loot_template` WHERE `Item` = 22484;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(14697, 22484, 0, 100, 0, 1, 0, 2, 3, 'Lumbering Horror - Necrotic Rune'),
(16379, 22484, 0, 100, 0, 1, 0, 2, 3, 'Spirit of the Damned - Necrotic Rune'),
(16380, 22484, 0, 100, 0, 1, 0, 2, 3, 'Bone Witch - Necrotic Rune'),
(16143, 22484, 0, 100, 0, 1, 0, 30, 30, 'Shadow of Doom - Necrotic Rune'),
(16141, 22484, 0, 33.33, 0, 1, 0, 1, 1, 'Ghoul Berserker - Necrotic Rune'),
(16298, 22484, 0, 33.33, 0, 1, 0, 1, 1, 'Spectral Soldier - Necrotic Rune'),
(16299, 22484, 0, 33.33, 0, 1, 0, 1, 1, 'Skeletal Shocktrooper - Necrotic Rune'),
(16383, 22484, 0, 33.33, 0, 1, 0, 1, 1, 'Flameshocker - Necrotic Rune');

-- Sealed Research Report items (mail quest follow-ups)
DELETE FROM `creature_loot_template` WHERE `Item` IN (22970, 22972, 22973, 22974, 22975, 22977);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(16141, 22970, 0, 2, 0, 1, 1, 1, 1, 'Ghoul Berserker - A Bloodstained Envelope'),
(16141, 22972, 0, 2, 0, 1, 1, 1, 1, 'Ghoul Berserker - A Careworn Note'),
(16141, 22973, 0, 2, 0, 1, 1, 1, 1, 'Ghoul Berserker - A Crumpled Missive'),
(16141, 22974, 0, 2, 0, 1, 1, 1, 1, 'Ghoul Berserker - A Ragged Page'),
(16141, 22975, 0, 2, 0, 1, 1, 1, 1, 'Ghoul Berserker - A Smudged Document'),
(16141, 22977, 0, 2, 0, 1, 1, 1, 1, 'Ghoul Berserker - A Torn Letter'),
(16298, 22970, 0, 2, 0, 1, 1, 1, 1, 'Spectral Soldier - A Bloodstained Envelope'),
(16298, 22972, 0, 2, 0, 1, 1, 1, 1, 'Spectral Soldier - A Careworn Note'),
(16298, 22973, 0, 2, 0, 1, 1, 1, 1, 'Spectral Soldier - A Crumpled Missive'),
(16298, 22974, 0, 2, 0, 1, 1, 1, 1, 'Spectral Soldier - A Ragged Page'),
(16298, 22975, 0, 2, 0, 1, 1, 1, 1, 'Spectral Soldier - A Smudged Document'),
(16298, 22977, 0, 2, 0, 1, 1, 1, 1, 'Spectral Soldier - A Torn Letter'),
(16299, 22970, 0, 2, 0, 1, 1, 1, 1, 'Skeletal Shocktrooper - A Bloodstained Envelope'),
(16299, 22972, 0, 2, 0, 1, 1, 1, 1, 'Skeletal Shocktrooper - A Careworn Note'),
(16299, 22973, 0, 2, 0, 1, 1, 1, 1, 'Skeletal Shocktrooper - A Crumpled Missive'),
(16299, 22974, 0, 2, 0, 1, 1, 1, 1, 'Skeletal Shocktrooper - A Ragged Page'),
(16299, 22975, 0, 2, 0, 1, 1, 1, 1, 'Skeletal Shocktrooper - A Smudged Document'),
(16299, 22977, 0, 2, 0, 1, 1, 1, 1, 'Skeletal Shocktrooper - A Torn Letter'),
(16383, 22970, 0, 2, 0, 1, 1, 1, 1, 'Flameshocker - A Bloodstained Envelope'),
(16383, 22972, 0, 2, 0, 1, 1, 1, 1, 'Flameshocker - A Careworn Note'),
(16383, 22973, 0, 2, 0, 1, 1, 1, 1, 'Flameshocker - A Crumpled Missive'),
(16383, 22974, 0, 2, 0, 1, 1, 1, 1, 'Flameshocker - A Ragged Page'),
(16383, 22975, 0, 2, 0, 1, 1, 1, 1, 'Flameshocker - A Smudged Document'),
(16383, 22977, 0, 2, 0, 1, 1, 1, 1, 'Flameshocker - A Torn Letter');

UPDATE `creature_loot_template` SET `Chance` = 25 WHERE `Item` = 22892;

-- Invasion camp minions: despawn timer + purple particles (Acore 2025_07_12_01)
DELETE FROM `creature_template_addon` WHERE `entry` IN (16141, 16298, 16299);
INSERT INTO `creature_template_addon` (`entry`, `path_id`, `mount`, `emote`, `visibilityDistanceType`, `auras`) VALUES
(16141, 0, 0, 0, 0, '28090 28126'),
(16298, 0, 0, 0, 0, '28090 28126 674'),
(16299, 0, 0, 0, 0, '28090 28126');

-- Quest mail rewards for sealed research reports
DELETE FROM `mail_loot_template` WHERE `Entry` IN (171, 172, 173, 174, 175, 176, 177);
INSERT INTO `mail_loot_template` (`Entry`, `Item`, `Comment`) VALUES
(171, 22723, 'A Letter from the Keeper of the Rolls'),
(172, 23008, 'Sealed Research Report'),
(173, 23010, 'Sealed Research Report'),
(174, 23011, 'Sealed Research Report'),
(175, 23012, 'Sealed Research Report'),
(176, 23013, 'Sealed Research Report'),
(177, 23016, 'Sealed Research Report');

UPDATE `quest_template_addon` SET `RewardMailDelay` = 1 WHERE `ID` IN (9295, 9299, 9300, 9301, 9302, 9304);
UPDATE `quest_template_addon` SET `RewardMailTemplateID` = 172 WHERE `ID` = 9299;
UPDATE `quest_template_addon` SET `RewardMailTemplateID` = 173 WHERE `ID` = 9295;
UPDATE `quest_template_addon` SET `RewardMailTemplateID` = 174 WHERE `ID` = 9300;
UPDATE `quest_template_addon` SET `RewardMailTemplateID` = 175 WHERE `ID` = 9302;
UPDATE `quest_template_addon` SET `RewardMailTemplateID` = 176 WHERE `ID` = 9301;
UPDATE `quest_template_addon` SET `RewardMailTemplateID` = 177 WHERE `ID` = 9304;

-- Argent Quartermaster vendor gossip requires quest 9153 rewarded
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 15 AND `SourceGroup` = 7165 AND `ConditionValue1` = 9153;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(15, 7165, 0, 0, 0, 8, 0, 9153, 0, 0, 0, 0, 0, '', 'Argent Quartermaster - Show vendor option after Under the Shadow');
