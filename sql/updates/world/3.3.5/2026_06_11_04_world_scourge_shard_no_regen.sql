-- Necrotic Shard must not passively regenerate; otherwise 15-dmg minion zaps are instantly undone.
UPDATE `creature_template` SET `RegenHealth` = 0 WHERE `entry` IN (16136, 16172);

-- Zap Crystal also applies to Damaged Necrotic Shard after the camp crystal transforms.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` = 28032 AND `SourceGroup` = 1 AND `ElseGroup` = 1;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(13, 1, 28032, 0, 1, 31, 0, 3, 16172, 0, 0, 0, 0, '', 'SI - Zap Crystal targets Damaged Necrotic Shard');
