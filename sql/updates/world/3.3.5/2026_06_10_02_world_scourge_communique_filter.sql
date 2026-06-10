-- Prevent scourge communique/lightning spells from hitting nearby NPCs and bots.
DELETE FROM `spell_script_names` WHERE `spell_id` IN (28281, 28326, 28351, 28365, 28366, 28367, 28373, 28386);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(28281, 'spell_scourge_invasion_communique_filter'),
(28326, 'spell_scourge_invasion_communique_filter'),
(28351, 'spell_scourge_invasion_communique_filter'),
(28365, 'spell_scourge_invasion_communique_filter'),
(28366, 'spell_scourge_invasion_communique_filter'),
(28367, 'spell_scourge_invasion_communique_filter'),
(28373, 'spell_scourge_invasion_communique_filter'),
(28386, 'spell_scourge_invasion_communique_filter');
