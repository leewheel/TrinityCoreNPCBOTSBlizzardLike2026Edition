-- Zul'Farrak (209) creature_text zhCN localization
-- Source: broadcast_text_locale (official WotLK 3.3.5a zhCN client data)
-- Date: 2025-06-12

-- Chief Ukorz Sandscalp (7267)
UPDATE `creature_text` SET `Text`='死吧，外来者！' WHERE `CreatureID`=7267 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='感受沙漠的狂暴吧！' WHERE `CreatureID`=7267 AND `GroupID`=1 AND `ID`=0;
UPDATE `creature_text` SET `Text`='沙怒部族拥有至高无上的统治权！' WHERE `CreatureID`=7267 AND `GroupID`=2 AND `ID`=0;
UPDATE `creature_text` SET `Text`='这片沙漠是属于我的！' WHERE `CreatureID`=7267 AND `GroupID`=3 AND `ID`=0;

-- Witch Doctor Zum'rah (7271)
UPDATE `creature_text` SET `Text`='你们怎敢侵入我的圣地！' WHERE `CreatureID`=7271 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='流沙会吞噬你的！' WHERE `CreatureID`=7271 AND `GroupID`=1 AND `ID`=1;
UPDATE `creature_text` SET `Text`='流沙会吞噬你的！' WHERE `CreatureID`=7271 AND `GroupID`=2 AND `ID`=2;
UPDATE `creature_text` SET `Text`='倒下吧！' WHERE `CreatureID`=7271 AND `GroupID`=2 AND `ID`=3;

-- Sandfury Executioner (7274)
UPDATE `creature_text` SET `Text`='让裁决开始执行吧！' WHERE `CreatureID`=7274 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='正义得到了伸张！' WHERE `CreatureID`=7274 AND `GroupID`=1 AND `ID`=0;

-- Sergeant Bly (7604)
UPDATE `creature_text` SET `Text`='什么？你怎么敢这么对我说话？！' WHERE `CreatureID`=7604 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='我们这就结束了？好吧，我一点都不喜欢你！' WHERE `CreatureID`=7604 AND `GroupID`=1 AND `ID`=0;

-- Weegli Blastfuse (7607)
UPDATE `creature_text` SET `Text`='啊，注意！' WHERE `CreatureID`=7607 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='好了，我这就去！' WHERE `CreatureID`=7607 AND `GroupID`=1 AND `ID`=0;

-- Antu'sul (8127)
UPDATE `creature_text` SET `Text`='苏利萨斯的孩子会保护他们的主人。苏醒吧，苏利萨斯！' WHERE `CreatureID`=8127 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='你们的午餐来了，孩子们。把它们全部吃光吧！' WHERE `CreatureID`=8127 AND `GroupID`=1 AND `ID`=0;
UPDATE `creature_text` SET `Text`='保护你们的主人！' WHERE `CreatureID`=8127 AND `GroupID`=2 AND `ID`=0;

-- Gossip texts (gossip_menu_option)
UPDATE `gossip_menu_option` SET `OptionText`='你现在能把那扇门炸开吗？' WHERE `MenuID`=940 AND `OptionID`=0;
UPDATE `gossip_menu_option` SET `OptionText`='就这样吧！我受够了帮你干活。让我们在战场上一决胜负吧！' WHERE `MenuID`=941 AND `OptionID`=1;
