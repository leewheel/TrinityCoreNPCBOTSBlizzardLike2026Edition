-- Scourge Invasion hotfix: TC-incompatible unit_flags and SmartAI actions

-- 262400 includes UNIT_FLAG_STUNNED (0x40000) which TrinityCore strips from creature_template.
-- Keep only the allowed IMMUNE_TO_PC bit (256).
UPDATE `creature_template` SET `unit_flags` = 256 WHERE `entry` IN (16431, 16531);

-- Shadow of Doom: TC SmartAI does not support action 18/19 (set/remove unit flags).
-- Behaviour moved to npc_shadow_of_doom C++ script.
DELETE FROM `smart_scripts` WHERE `entryorguid` = 16143 AND `source_type` = 0;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_shadow_of_doom' WHERE `entry` = 16143;
