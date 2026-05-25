-- Quest 900 Samophlange: Fuel Control Valve (61936) still not clickable on some clients
-- Root cause: display 353 sits inside barrel mesh; lockId 93 was fixed earlier but hitbox remains poor.
-- Align with working Regulator Valve (61935): display 755, larger interact, nudged position.

UPDATE `gameobject_template`
SET `displayId`=755, `size`=2, `Data0`=43, `Data1`=900, `Data13`=1, `AIName`='SmartGameObjectAI'
WHERE `entry`=61936;

UPDATE `gameobject_template_addon` SET `flags`=0 WHERE `entry`=61936;

-- Fresh spawn state + pull valve slightly out of the barrel for LOS / click
UPDATE `gameobject`
SET `state`=0, `position_x`=841.75, `position_y`=-2686.35, `position_z`=93.65
WHERE `guid`=15731 AND `id`=61936;
