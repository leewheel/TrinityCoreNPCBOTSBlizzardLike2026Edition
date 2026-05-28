-- By leewheel 20260528 - NPCBot channel-chat persona presets and runtime state.

CREATE TABLE IF NOT EXISTS `characters_npcbot_chat_persona` (
  `bot_entry` INT UNSIGNED NOT NULL,
  `persona_name` VARCHAR(64) NOT NULL DEFAULT '',
  `style_prompt` VARCHAR(512) NOT NULL DEFAULT '',
  `topic_bias` VARCHAR(128) NOT NULL DEFAULT '',
  `verbosity` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '0=short,1=normal,2=long',
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`bot_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `characters_npcbot_chat_runtime` (
  `bot_entry` INT UNSIGNED NOT NULL,
  `last_chat_at` TIMESTAMP NULL DEFAULT NULL,
  `last_channel` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '1=world,2=party,3=raid',
  `cooldown_until` INT UNSIGNED NOT NULL DEFAULT 0,
  `recent_hash` VARCHAR(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`bot_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
