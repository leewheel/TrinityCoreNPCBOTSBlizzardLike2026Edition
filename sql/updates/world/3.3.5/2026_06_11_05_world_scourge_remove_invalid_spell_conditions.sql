-- 28041 (Damage Crystal) is single-target; TYPE_SPELL_IMPLICIT_TARGET (13) requires
-- chain/area/cone implicit targets. Cultist death already casts it on the shard in script.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` = 28041;
