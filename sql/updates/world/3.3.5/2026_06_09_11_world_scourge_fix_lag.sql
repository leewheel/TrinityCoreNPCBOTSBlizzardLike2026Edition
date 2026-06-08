-- 修复天灾入侵严重卡顿问题
--
-- 根因：06_data_full.sql 从 Acore 导入事件121-126的刷怪时，
-- 包含了 npc_si_controller (16356) 的 1129 个 spawn。
-- 每个 si_controller 都 setActive(true) 保持网格激活，
-- 每10秒调用一次 sScourgeInvasionMgr->Update()，导致服务器卡死。
--
-- 修复：只保留一个 si_controller，其余全部删除。
-- si_controller 的正确位置是东瘟疫之地的单独一个（由 09_00_fix.sql 创建）

-- 删除事件121-126中所有16356的 GEC 绑定
DELETE FROM `game_event_creature` WHERE `guid` IN (SELECT `guid` FROM `creature` WHERE `id` = 16356) AND `eventEntry` BETWEEN 121 AND 126;

-- 删除多余16356（保留绑定到事件17的那个）
DELETE FROM `creature` WHERE `id` = 16356 AND `guid` NOT IN (SELECT `guid` FROM `game_event_creature` WHERE `eventEntry` = 17);
