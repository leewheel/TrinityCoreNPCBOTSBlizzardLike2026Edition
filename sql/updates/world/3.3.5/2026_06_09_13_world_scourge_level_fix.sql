-- 天灾入侵地面部队等级修正
-- Skeletal Soldier(16422) 和 Spectral Apparition(16423) 在 TC base 中为 6-7 级
-- 天灾入侵是 70 级内容，需要升级到 69-70

UPDATE `creature_template` SET `minlevel` = 69, `maxlevel` = 70 WHERE `entry` IN (16422, 16423);
