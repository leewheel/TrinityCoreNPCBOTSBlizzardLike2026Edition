-- 清理天灾入侵初始化数据中的脏数据
-- 
-- 问题：scourge_invasion_state 表中 attackTimer 和 remainingNecropoli 字段
-- 可能因之前的溢出bug被写入了巨大数值，导致 .si 命令显示几年都不触发的倒计时。
-- 另外 remainingNecropoli 预填非零值会导致区域误显示"战斗中"。
--
-- 修复：所有区域状态归零，由 StartEvents() 统一设置初始计时器
-- （区域 5-10分钟，主城 10-20分钟）

UPDATE `scourge_invasion_state` SET `attackTimer` = 0, `remainingNecropoli` = 0, `lastAttackZone` = 0;
