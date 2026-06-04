-- 移植自 Acore: ICC 霜翼大厅事件系统 + Blood Quickening 触发器修正
-- 
-- 区域触发器说明：
-- 5616-5618 → 血色厅堂入口，触发 Crok Scourgebane 的 Blood Quickening 事件
-- 5623       → 霜翼大厅入口，触发蜘蛛通道 Gauntlet 事件
-- 5628-5631  → 辛达苟萨房间入口，触发冰霜巨龙飞入动画

-- 1. 注册 Gauntlet Controller (霜翼大厅蜘蛛通道事件控制器)
UPDATE `world`.`creature_template` SET `ScriptName` = 'npc_icc_gauntlet_controller' WHERE `entry` = 22515;

-- 2. 注册 Area Triggers
REPLACE INTO `world`.`areatrigger_scripts` (`entry`, `ScriptName`) VALUES
-- Blood Quickening 入口（触发 Crok 事件）
(5616, 'at_icc_start_frostwing_gauntlet'),
(5617, 'at_icc_start_frostwing_gauntlet'),
(5618, 'at_icc_start_frostwing_gauntlet'),
-- 霜翼大厅 Gauntlet（蜘蛛通道）
(5623, 'at_icc_gauntlet_event'),
-- 辛达苟萨飞入动画
(5628, 'at_icc_spire_frostwyrm'),
(5629, 'at_icc_spire_frostwyrm'),
(5630, 'at_icc_spire_frostwyrm'),
(5631, 'at_icc_spire_frostwyrm');
