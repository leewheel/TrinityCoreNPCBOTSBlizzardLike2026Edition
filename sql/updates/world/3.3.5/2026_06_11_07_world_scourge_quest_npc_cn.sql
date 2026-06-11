-- Scourge invasion quest NPC dialogue: full Chinese (TrinityCore schema)
-- Updates quest_template + *_locale so zhCN clients see Chinese text

UPDATE `quest_template` SET
  `LogTitle` = '末日之影',
  `QuestDescription` = '在你攻破天灾军团的召唤法阵后，就要面对守护法阵的侍僧了。$B$B说老实话，这些家伙不是人，而是暗影，巫妖王最恐怖的爪牙。使用死灵石可令它们显形，魔法和武器攻击都能取它们的性命。',
  `LogDescription` = '前往一处召唤法阵，杀死一名末日之影，然后向东瘟疫之地圣光之愿礼拜堂的指挥官托马斯·海勒拉复命。',
  `AreaDescription` = '去东瘟疫之地找圣光之愿礼拜堂的指挥官托马斯·海勒拉。'
WHERE `ID` = 9085;

DELETE FROM `quest_template_locale` WHERE `ID` = 9085 AND `locale` = 'zhCN';
INSERT INTO `quest_template_locale` (`ID`,`locale`,`Title`,`Details`,`Objectives`,`EndText`,`CompletedText`,`ObjectiveText1`,`ObjectiveText2`,`ObjectiveText3`,`ObjectiveText4`,`VerifiedBuild`) VALUES
(9085,'zhCN','末日之影','在你攻破天灾军团的召唤法阵后，就要面对守护法阵的侍僧了。$B$B说老实话，这些家伙不是人，而是暗影，巫妖王最恐怖的爪牙。使用死灵石可令它们显形，魔法和武器攻击都能取它们的性命。','前往一处召唤法阵，杀死一名末日之影，然后向东瘟疫之地圣光之愿礼拜堂的指挥官托马斯·海勒拉复命。','去东瘟疫之地找圣光之愿礼拜堂的指挥官托马斯·海勒拉。','','','','','',0);

UPDATE `quest_template` SET
  `LogTitle` = '阴影之下',
  `QuestDescription` = '那些漂浮在空中的天灾浮空城污染了部落和联盟的土地。塔纳利斯、诅咒之地、冬泉谷和燃烧平原都笼罩在它们的阴影之下。$B$B我们只有协力作战，才能消灭敌人。$B$B在天灾入侵之地，有多处魔法阵支持着这些浮空城。我们决定要攻破法阵周围的防御势力。',
  `LogDescription` = '在你的地图上寻找天灾入侵的地区。击败浮空城周围的天灾士兵，摧毁一处浮空城。将10块死灵石交给东瘟疫之地圣光之愿礼拜堂的指挥官托马斯·海勒拉。',
  `AreaDescription` = '去东瘟疫之地找圣光之愿礼拜堂的指挥官托马斯·海勒拉。'
WHERE `ID` = 9153;

DELETE FROM `quest_template_locale` WHERE `ID` = 9153 AND `locale` = 'zhCN';
INSERT INTO `quest_template_locale` (`ID`,`locale`,`Title`,`Details`,`Objectives`,`EndText`,`CompletedText`,`ObjectiveText1`,`ObjectiveText2`,`ObjectiveText3`,`ObjectiveText4`,`VerifiedBuild`) VALUES
(9153,'zhCN','阴影之下','那些漂浮在空中的天灾浮空城污染了部落和联盟的土地。塔纳利斯、诅咒之地、冬泉谷和燃烧平原都笼罩在它们的阴影之下。$B$B我们只有协力作战，才能消灭敌人。$B$B在天灾入侵之地，有多处魔法阵支持着这些浮空城。我们决定要攻破法阵周围的防御势力。','在你的地图上寻找天灾入侵的地区。击败浮空城周围的天灾士兵，摧毁一处浮空城。将10块死灵石交给东瘟疫之地圣光之愿礼拜堂的指挥官托马斯·海勒拉。','去东瘟疫之地找圣光之愿礼拜堂的指挥官托马斯·海勒拉。','','','','','',0);

UPDATE `quest_template` SET
  `LogTitle` = '调查铁炉堡的天灾入侵',
  `QuestDescription` = '巫妖王的部队已经在我们鼻子底下扎了营，我们可不能坐以待毙！你会拿起武器跟这些可憎的入侵者作战，保卫铁炉堡的吧？$B$B城外出现了奇怪的符文法阵，它散发着神秘的能量，亡灵和悬浮在半空的浮空城也被同样的能量环绕着。我想这个符文法阵对天灾士兵们必然有着重要的意义，因此想让你调查此事。去和城外的亡灵作战吧，带回你杀死他们的证据，以及你的调查结果，我会奖励你的。去吧！',
  `LogDescription` = '从铁炉堡外面的天灾士兵身上收集3块暗淡的死灵石，并调查天灾营地附近的发光符文法阵。',
  `AreaDescription` = '去找铁炉堡大门的奈维尔中尉。',
  `ObjectiveText1` = '调查法阵'
WHERE `ID` = 9261;

DELETE FROM `quest_template_locale` WHERE `ID` = 9261 AND `locale` = 'zhCN';
INSERT INTO `quest_template_locale` (`ID`,`locale`,`Title`,`Details`,`Objectives`,`EndText`,`CompletedText`,`ObjectiveText1`,`ObjectiveText2`,`ObjectiveText3`,`ObjectiveText4`,`VerifiedBuild`) VALUES
(9261,'zhCN','调查铁炉堡的天灾入侵','巫妖王的部队已经在我们鼻子底下扎了营，我们可不能坐以待毙！你会拿起武器跟这些可憎的入侵者作战，保卫铁炉堡的吧？$B$B城外出现了奇怪的符文法阵，它散发着神秘的能量，亡灵和悬浮在半空的浮空城也被同样的能量环绕着。我想这个符文法阵对天灾士兵们必然有着重要的意义，因此想让你调查此事。去和城外的亡灵作战吧，带回你杀死他们的证据，以及你的调查结果，我会奖励你的。去吧！','从铁炉堡外面的天灾士兵身上收集3块暗淡的死灵石，并调查天灾营地附近的发光符文法阵。','去找铁炉堡大门的奈维尔中尉。','','调查法阵','','','',0);

