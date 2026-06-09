-- 修复 Argent Dawn NPC 重叠问题
-- 根因：06_data_full.sql 重复导入了里程碑NPC（事件127-130的NPC），
-- 且绑定到了事件17，导致每处4个NPC重叠显示。
-- 
-- Acore原始设计：
--   事件127（50次入侵完成）→ Paladin + Initiate
--   事件128（100次）→ Crusader + Cleric
--   事件129（150次）→ Champion + Priest
--   事件130 → Quartermaster + Outfitter
--
-- 修复：删除所有重复刷怪点，恢复正确的里程碑事件绑定

-- 删除重复刷怪（06_data_full.sql导入的 GID 248666-251252 范围）
DELETE FROM `game_event_creature` WHERE `guid` >= 248666 AND `guid` <= 251252;
DELETE FROM `creature` WHERE `guid` >= 248666 AND `guid` <= 251252;

-- 恢复原Acore GUID的正确里程碑绑定
DELETE FROM `game_event_creature` WHERE `guid` BETWEEN 153322 AND 153342;

-- 127: 50 invasions — Argent Dawn Paladin (4) + Initiate (3)
INSERT INTO `game_event_creature` (`eventEntry`, `guid`) VALUES
(127, 153322), (127, 153323), (127, 153324), (127, 153325),
(127, 153326), (127, 153327), (127, 153328);

-- 128: 100 invasions — Argent Dawn Crusader (4) + Cleric (3)
INSERT INTO `game_event_creature` (`eventEntry`, `guid`) VALUES
(128, 153329), (128, 153330), (128, 153331), (128, 153332),
(128, 153333), (128, 153334), (128, 153335);

-- 129: 150 invasions — Argent Dawn Champion (4) + Priest (3)
INSERT INTO `game_event_creature` (`eventEntry`, `guid`) VALUES
(129, 153336), (129, 153337), (129, 153338), (129, 153339),
(129, 153340), (129, 153341), (129, 153342);
