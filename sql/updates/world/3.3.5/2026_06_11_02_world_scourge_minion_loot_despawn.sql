-- Scourge invasion camp minions: ensure loot works with bots/pets and corpses stay lootable.

-- No 50% player damage requirement for kill credit/loot on dynamically spawned camp minions.
UPDATE `creature_template` SET `flags_extra` = `flags_extra` | 0x200000
WHERE `entry` IN (16141, 16298, 16299, 14697, 16379, 16380, 16383);

-- 17680 (Spirit Spawn-out) forced despawn after 3s removed corpses before players could loot.
DELETE FROM `smart_scripts`
WHERE `entryorguid` IN (16141, 16298, 16299, 14697, 16379, 16380, 16383)
  AND `source_type` = 0
  AND `event_type` = 8
  AND `event_param1` = 17680
  AND `action_type` = 41;
