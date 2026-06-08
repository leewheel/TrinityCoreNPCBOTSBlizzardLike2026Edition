-- 注册 npc_taxi 和 transport_zeppelins 脚本到数据库

UPDATE `creature_template` SET `ScriptName` = 'npc_taxi' WHERE `entry` IN (
20903, 29154, 19409, 19401, 23413, 25059, 25236,
20162, 23415, 27575, 26443, 26949, 23704, 17209
);

UPDATE `gameobject_template` SET `ScriptName` = 'go_transport_the_iron_eagle' WHERE `entry` = 175080;
UPDATE `gameobject_template` SET `ScriptName` = 'go_transport_the_thundercaller' WHERE `entry` = 164871;
UPDATE `gameobject_template` SET `ScriptName` = 'go_transport_the_purple_princess' WHERE `entry` = 176495;
