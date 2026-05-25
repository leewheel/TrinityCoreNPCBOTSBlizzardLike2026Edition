-- Quest 900 "Samophlange" (什么什么平衡器) - Venture Co. research station valves
-- TDB incorrectly set GO_FLAG_INTERACT_COND (4) on the three valves; clients cannot click them.
-- Fuel Control Valve (61936) also had wrong lockId (93, copied from Control Console) and no SAI.

UPDATE `gameobject_template_addon` SET `flags`=0 WHERE `entry` IN (4072, 61935, 61936);

UPDATE `gameobject_template` SET `displayId`=755, `size`=2, `Data0`=43, `Data1`=900, `Data13`=1, `AIName`='SmartGameObjectAI' WHERE `entry`=61936;

UPDATE `gameobject` SET `state`=0, `position_x`=841.75, `position_y`=-2686.35, `position_z`=93.65 WHERE `guid`=15731 AND `id`=61936;

DELETE FROM `smart_scripts` WHERE `entryorguid`=61936 AND `source_type`=1;
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(61936,1,0,0,70,0,100,0,2,0,0,0,12,3285,1,180000,0,0,0,8,0,0,0,841.276,-2686.75,93.0556,4.39584,"Fuel Control Valve - On Gameobject State Changed - Summon Creature 'Venture Co. Peon'"),
(61936,1,1,0,70,0,100,0,2,0,0,0,1,0,0,0,0,0,0,19,3285,0,0,0,0,0,0,"Fuel Control Valve - On Gameobject State Changed - Say Line 0 (Venture Co. Peon)");

UPDATE `gameobject_template` SET `Data1`=900 WHERE `entry` IN (4072, 61935);