UPDATE `quest_template` SET
  `LogTitle` = '调查达纳苏斯的天灾入侵',
  `QuestDescription` = '巫妖王的部队已经在我们鼻子底下扎了营，我们可不能坐以待毙！你会拿起武器跟这些可憎的入侵者作战，保卫达纳苏斯的吧？$B$B城外出现了奇怪的符文法阵，它散发着神秘的能量，亡灵和悬浮在半空的浮空城也被同样的能量环绕着。我想这个符文法阵对天灾士兵们必然有着重要的意义，因此想让你调查此事。去和城外的亡灵作战吧，带回你杀死他们的证据，以及你的调查结果，我会奖励你的。去吧！',
  `LogDescription` = '从达纳苏斯外面的天灾士兵身上收集3块暗淡的死灵石，并调查天灾营地附近的发光符文法阵。',
  `AreaDescription` = '去泰达希尔找阿里斯瑞恩之池的贝萨中尉。',
  `ObjectiveText1` = '调查法阵'
WHERE `ID` = 9262;

DELETE FROM `quest_template_locale` WHERE `ID` = 9262 AND `locale` = 'zhCN';
INSERT INTO `quest_template_locale` (`ID`,`locale`,`Title`,`Details`,`Objectives`,`EndText`,`CompletedText`,`ObjectiveText1`,`ObjectiveText2`,`ObjectiveText3`,`ObjectiveText4`,`VerifiedBuild`) VALUES
(9262,'zhCN','调查达纳苏斯的天灾入侵','巫妖王的部队已经在我们鼻子底下扎了营，我们可不能坐以待毙！你会拿起武器跟这些可憎的入侵者作战，保卫达纳苏斯的吧？$B$B城外出现了奇怪的符文法阵，它散发着神秘的能量，亡灵和悬浮在半空的浮空城也被同样的能量环绕着。我想这个符文法阵对天灾士兵们必然有着重要的意义，因此想让你调查此事。去和城外的亡灵作战吧，带回你杀死他们的证据，以及你的调查结果，我会奖励你的。去吧！','从达纳苏斯外面的天灾士兵身上收集3块暗淡的死灵石，并调查天灾营地附近的发光符文法阵。','去泰达希尔找阿里斯瑞恩之池的贝萨中尉。','','调查法阵','','','',0);

UPDATE `quest_template` SET
  `LogTitle` = '调查奥格瑞玛的天灾入侵',
  `QuestDescription` = '天灾士兵们在主城外扎了营，我们不能坐以待毙！你会拿起武器跟这些可憎的入侵者作战，保卫奥格瑞玛的吧？$B$B城外出现了奇怪的符文法阵，它散发着神秘的能量，亡灵和悬浮在半空的浮空城也被同样的能量环绕着。我想这个符文法阵对天灾士兵们必然有着重要的意义，因此想让你调查此事。去和城外的亡灵作战吧，带回你杀死他们的证据，以及你的调查结果，我会奖励你的。去吧！',
  `LogDescription` = '从奥格瑞玛外面的天灾士兵身上收集3块暗淡的死灵石，并调查营地附近的发光的符文法阵。',
  `AreaDescription` = '去找奥格瑞玛的达格尔中尉。',
  `ObjectiveText1` = '调查法阵'
WHERE `ID` = 9263;

DELETE FROM `quest_template_locale` WHERE `ID` = 9263 AND `locale` = 'zhCN';
INSERT INTO `quest_template_locale` (`ID`,`locale`,`Title`,`Details`,`Objectives`,`EndText`,`CompletedText`,`ObjectiveText1`,`ObjectiveText2`,`ObjectiveText3`,`ObjectiveText4`,`VerifiedBuild`) VALUES
(9263,'zhCN','调查奥格瑞玛的天灾入侵','天灾士兵们在主城外扎了营，我们不能坐以待毙！你会拿起武器跟这些可憎的入侵者作战，保卫奥格瑞玛的吧？$B$B城外出现了奇怪的符文法阵，它散发着神秘的能量，亡灵和悬浮在半空的浮空城也被同样的能量环绕着。我想这个符文法阵对天灾士兵们必然有着重要的意义，因此想让你调查此事。去和城外的亡灵作战吧，带回你杀死他们的证据，以及你的调查结果，我会奖励你的。去吧！','从奥格瑞玛外面的天灾士兵身上收集3块暗淡的死灵石，并调查营地附近的发光的符文法阵。','去找奥格瑞玛的达格尔中尉。','','调查法阵','','','',0);

UPDATE `quest_template` SET
  `LogTitle` = '调查雷霆崖的天灾入侵',
  `QuestDescription` = '天灾士兵们在主城外扎了营，我们不能坐以待毙！你会拿起武器跟这些可憎的入侵者作战，保卫雷霆崖的吧？$B$B城外出现了奇怪的符文法阵，它散发着神秘的能量，亡灵和悬浮在半空的浮空城也被同样的能量环绕着。我想这个符文法阵对天灾士兵们必然有着重要的意义，因此想让你调查此事。去和城外的亡灵作战吧，带回你杀死他们的证据，以及你的调查结果，我会奖励你的。去吧！',
  `LogDescription` = '从雷霆崖外面的天灾士兵身上收集3块暗淡的死灵石，并调查营地附近的发光的符文法阵。',
  `AreaDescription` = '去找莫高雷的利山德中尉。',
  `ObjectiveText1` = '调查法阵'
WHERE `ID` = 9264;

