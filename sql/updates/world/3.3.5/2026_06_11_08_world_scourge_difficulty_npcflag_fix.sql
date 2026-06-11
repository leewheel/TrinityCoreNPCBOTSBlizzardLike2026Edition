-- Sync difficulty-1 creature templates with scourge argent vendor NPCs (startup validation).
-- 16363 Grobbulus Cloud -> 29379, 16385 Lightning Totem -> 29360

UPDATE `creature_template` SET `npcflag` = `npcflag` | 129, `gossip_menu_id` = 7230 WHERE `entry` = 29379;
UPDATE `creature_template` SET `gossip_menu_id` = 7165 WHERE `entry` = 29360 AND `npcflag` & 129;
