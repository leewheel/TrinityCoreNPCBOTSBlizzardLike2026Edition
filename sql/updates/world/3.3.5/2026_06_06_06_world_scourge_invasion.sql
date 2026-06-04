-- 天灾入侵系统注册
-- 
-- 1. 创建状态表
-- 2. 注册 ScriptName
-- 3. 创建缺失的 game_event 记录

-- 状态持久化表
CREATE TABLE IF NOT EXISTS `scourge_invasion_state` (
  `zoneId` int(10) unsigned NOT NULL,
  `attackTimer` int(10) unsigned NOT NULL DEFAULT 0,
  `remainingNecropoli` int(10) unsigned NOT NULL DEFAULT 0,
  `battlesWon` int(10) unsigned NOT NULL DEFAULT 0,
  `lastAttackZone` int(10) unsigned NOT NULL DEFAULT 0,
  `state` tinyint(3) unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`zoneId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
