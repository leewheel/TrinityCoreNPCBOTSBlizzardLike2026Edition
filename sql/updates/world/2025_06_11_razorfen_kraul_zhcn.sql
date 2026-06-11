-- Razorfen Kraul (47) creature_text zhCN localization
-- Source: broadcast_text_locale (official WotLK 3.3.5a zhCN client data)
-- Charlga's texts (6179-6183) have empty broadcast_text_locale entries; translations used from verified sources
-- Date: 2025-06-11

-- Overlord Ramtusk (4420)
UPDATE `creature_text` SET `Text`='胜利！为了阿迦玛甘而战！' WHERE `CreatureID`=4420 AND `GroupID`=0 AND `ID`=0;

-- Charlga Razorflank (4421) — broadcast_text_locale rows are empty
UPDATE `creature_text` SET `Text`='讨厌的小崽子。让我来教训教训你们！' WHERE `CreatureID`=4421 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='下一个是谁？' WHERE `CreatureID`=4421 AND `GroupID`=1 AND `ID`=0;
UPDATE `creature_text` SET `Text`='你们这些外来者将为侵犯我们的领地付出代价！' WHERE `CreatureID`=4421 AND `GroupID`=2 AND `ID`=0;
UPDATE `creature_text` SET `Text`='哈！这里由我的力量主宰！' WHERE `CreatureID`=4421 AND `GroupID`=3 AND `ID`=0;
UPDATE `creature_text` SET `Text`='我们的新盟友会为我们报仇的！' WHERE `CreatureID`=4421 AND `GroupID`=4 AND `ID`=0;

-- Agathelos the Raging (4422) / Raging Agam'ar (4514) — shared broadcast_text 38630
UPDATE `creature_text` SET `Text`='%s进入狂暴状态！' WHERE `CreatureID`=4422 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='%s进入狂暴状态！' WHERE `CreatureID`=4514 AND `GroupID`=0 AND `ID`=0;

-- Willix the Importer (4508)
UPDATE `creature_text` SET `Text`='唷，唷！终于出来了。不过接下来还有危险，所以要保持警惕。' WHERE `CreatureID`=4508 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='那上面就是卡尔加·刺肋的住处。那个该死的又干又瘪的老太婆。' WHERE `CreatureID`=4508 AND `GroupID`=1 AND `ID`=0;
UPDATE `creature_text` SET `Text`='这个沟里就有蓝叶薯！它们可都是唾手可得的金子啊！' WHERE `CreatureID`=4508 AND `GroupID`=2 AND `ID`=0;
UPDATE `creature_text` SET `Text`='这里到处都潜伏着危险。' WHERE `CreatureID`=4508 AND `GroupID`=3 AND `ID`=0;
UPDATE `creature_text` SET `Text`='我不明白这些愚蠢的动物是怎么在这种地方生存的……闻起来真臭！' WHERE `CreatureID`=4508 AND `GroupID`=4 AND `ID`=0;
UPDATE `creature_text` SET `Text`='我想我看到了一条可以走出这片荆棘林的路。' WHERE `CreatureID`=4508 AND `GroupID`=5 AND `ID`=0;
UPDATE `creature_text` SET `Text`='能走出这条臭沟真好，尽管出去了也好不了多少。' WHERE `CreatureID`=4508 AND `GroupID`=6 AND `ID`=0;
UPDATE `creature_text` SET `Text`='终于！我终于能走出这个地方了。' WHERE `CreatureID`=4508 AND `GroupID`=7 AND `ID`=0;
UPDATE `creature_text` SET `Text`='我想在回到棘齿城之前休息一下。谢谢你的帮助！' WHERE `CreatureID`=4508 AND `GroupID`=8 AND `ID`=0;
UPDATE `creature_text` SET `Text`='好了，现在我要起身去棘齿城了！祝你好运！' WHERE `CreatureID`=4508 AND `GroupID`=9 AND `ID`=0;
UPDATE `creature_text` SET `Text`='呃！$n正朝着我们过来了！' WHERE `CreatureID`=4508 AND `GroupID`=10 AND `ID`=0;
UPDATE `creature_text` SET `Text`='啊啊啊！$n朝我冲过来了！' WHERE `CreatureID`=4508 AND `GroupID`=10 AND `ID`=1;
UPDATE `creature_text` SET `Text`='$n正在向这里冲来！做好战斗准备！' WHERE `CreatureID`=4508 AND `GroupID`=10 AND `ID`=2;
UPDATE `creature_text` SET `Text`='救命！快帮我把这$n搞定！' WHERE `CreatureID`=4508 AND `GroupID`=10 AND `ID`=3;
