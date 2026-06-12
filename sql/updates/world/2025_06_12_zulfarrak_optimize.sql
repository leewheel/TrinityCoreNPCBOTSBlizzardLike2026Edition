-- Zul'Farrak optimizations: Sezz'ziz from SmartAI → C++ Boss AI
-- Date: 2025-06-12

-- Assign C++ script and disable SmartAI for Shadowpriest Sezz'ziz (7275)
UPDATE `creature_template` SET `ScriptName`='boss_shadowpriest_sezziz', `AIName`='' WHERE `entry`=7275;

-- Remove now-obsolete SmartAI rows (replaced by C++ Boss AI)
DELETE FROM `smart_scripts` WHERE `entryorguid`=7275 AND `source_type`=0;
