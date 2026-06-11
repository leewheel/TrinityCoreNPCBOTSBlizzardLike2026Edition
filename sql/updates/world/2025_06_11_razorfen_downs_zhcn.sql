-- Razorfen Downs (129) creature_text zhCN localization
-- Source: broadcast_text_locale (official WotLK 3.3.5a zhCN client data)
-- Date: 2025-06-11

-- Plaguemaw the Rotting (7357)
UPDATE `creature_text` SET `Text`='我们要奴役野猪人！' WHERE `CreatureID`=7357 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='我们要统治这片贫瘠之地！' WHERE `CreatureID`=7357 AND `GroupID`=1 AND `ID`=0;
UPDATE `creature_text` SET `Text`='亡灵天灾很快就将统治世界！' WHERE `CreatureID`=7357 AND `GroupID`=2 AND `ID`=0;
UPDATE `creature_text` SET `Text`='兄弟们，杀了他们！为了亡灵天灾而战！' WHERE `CreatureID`=7357 AND `GroupID`=3 AND `ID`=0;

-- Amnennar the Coldbringer (7358)
UPDATE `creature_text` SET `Text`='你不会从这儿活着出去的。' WHERE `CreatureID`=7358 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='小菜一碟。' WHERE `CreatureID`=7358 AND `GroupID`=1 AND `ID`=0;
UPDATE `creature_text` SET `Text`='我是巫妖王的得力助手！' WHERE `CreatureID`=7358 AND `GroupID`=2 AND `ID`=0;
UPDATE `creature_text` SET `Text`='到我这儿来，仆从们！' WHERE `CreatureID`=7358 AND `GroupID`=3 AND `ID`=0;
UPDATE `creature_text` SET `Text`='来吧，帮助你们的主人！' WHERE `CreatureID`=7358 AND `GroupID`=4 AND `ID`=0;
UPDATE `creature_text` SET `Text`='%s开始从冰冷的空气中召唤鬼魂！' WHERE `CreatureID`=7358 AND `GroupID`=5 AND `ID`=0;

-- Glutton (8567)
UPDATE `creature_text` SET `Text`='我闻到活人的臭气！' WHERE `CreatureID`=8567 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='我要把你们统统吃掉！' WHERE `CreatureID`=8567 AND `GroupID`=1 AND `ID`=0;
UPDATE `creature_text` SET `Text`='%s感到饥饿难耐！' WHERE `CreatureID`=8567 AND `GroupID`=2 AND `ID`=0;
UPDATE `creature_text` SET `Text`='%s饥饿难耐！' WHERE `CreatureID`=8567 AND `GroupID`=3 AND `ID`=0;

-- Belnistrasz (8516)
UPDATE `creature_text` SET `Text`='准备好，靠近点。要是你放松戒备的话，那些恶魔会从暗处跳出来偷袭你的。' WHERE `CreatureID`=8516 AND `GroupID`=0 AND `ID`=0;
UPDATE `creature_text` SET `Text`='嗯，我大约需要5分钟的时间来完成关闭它的仪式。在我开始之后，你一定要好好保护我，否则我们都会没命的！' WHERE `CreatureID`=8516 AND `GroupID`=1 AND `ID`=0;
UPDATE `creature_text` SET `Text`='还剩三分钟——我感觉到能量正在不断汇集！继续保护我！' WHERE `CreatureID`=8516 AND `GroupID`=2 AND `ID`=0;
UPDATE `creature_text` SET `Text`='只有两分钟时间了！我们只完成了一半，不要放松警惕！' WHERE `CreatureID`=8516 AND `GroupID`=3 AND `ID`=0;
UPDATE `creature_text` SET `Text`='再坚持一分钟！坚持住，仪式就要完成了！' WHERE `CreatureID`=8516 AND `GroupID`=4 AND `ID`=0;
UPDATE `creature_text` SET `Text`='很好——我们成功了！雕像中的火焰即将永远熄灭！我就知道你能行的！' WHERE `CreatureID`=8516 AND `GroupID`=5 AND `ID`=0;
UPDATE `creature_text` SET `Text`='你会后悔遇见我的，$n。' WHERE `CreatureID`=8516 AND `GroupID`=6 AND `ID`=0;
UPDATE `creature_text` SET `Text`='小心$n！' WHERE `CreatureID`=8516 AND `GroupID`=7 AND `ID`=0;

-- Henry Stern (8696)
UPDATE `creature_text` SET `Text`='$n，我要再次对你表示感谢。现在我会留在这里，等安全了再离开。' WHERE `CreatureID`=8696 AND `GroupID`=0 AND `ID`=0;

-- Enraged texts (shared broadcast_text 24144)
UPDATE `creature_text` SET `Text`='%s变得愤怒了！' WHERE `CreatureID` IN (7327,7328,7329,7332) AND `GroupID`=0 AND `ID`=0;
