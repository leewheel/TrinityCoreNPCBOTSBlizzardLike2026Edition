-- 修复 Defias Thug (guid 80149) waypoint_scripts 导致 source object is NULL
-- 
-- 完整链路分析：
--   creature guid=80149, entry=38 (Defias Thug) → path_id=6411920
--   waypoint_data id=6411920, point=10 → action=8014900
--   waypoint_scripts id=8014900:
--     命令1（delay=1）: SCRIPT_COMMAND_MOVEMENT RANDOM（datalong=1, 距离5码）
--     命令2（delay=20）: SCRIPT_COMMAND_MOVEMENT WAYPOINT（datalong=2, 路径6411920）
-- 
-- 根因：第10点触发脚本 → 立即随机走动 → 20秒后切回路径。
-- 但这只3级怪（Defias Thug）在野外随机走动时可能被击杀/消失，
-- 20秒后脚本执行时 Creature GUID 无法解析，报 "source object is NULL"。
-- 
-- 修复：移除 RANDOM 命令，只保留 WAYPOINT 命令，始终保持路径巡逻，不走远就不会消失。

DELETE FROM `waypoint_scripts` WHERE `id` = 8014900;
INSERT INTO `waypoint_scripts` (`id`, `delay`, `command`, `datalong`, `datalong2`, `dataint`, `x`, `y`, `z`, `o`, `guid`) VALUES
(8014900, 1, 35, 2, 0, 6411920, 0, 0, 0, 0, 921);
