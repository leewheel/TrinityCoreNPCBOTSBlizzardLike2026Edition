-- Fix startup error: conditions on gossip menu 7165 option 0 without gossip_menu_option row.
-- Introduced by scourge invasion argent quartermaster data (2026_06_09_06 / 2026_06_11_00).

DELETE FROM `gossip_menu_option` WHERE `MenuID` = 7165 AND `OptionID` = 0;
INSERT INTO `gossip_menu_option` (`MenuID`, `OptionID`, `OptionIcon`, `OptionText`, `OptionBroadcastTextID`, `OptionType`, `OptionNpcFlag`, `ActionMenuID`, `ActionPoiID`, `BoxCoded`, `BoxMoney`, `BoxText`, `BoxBroadcastTextID`, `VerifiedBuild`) VALUES
(7165, 0, 1, '我想使用死亡符文。', 0, 3, 128, 0, 0, 0, 0, '', 0, 0);

-- Acore: both Argent Quartermaster and Outfitter use menu 7165 + quest 9153 condition.
UPDATE `creature_template` SET `gossip_menu_id` = 7165 WHERE `entry` IN (16385, 16786, 16787, 29360);
UPDATE `creature_template` SET `npcflag` = `npcflag` | 129, `gossip_menu_id` = 7230 WHERE `entry` = 29379;

-- Camp outfitter menu (16363) — vendor option without separate condition row.
DELETE FROM `gossip_menu_option` WHERE `MenuID` IN (7230, 7231, 7232) AND `OptionID` = 0;
INSERT INTO `gossip_menu_option` (`MenuID`, `OptionID`, `OptionIcon`, `OptionText`, `OptionBroadcastTextID`, `OptionType`, `OptionNpcFlag`, `ActionMenuID`, `ActionPoiID`, `BoxCoded`, `BoxMoney`, `BoxText`, `BoxBroadcastTextID`, `VerifiedBuild`) VALUES
(7230, 0, 1, '看看这些装备，或许有你用得上的。', 0, 3, 128, 0, 0, 0, 0, '', 0, 0),
(7231, 0, 1, '看看这些装备，或许有你用得上的。', 0, 3, 128, 0, 0, 0, 0, '', 0, 0),
(7232, 0, 1, '用死亡符文来换取强效黎明印记。', 0, 3, 128, 0, 0, 0, 0, '', 0, 0);
