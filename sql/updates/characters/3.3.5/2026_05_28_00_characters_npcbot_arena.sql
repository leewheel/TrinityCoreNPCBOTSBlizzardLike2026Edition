-- By leewheel 20260528 - NPCBot arena profile + fixed roster persistence (2v2/3v3/5v5).

CREATE TABLE IF NOT EXISTS `characters_npcbot_arena_profile` (
  `owner_guid` INT UNSIGNED NOT NULL,
  `bracket_type` TINYINT UNSIGNED NOT NULL COMMENT '2/3/5',
  `team_name` VARCHAR(24) NOT NULL DEFAULT '',
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`owner_guid`, `bracket_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `characters_npcbot_arena_roster` (
  `owner_guid` INT UNSIGNED NOT NULL,
  `bracket_type` TINYINT UNSIGNED NOT NULL COMMENT '2/3/5',
  `slot_index` TINYINT UNSIGNED NOT NULL COMMENT '0..3 (max for 5v5 bots=4)',
  `bot_entry` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'locked bot template entry',
  `bot_class` TINYINT UNSIGNED NOT NULL,
  `bot_spec` TINYINT UNSIGNED NOT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`owner_guid`, `bracket_type`, `slot_index`),
  KEY `idx_owner_bracket` (`owner_guid`, `bracket_type`),
  KEY `idx_bot_entry` (`bot_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- By leewheel 20260528 - upgrade existing schema to persistent bot-entry lock support.
SET @has_bot_entry := (
  SELECT COUNT(*) FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'characters_npcbot_arena_roster'
    AND COLUMN_NAME = 'bot_entry'
);
SET @ddl := IF(@has_bot_entry = 0,
  'ALTER TABLE `characters_npcbot_arena_roster` ADD COLUMN `bot_entry` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT ''locked bot template entry'' AFTER `slot_index`',
  'SELECT 1');
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