DELETE FROM `quest_template_locale` WHERE `ID` = 9264 AND `locale` = 'zhCN';
INSERT INTO `quest_template_locale` (`ID`,`locale`,`Title`,`Details`,`Objectives`,`EndText`,`CompletedText`,`ObjectiveText1`,`ObjectiveText2`,`ObjectiveText3`,`ObjectiveText4`,`VerifiedBuild`) VALUES
(9264,'zhCN','调查雷霆崖的天灾入侵','天灾士兵们在主城外扎了营，我们不能坐以待毙！你会拿起武器跟这些可憎的入侵者作战，保卫雷霆崖的吧？$B$B城外出现了奇怪的符文法阵，它散发着神秘的能量，亡灵和悬浮在半空的浮空城也被同样的能量环绕着。我想这个符文法阵对天灾士兵们必然有着重要的意义，因此想让你调查此事。去和城外的亡灵作战吧，带回你杀死他们的证据，以及你的调查结果，我会奖励你的。去吧！','从雷霆崖外面的天灾士兵身上收集3块暗淡的死灵石，并调查营地附近的发光的符文法阵。','去找莫高雷的利山德中尉。','','调查法阵','','','',0);

UPDATE `quest_template` SET
  `LogTitle` = '破碎的死灵水晶',
  `QuestDescription` = '你在憎恶的尸体上找到一块散发着腐坏能量的破碎水晶。',
  `LogDescription` = '将破碎的死灵水晶交给暴风城门外的奥林中尉。',
  `AreaDescription` = ''
WHERE `ID` = 9292;

DELETE FROM `quest_template_locale` WHERE `ID` = 9292 AND `locale` = 'zhCN';
INSERT INTO `quest_template_locale` (`ID`,`locale`,`Title`,`Details`,`Objectives`,`EndText`,`CompletedText`,`ObjectiveText1`,`ObjectiveText2`,`ObjectiveText3`,`ObjectiveText4`,`VerifiedBuild`) VALUES
(9292,'zhCN','破碎的死灵水晶','你在憎恶的尸体上找到一块散发着腐坏能量的破碎水晶。','将破碎的死灵水晶交给暴风城门外的奥林中尉。','','','','','','',0);

UPDATE `quest_template` SET
  `LogTitle` = '被撕碎的信',
  `QuestDescription` = '你在某个天灾士兵的尸体上找到了这封信。破旧发黄的信纸散发着霉臭，看来它在亡灵手里已经有些日子了。或许圣光之愿礼拜堂的家伙会对此感兴趣的……',
  `LogDescription` = '将被撕碎的信交给东瘟疫之地圣光之愿礼拜堂的名单登记员。',
  `AreaDescription` = ''
WHERE `ID` = 9295;

DELETE FROM `quest_template_locale` WHERE `ID` = 9295 AND `locale` = 'zhCN';
INSERT INTO `quest_template_locale` (`ID`,`locale`,`Title`,`Details`,`Objectives`,`EndText`,`CompletedText`,`ObjectiveText1`,`ObjectiveText2`,`ObjectiveText3`,`ObjectiveText4`,`VerifiedBuild`) VALUES
(9295,'zhCN','被撕碎的信','你在某个天灾士兵的尸体上找到了这封信。破旧发黄的信纸散发着霉臭，看来它在亡灵手里已经有些日子了。或许圣光之愿礼拜堂的家伙会对此感兴趣的……','将被撕碎的信交给东瘟疫之地圣光之愿礼拜堂的名单登记员。','','','','','','',0);

UPDATE `quest_template` SET
  `LogTitle` = '前线的便条',
  `QuestDescription` = '你在杀掉的众多天谴军团爪牙其中一人身上找到这封信。从它的样子和气味判断，它已经和不死族在一起一些日子了。或许圣光之愿礼拜堂的某人会对它有兴趣进而好好的研究它...',
  `LogDescription` = '将干净的便条带到东瘟疫之地的圣光之愿礼拜堂交给名册保管者。',
  `AreaDescription` = ''
WHERE `ID` = 9299;

DELETE FROM `quest_template_locale` WHERE `ID` = 9299 AND `locale` = 'zhCN';
INSERT INTO `quest_template_locale` (`ID`,`locale`,`Title`,`Details`,`Objectives`,`EndText`,`CompletedText`,`ObjectiveText1`,`ObjectiveText2`,`ObjectiveText3`,`ObjectiveText4`,`VerifiedBuild`) VALUES
(9299,'zhCN','前线的便条','你在杀掉的众多天谴军团爪牙其中一人身上找到这封信。从它的样子和气味判断，它已经和不死族在一起一些日子了。或许圣光之愿礼拜堂的某人会对它有兴趣进而好好的研究它...','将干净的便条带到东瘟疫之地的圣光之愿礼拜堂交给名册保管者。','','','','','','',0);

UPDATE `quest_template` SET
  `LogTitle` = '破旧的信',
  `QuestDescription` = '你在某个天灾士兵的尸体上找到了这封信。破旧发黄的信纸散发着霉臭，看来它在亡灵手里已经有些日子了。或许圣光之愿礼拜堂的家伙会对此感兴趣的……',
  `LogDescription` = '将破旧的信交给东瘟疫之地圣光之愿礼拜堂的名单登记员。',
  `AreaDescription` = ''
WHERE `ID` = 9300;

DELETE FROM `quest_template_locale` WHERE `ID` = 9300 AND `locale` = 'zhCN';
INSERT INTO `quest_template_locale` (`ID`,`locale`,`Title`,`Details`,`Objectives`,`EndText`,`CompletedText`,`ObjectiveText1`,`ObjectiveText2`,`ObjectiveText3`,`ObjectiveText4`,`VerifiedBuild`) VALUES
(9300,'zhCN','破旧的信','你在某个天灾士兵的尸体上找到了这封信。破旧发黄的信纸散发着霉臭，看来它在亡灵手里已经有些日子了。或许圣光之愿礼拜堂的家伙会对此感兴趣的……','将破旧的信交给东瘟疫之地圣光之愿礼拜堂的名单登记员。','','','','','','',0);

