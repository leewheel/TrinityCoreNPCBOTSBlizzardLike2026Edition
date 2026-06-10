-- Extend communique/zap crystal filter to all invasion bolt spells that can hit bystanders.
DELETE FROM `spell_script_names` WHERE `spell_id` IN (28032, 28041, 28056, 28364);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(28032, 'spell_scourge_invasion_communique_filter'),
(28041, 'spell_scourge_invasion_communique_filter'),
(28056, 'spell_scourge_invasion_communique_filter'),
(28364, 'spell_scourge_invasion_communique_filter');

-- Crystal zap conditions (minion death chain)
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` IN (28032, 28056, 28041) AND `SourceGroup` = 1;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorType`,`ErrorTextId`,`ScriptName`,`Comment`) VALUES
(13, 1, 28032, 0, 0, 31, 0, 3, 16136, 0, 0, 0, 0, '', 'SI - Zap Crystal targets Necrotic Shard'),
(13, 1, 28056, 0, 0, 31, 0, 3, 16172, 0, 0, 0, 0, '', 'SI - Zap Crystal Corpse targets Damaged Necrotic Shard'),
(13, 1, 28056, 0, 1, 31, 0, 3, 16136, 0, 0, 0, 0, '', 'SI - Zap Crystal Corpse targets Necrotic Shard'),
(13, 1, 28041, 0, 0, 31, 0, 3, 16136, 0, 0, 0, 0, '', 'SI - Damage Crystal targets Necrotic Shard'),
(13, 1, 28041, 0, 1, 31, 0, 3, 16172, 0, 0, 0, 0, '', 'SI - Damage Crystal targets Damaged Necrotic Shard');