UPDATE `quest_template` SET
  `LogTitle` = '浸血的信封',
  `QuestDescription` = '你在某个天灾士兵的尸体上找到了这封信。破旧发黄的信纸散发着霉臭，看来它在亡灵手里已经有些日子了。或许圣光之愿礼拜堂的家伙会对此感兴趣的……',
  `LogDescription` = '将浸血的信封交给东瘟疫之地圣光之愿礼拜堂的名单登记员。',
  `AreaDescription` = ''
WHERE `ID` = 9301;

DELETE FROM `quest_template_locale` WHERE `ID` = 9301 AND `locale` = 'zhCN';
INSERT INTO `quest_template_locale` (`ID`,`locale`,`Title`,`Details`,`Objectives`,`EndText`,`CompletedText`,`ObjectiveText1`,`ObjectiveText2`,`ObjectiveText3`,`ObjectiveText4`,`VerifiedBuild`) VALUES
(9301,'zhCN','浸血的信封','你在某个天灾士兵的尸体上找到了这封信。破旧发黄的信纸散发着霉臭，看来它在亡灵手里已经有些日子了。或许圣光之愿礼拜堂的家伙会对此感兴趣的……','将浸血的信封交给东瘟疫之地圣光之愿礼拜堂的名单登记员。','','','','','','',0);

UPDATE `quest_template` SET
  `LogTitle` = '被弄皱的信',
  `QuestDescription` = '你在某个天灾士兵的尸体上找到了这封信。破旧发黄的信纸散发着霉臭，看来它在亡灵手里已经有些日子了。或许圣光之愿礼拜堂的家伙会对此感兴趣的……',
  `LogDescription` = '将被弄皱的信交给东瘟疫之地圣光之愿礼拜堂的名单登记员。',
  `AreaDescription` = ''
WHERE `ID` = 9302;

DELETE FROM `quest_template_locale` WHERE `ID` = 9302 AND `locale` = 'zhCN';
INSERT INTO `quest_template_locale` (`ID`,`locale`,`Title`,`Details`,`Objectives`,`EndText`,`CompletedText`,`ObjectiveText1`,`ObjectiveText2`,`ObjectiveText3`,`ObjectiveText4`,`VerifiedBuild`) VALUES
(9302,'zhCN','被弄皱的信','你在某个天灾士兵的尸体上找到了这封信。破旧发黄的信纸散发着霉臭，看来它在亡灵手里已经有些日子了。或许圣光之愿礼拜堂的家伙会对此感兴趣的……','将被弄皱的信交给东瘟疫之地圣光之愿礼拜堂的名单登记员。','','','','','','',0);

UPDATE `quest_template` SET
  `LogTitle` = '前线的文件',
  `QuestDescription` = '你在杀掉的众多天谴军团爪牙其中一人身上找到这封信。从它的样子和气味判断，它已经和不死族在一起一些日子了。或许圣光之愿礼拜堂的某人会对它有兴趣进而好好的研究它……',
  `LogDescription` = '将弄脏的文件带到东瘟疫之地的圣光礼拜堂交给名册保管者。',
  `AreaDescription` = ''
WHERE `ID` = 9304;

DELETE FROM `quest_template_locale` WHERE `ID` = 9304 AND `locale` = 'zhCN';
INSERT INTO `quest_template_locale` (`ID`,`locale`,`Title`,`Details`,`Objectives`,`EndText`,`CompletedText`,`ObjectiveText1`,`ObjectiveText2`,`ObjectiveText3`,`ObjectiveText4`,`VerifiedBuild`) VALUES
(9304,'zhCN','前线的文件','你在杀掉的众多天谴军团爪牙其中一人身上找到这封信。从它的样子和气味判断，它已经和不死族在一起一些日子了。或许圣光之愿礼拜堂的某人会对它有兴趣进而好好的研究它……','将弄脏的文件带到东瘟疫之地的圣光礼拜堂交给名册保管者。','','','','','','',0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 9085;
INSERT INTO `quest_offer_reward` (`ID`,`Emote1`,`Emote2`,`Emote3`,`Emote4`,`EmoteDelay1`,`EmoteDelay2`,`EmoteDelay3`,`EmoteDelay4`,`RewardText`,`VerifiedBuild`) VALUES
(9085,0,0,0,0,0,0,0,0,'做得漂亮，$N。随着暗影被摧毁，我们对抗巫妖的胜利希望也得以保存。',0);
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 9085 AND `locale` = 'zhCN';
INSERT INTO `quest_offer_reward_locale` (`ID`,`locale`,`RewardText`,`VerifiedBuild`) VALUES
(9085,'zhCN','做得漂亮，$N。随着暗影被摧毁，我们对抗巫妖的胜利希望也得以保存。',0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 9153;
INSERT INTO `quest_offer_reward` (`ID`,`Emote1`,`Emote2`,`Emote3`,`Emote4`,`EmoteDelay1`,`EmoteDelay2`,`EmoteDelay3`,`EmoteDelay4`,`RewardText`,`VerifiedBuild`) VALUES
(9153,0,0,0,0,0,0,0,0,'你做的很好，$N。通过持续增加的获胜记录，我们将可以战胜巫妖王和他的手下。$B$B休息一下养精蓄锐，但你之后务必要再次回到前线，以免我们损失了今天才夺回的土地。$B$B为了在日后的战役中帮助你，我们会用特别的物品和你交易更多的亡域符文。',0);
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 9153 AND `locale` = 'zhCN';
INSERT INTO `quest_offer_reward_locale` (`ID`,`locale`,`RewardText`,`VerifiedBuild`) VALUES
(9153,'zhCN','你做的很好，$N。通过持续增加的获胜记录，我们将可以战胜巫妖王和他的手下。$B$B休息一下养精蓄锐，但你之后务必要再次回到前线，以免我们损失了今天才夺回的土地。$B$B为了在日后的战役中帮助你，我们会用特别的物品和你交易更多的亡域符文。',0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 9261;
INSERT INTO `quest_offer_reward` (`ID`,`Emote1`,`Emote2`,`Emote3`,`Emote4`,`EmoteDelay1`,`EmoteDelay2`,`EmoteDelay3`,`EmoteDelay4`,`RewardText`,`VerifiedBuild`) VALUES
(9261,0,0,0,0,0,0,0,0,'嗯。从你告诉我的事情，法阵里的声音……它们一定是被用来做为某种沟通的工具。我一想到那些邪恶的脑袋会通过这样的魔法联系就不寒而栗。无论如何，你看起来还是很健全很清醒。银色黎明感谢你的努力。',0);
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 9261 AND `locale` = 'zhCN';
INSERT INTO `quest_offer_reward_locale` (`ID`,`locale`,`RewardText`,`VerifiedBuild`) VALUES
(9261,'zhCN','嗯。从你告诉我的事情，法阵里的声音……它们一定是被用来做为某种沟通的工具。我一想到那些邪恶的脑袋会通过这样的魔法联系就不寒而栗。无论如何，你看起来还是很健全很清醒。银色黎明感谢你的努力。',0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 9262;
INSERT INTO `quest_offer_reward` (`ID`,`Emote1`,`Emote2`,`Emote3`,`Emote4`,`EmoteDelay1`,`EmoteDelay2`,`EmoteDelay3`,`EmoteDelay4`,`RewardText`,`VerifiedBuild`) VALUES
(9262,0,0,0,0,0,0,0,0,'嗯。从你告诉我的事情，法阵里的声音……它们一定是被用来做为某种沟通的工具。我一想到那些邪恶的脑袋会通过这样的魔法联系就不寒而栗。无论如何，你看起来还是很健全很清醒。银色黎明感谢你的努力。',0);
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 9262 AND `locale` = 'zhCN';
INSERT INTO `quest_offer_reward_locale` (`ID`,`locale`,`RewardText`,`VerifiedBuild`) VALUES
(9262,'zhCN','嗯。从你告诉我的事情，法阵里的声音……它们一定是被用来做为某种沟通的工具。我一想到那些邪恶的脑袋会通过这样的魔法联系就不寒而栗。无论如何，你看起来还是很健全很清醒。银色黎明感谢你的努力。',0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 9263;
INSERT INTO `quest_offer_reward` (`ID`,`Emote1`,`Emote2`,`Emote3`,`Emote4`,`EmoteDelay1`,`EmoteDelay2`,`EmoteDelay3`,`EmoteDelay4`,`RewardText`,`VerifiedBuild`) VALUES
(9263,0,0,0,0,0,0,0,0,'嗯。从你告诉我的事情，法阵里的声音……它们一定是被用来做为某种沟通的工具。我一想到那些邪恶的脑袋会通过这样的魔法联系就不寒而栗。无论如何，你看起来还是很健全很清醒。银色黎明感谢你的努力。',0);
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 9263 AND `locale` = 'zhCN';
INSERT INTO `quest_offer_reward_locale` (`ID`,`locale`,`RewardText`,`VerifiedBuild`) VALUES
(9263,'zhCN','嗯。从你告诉我的事情，法阵里的声音……它们一定是被用来做为某种沟通的工具。我一想到那些邪恶的脑袋会通过这样的魔法联系就不寒而栗。无论如何，你看起来还是很健全很清醒。银色黎明感谢你的努力。',0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 9264;
INSERT INTO `quest_offer_reward` (`ID`,`Emote1`,`Emote2`,`Emote3`,`Emote4`,`EmoteDelay1`,`EmoteDelay2`,`EmoteDelay3`,`EmoteDelay4`,`RewardText`,`VerifiedBuild`) VALUES
(9264,0,0,0,0,0,0,0,0,'嗯。从你告诉我的$B$B事情，法阵里的声音……它们一定是被用来做为某种沟通的工具。我一想到那些邪恶的脑袋会通过这样的魔法联系就不寒而栗。无论如何，你看起来还是很健全很清醒。银色黎明感谢你的努力。',0);
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 9264 AND `locale` = 'zhCN';
INSERT INTO `quest_offer_reward_locale` (`ID`,`locale`,`RewardText`,`VerifiedBuild`) VALUES
(9264,'zhCN','嗯。从你告诉我的$B$B事情，法阵里的声音……它们一定是被用来做为某种沟通的工具。我一想到那些邪恶的脑袋会通过这样的魔法联系就不寒而栗。无论如何，你看起来还是很健全很清醒。银色黎明感谢你的努力。',0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 9292;
INSERT INTO `quest_offer_reward` (`ID`,`Emote1`,`Emote2`,`Emote3`,`Emote4`,`EmoteDelay1`,`EmoteDelay2`,`EmoteDelay3`,`EmoteDelay4`,`RewardText`,`VerifiedBuild`) VALUES
(9292,0,0,0,0,0,0,0,0,'嗯，你从一具受到憎恨侵袭的尸体身上取得这项物品？$B$B我们已听说天灾军团召唤亡域基础营地的事。我猜他们想在暴风城里也造一个。很好，这样他们就等着迎接失败吧。',0);
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 9292 AND `locale` = 'zhCN';
INSERT INTO `quest_offer_reward_locale` (`ID`,`locale`,`RewardText`,`VerifiedBuild`) VALUES
(9292,'zhCN','嗯，你从一具受到憎恨侵袭的尸体身上取得这项物品？$B$B我们已听说天灾军团召唤亡域基础营地的事。我猜他们想在暴风城里也造一个。很好，这样他们就等着迎接失败吧。',0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 9295;
INSERT INTO `quest_offer_reward` (`ID`,`Emote1`,`Emote2`,`Emote3`,`Emote4`,`EmoteDelay1`,`EmoteDelay2`,`EmoteDelay3`,`EmoteDelay4`,`RewardText`,`VerifiedBuild`) VALUES
(9295,0,0,0,0,0,0,0,0,'多悲哀啊。这些话语，清楚的是要传达给某个士兵深爱的人，只是它永远也到不了目的地了。里面写的事情是多年前发生的，我不确定原本的收件者是否还活着。$B$B不过，还是有希望。有他写的名字和讯息，我大概可以找到那户人家！需要一点时间调查，不过暴风城的图书馆员还欠我个人情……$B$B谢谢你把这个带来给我。有你的帮忙，我们终于能让某人的心灵得到平静。',0);
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 9295 AND `locale` = 'zhCN';
INSERT INTO `quest_offer_reward_locale` (`ID`,`locale`,`RewardText`,`VerifiedBuild`) VALUES
(9295,'zhCN','多悲哀啊。这些话语，清楚的是要传达给某个士兵深爱的人，只是它永远也到不了目的地了。里面写的事情是多年前发生的，我不确定原本的收件者是否还活着。$B$B不过，还是有希望。有他写的名字和讯息，我大概可以找到那户人家！需要一点时间调查，不过暴风城的图书馆员还欠我个人情……$B$B谢谢你把这个带来给我。有你的帮忙，我们终于能让某人的心灵得到平静。',0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 9299;
INSERT INTO `quest_offer_reward` (`ID`,`Emote1`,`Emote2`,`Emote3`,`Emote4`,`EmoteDelay1`,`EmoteDelay2`,`EmoteDelay3`,`EmoteDelay4`,`RewardText`,`VerifiedBuild`) VALUES
(9299,0,0,0,0,0,0,0,0,'多悲哀啊$B$B。这些话语，清楚的是要传达给某个士兵深爱的人，只是它永远也到不了目的地了。里面写的事情是多年前发生的，我不确定原本的收件者是否还活着。$B$B不过，还是有希望。有他写的名字和讯息，我大概可以找到那户人家！需要一点时间调查，不过暴风城的图书馆员还欠我个人情……$B$B谢谢你把这个带来给我。有你的帮忙，我们终于能让某人的心灵得到平静。',0);
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 9299 AND `locale` = 'zhCN';
INSERT INTO `quest_offer_reward_locale` (`ID`,`locale`,`RewardText`,`VerifiedBuild`) VALUES
(9299,'zhCN','多悲哀啊$B$B。这些话语，清楚的是要传达给某个士兵深爱的人，只是它永远也到不了目的地了。里面写的事情是多年前发生的，我不确定原本的收件者是否还活着。$B$B不过，还是有希望。有他写的名字和讯息，我大概可以找到那户人家！需要一点时间调查，不过暴风城的图书馆员还欠我个人情……$B$B谢谢你把这个带来给我。有你的帮忙，我们终于能让某人的心灵得到平静。',0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 9300;
INSERT INTO `quest_offer_reward` (`ID`,`Emote1`,`Emote2`,`Emote3`,`Emote4`,`EmoteDelay1`,`EmoteDelay2`,`EmoteDelay3`,`EmoteDelay4`,`RewardText`,`VerifiedBuild`) VALUES
(9300,0,0,0,0,0,0,0,0,'多悲哀啊。这些话语，清楚的是要传达给某个士兵深爱的人，只是它永远也到不了目的地了。里面写的事情是多年前发生的，我不确定原本的收件者是否还活着。$B$B不过，还是有希望。有他写的名字和讯息，我大概可以找到那户人家！需要一点时间调查，不过暴风城的图书馆员还欠我个人情……$B$B谢谢你把这个带来给我。有你的帮忙，我们终于能让某人的心灵得到平静。',0);
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 9300 AND `locale` = 'zhCN';
INSERT INTO `quest_offer_reward_locale` (`ID`,`locale`,`RewardText`,`VerifiedBuild`) VALUES
(9300,'zhCN','多悲哀啊。这些话语，清楚的是要传达给某个士兵深爱的人，只是它永远也到不了目的地了。里面写的事情是多年前发生的，我不确定原本的收件者是否还活着。$B$B不过，还是有希望。有他写的名字和讯息，我大概可以找到那户人家！需要一点时间调查，不过暴风城的图书馆员还欠我个人情……$B$B谢谢你把这个带来给我。有你的帮忙，我们终于能让某人的心灵得到平静。',0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 9301;
INSERT INTO `quest_offer_reward` (`ID`,`Emote1`,`Emote2`,`Emote3`,`Emote4`,`EmoteDelay1`,`EmoteDelay2`,`EmoteDelay3`,`EmoteDelay4`,`RewardText`,`VerifiedBuild`) VALUES
(9301,0,0,0,0,0,0,0,0,'多悲哀啊。这些话语，清楚的是要传达给某个士兵深爱的人，只是它永远也到不了目的地了。里面写的事情是多年前发生的，我不确定原本的收件者是否还活着。$B$B不过，还是有希望。有他写的名字和讯息，我大概可以找到那户人家！需要一点时间调查，不过暴风城的图书馆员还欠我个人情……$B$B谢谢你把这个带来给我。有你的帮忙，我们终于能让某人的心灵得到平静。',0);
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 9301 AND `locale` = 'zhCN';
INSERT INTO `quest_offer_reward_locale` (`ID`,`locale`,`RewardText`,`VerifiedBuild`) VALUES
(9301,'zhCN','多悲哀啊。这些话语，清楚的是要传达给某个士兵深爱的人，只是它永远也到不了目的地了。里面写的事情是多年前发生的，我不确定原本的收件者是否还活着。$B$B不过，还是有希望。有他写的名字和讯息，我大概可以找到那户人家！需要一点时间调查，不过暴风城的图书馆员还欠我个人情……$B$B谢谢你把这个带来给我。有你的帮忙，我们终于能让某人的心灵得到平静。',0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 9302;
INSERT INTO `quest_offer_reward` (`ID`,`Emote1`,`Emote2`,`Emote3`,`Emote4`,`EmoteDelay1`,`EmoteDelay2`,`EmoteDelay3`,`EmoteDelay4`,`RewardText`,`VerifiedBuild`) VALUES
(9302,0,0,0,0,0,0,0,0,'多悲哀啊。这些话语，清楚的是要传达给某个士兵深爱的人，只是它永远也到不了目的地了。里面写的事情是多年前发生的，我不确定原本的收件者是否还活着。$B$B不过，还是有希望。有他写的名字和讯息，我大概可以找到那户人家！需要一点时间调查，不过暴风城的图书馆员还欠我个人情……$B$B谢谢你把这个带来给我。有你的帮忙，我们终于能让某人的心灵得到平静。',0);
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 9302 AND `locale` = 'zhCN';
INSERT INTO `quest_offer_reward_locale` (`ID`,`locale`,`RewardText`,`VerifiedBuild`) VALUES
(9302,'zhCN','多悲哀啊。这些话语，清楚的是要传达给某个士兵深爱的人，只是它永远也到不了目的地了。里面写的事情是多年前发生的，我不确定原本的收件者是否还活着。$B$B不过，还是有希望。有他写的名字和讯息，我大概可以找到那户人家！需要一点时间调查，不过暴风城的图书馆员还欠我个人情……$B$B谢谢你把这个带来给我。有你的帮忙，我们终于能让某人的心灵得到平静。',0);

DELETE FROM `quest_offer_reward` WHERE `ID` = 9304;
INSERT INTO `quest_offer_reward` (`ID`,`Emote1`,`Emote2`,`Emote3`,`Emote4`,`EmoteDelay1`,`EmoteDelay2`,`EmoteDelay3`,`EmoteDelay4`,`RewardText`,`VerifiedBuild`) VALUES
(9304,0,0,0,0,0,0,0,0,'多悲哀啊。这些话语，清楚的是要传达给某个士兵深爱的人，只是它永远也到不了目的地了。里面写的事情是多年前发生的，我不确定原本的收件者是否还活着。$B$B不过，还是有希望。有他写的名字和讯息，我大概可以找到那户人家！需要一点时间调查，不过暴风城的图书馆员还欠我个人情……$B$B谢谢你把这个带来给我。有你的帮忙，我们终于能让某人的心灵得到平静。',0);
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 9304 AND `locale` = 'zhCN';
INSERT INTO `quest_offer_reward_locale` (`ID`,`locale`,`RewardText`,`VerifiedBuild`) VALUES
(9304,'zhCN','多悲哀啊。这些话语，清楚的是要传达给某个士兵深爱的人，只是它永远也到不了目的地了。里面写的事情是多年前发生的，我不确定原本的收件者是否还活着。$B$B不过，还是有希望。有他写的名字和讯息，我大概可以找到那户人家！需要一点时间调查，不过暴风城的图书馆员还欠我个人情……$B$B谢谢你把这个带来给我。有你的帮忙，我们终于能让某人的心灵得到平静。',0);

DELETE FROM `quest_request_items` WHERE `ID` = 9085;
INSERT INTO `quest_request_items` (`ID`,`EmoteOnComplete`,`EmoteOnIncomplete`,`CompletionText`,`VerifiedBuild`) VALUES
(9085,1,0,'运气如何，$N？',0);
DELETE FROM `quest_request_items_locale` WHERE `ID` = 9085 AND `locale` = 'zhCN';
INSERT INTO `quest_request_items_locale` (`ID`,`locale`,`CompletionText`,`VerifiedBuild`) VALUES
(9085,'zhCN','运气如何，$N？',0);

DELETE FROM `quest_request_items` WHERE `ID` = 9153;
INSERT INTO `quest_request_items` (`ID`,`EmoteOnComplete`,`EmoteOnIncomplete`,`CompletionText`,`VerifiedBuild`) VALUES
(9153,1,0,'要战胜这个威胁需要一些时间。你的进展如何，$N？',0);
DELETE FROM `quest_request_items_locale` WHERE `ID` = 9153 AND `locale` = 'zhCN';
INSERT INTO `quest_request_items_locale` (`ID`,`locale`,`CompletionText`,`VerifiedBuild`) VALUES
(9153,'zhCN','要战胜这个威胁需要一些时间。你的进展如何，$N？',0);

DELETE FROM `quest_request_items` WHERE `ID` = 9261;
INSERT INTO `quest_request_items` (`ID`,`EmoteOnComplete`,`EmoteOnIncomplete`,`CompletionText`,`VerifiedBuild`) VALUES
(9261,1,0,'你调查过入侵行动了吗？没有时间可以浪费了！',0);
DELETE FROM `quest_request_items_locale` WHERE `ID` = 9261 AND `locale` = 'zhCN';
INSERT INTO `quest_request_items_locale` (`ID`,`locale`,`CompletionText`,`VerifiedBuild`) VALUES
(9261,'zhCN','你调查过入侵行动了吗？没有时间可以浪费了！',0);

DELETE FROM `quest_request_items` WHERE `ID` = 9262;
INSERT INTO `quest_request_items` (`ID`,`EmoteOnComplete`,`EmoteOnIncomplete`,`CompletionText`,`VerifiedBuild`) VALUES
(9262,1,0,'你调查过入侵行动了吗？没有时间可以浪费了！',0);
DELETE FROM `quest_request_items_locale` WHERE `ID` = 9262 AND `locale` = 'zhCN';
INSERT INTO `quest_request_items_locale` (`ID`,`locale`,`CompletionText`,`VerifiedBuild`) VALUES
(9262,'zhCN','你调查过入侵行动了吗？没有时间可以浪费了！',0);

DELETE FROM `quest_request_items` WHERE `ID` = 9263;
INSERT INTO `quest_request_items` (`ID`,`EmoteOnComplete`,`EmoteOnIncomplete`,`CompletionText`,`VerifiedBuild`) VALUES
(9263,1,0,'你调查过入侵行动了吗？没有时间可以浪费了！',0);
DELETE FROM `quest_request_items_locale` WHERE `ID` = 9263 AND `locale` = 'zhCN';
INSERT INTO `quest_request_items_locale` (`ID`,`locale`,`CompletionText`,`VerifiedBuild`) VALUES
(9263,'zhCN','你调查过入侵行动了吗？没有时间可以浪费了！',0);

DELETE FROM `quest_request_items` WHERE `ID` = 9264;
INSERT INTO `quest_request_items` (`ID`,`EmoteOnComplete`,`EmoteOnIncomplete`,`CompletionText`,`VerifiedBuild`) VALUES
(9264,1,0,'你调查过入侵行动了吗？没有时间可以浪费了！',0);
DELETE FROM `quest_request_items_locale` WHERE `ID` = 9264 AND `locale` = 'zhCN';
INSERT INTO `quest_request_items_locale` (`ID`,`locale`,`CompletionText`,`VerifiedBuild`) VALUES
(9264,'zhCN','你调查过入侵行动了吗？没有时间可以浪费了！',0);

DELETE FROM `quest_request_items` WHERE `ID` = 9292;
INSERT INTO `quest_request_items` (`ID`,`EmoteOnComplete`,`EmoteOnIncomplete`,`CompletionText`,`VerifiedBuild`) VALUES
(9292,1,0,'你需要协助吗？',0);
DELETE FROM `quest_request_items_locale` WHERE `ID` = 9292 AND `locale` = 'zhCN';
INSERT INTO `quest_request_items_locale` (`ID`,`locale`,`CompletionText`,`VerifiedBuild`) VALUES
(9292,'zhCN','你需要协助吗？',0);

DELETE FROM `quest_request_items` WHERE `ID` = 9295;
INSERT INTO `quest_request_items` (`ID`,`EmoteOnComplete`,`EmoteOnIncomplete`,`CompletionText`,`VerifiedBuild`) VALUES
(9295,1,0,'什么事吗？你手里拿着的是什么？',0);
DELETE FROM `quest_request_items_locale` WHERE `ID` = 9295 AND `locale` = 'zhCN';
INSERT INTO `quest_request_items_locale` (`ID`,`locale`,`CompletionText`,`VerifiedBuild`) VALUES
(9295,'zhCN','什么事吗？你手里拿着的是什么？',0);

DELETE FROM `quest_request_items` WHERE `ID` = 9299;
INSERT INTO `quest_request_items` (`ID`,`EmoteOnComplete`,`EmoteOnIncomplete`,`CompletionText`,`VerifiedBuild`) VALUES
(9299,1,0,'什么事吗？你手里拿着的是什么？',0);
DELETE FROM `quest_request_items_locale` WHERE `ID` = 9299 AND `locale` = 'zhCN';
INSERT INTO `quest_request_items_locale` (`ID`,`locale`,`CompletionText`,`VerifiedBuild`) VALUES
(9299,'zhCN','什么事吗？你手里拿着的是什么？',0);

DELETE FROM `quest_request_items` WHERE `ID` = 9300;
INSERT INTO `quest_request_items` (`ID`,`EmoteOnComplete`,`EmoteOnIncomplete`,`CompletionText`,`VerifiedBuild`) VALUES
(9300,1,0,'什么事吗？你手里拿着的是什么？',0);
DELETE FROM `quest_request_items_locale` WHERE `ID` = 9300 AND `locale` = 'zhCN';
INSERT INTO `quest_request_items_locale` (`ID`,`locale`,`CompletionText`,`VerifiedBuild`) VALUES
(9300,'zhCN','什么事吗？你手里拿着的是什么？',0);

DELETE FROM `quest_request_items` WHERE `ID` = 9301;
INSERT INTO `quest_request_items` (`ID`,`EmoteOnComplete`,`EmoteOnIncomplete`,`CompletionText`,`VerifiedBuild`) VALUES
(9301,1,0,'什么事吗？你手里拿着的是什么？',0);
DELETE FROM `quest_request_items_locale` WHERE `ID` = 9301 AND `locale` = 'zhCN';
INSERT INTO `quest_request_items_locale` (`ID`,`locale`,`CompletionText`,`VerifiedBuild`) VALUES
(9301,'zhCN','什么事吗？你手里拿着的是什么？',0);

DELETE FROM `quest_request_items` WHERE `ID` = 9302;
INSERT INTO `quest_request_items` (`ID`,`EmoteOnComplete`,`EmoteOnIncomplete`,`CompletionText`,`VerifiedBuild`) VALUES
(9302,1,0,'什么事吗？你手里拿着的是什么？',0);
DELETE FROM `quest_request_items_locale` WHERE `ID` = 9302 AND `locale` = 'zhCN';
INSERT INTO `quest_request_items_locale` (`ID`,`locale`,`CompletionText`,`VerifiedBuild`) VALUES
(9302,'zhCN','什么事吗？你手里拿着的是什么？',0);

DELETE FROM `quest_request_items` WHERE `ID` = 9304;
INSERT INTO `quest_request_items` (`ID`,`EmoteOnComplete`,`EmoteOnIncomplete`,`CompletionText`,`VerifiedBuild`) VALUES
(9304,1,0,'什么事吗？你手里拿着的是什么？',0);
DELETE FROM `quest_request_items_locale` WHERE `ID` = 9304 AND `locale` = 'zhCN';
INSERT INTO `quest_request_items_locale` (`ID`,`locale`,`CompletionText`,`VerifiedBuild`) VALUES
(9304,'zhCN','什么事吗？你手里拿着的是什么？',0);

UPDATE `quest_greeting` SET `Greeting` = '巫妖王从冰封的北方向我们发动了战争，唯有银色黎明挡在他的面前。' WHERE `ID` = 16361 AND `Type` = 0;
DELETE FROM `quest_greeting_locale` WHERE `ID` = 16361 AND `Type` = 0 AND `locale` = 'zhCN';
INSERT INTO `quest_greeting_locale` (`ID`,`Type`,`locale`,`Greeting`,`VerifiedBuild`) VALUES
(16361,0,'zhCN','巫妖王从冰封的北方向我们发动了战争，唯有银色黎明挡在他的面前。',0);

UPDATE `creature_text` SET `BroadcastTextId` = 0 WHERE `CreatureID` IN (16143, 10181) AND `GroupID` IN (0, 3);
