-- Global scourge patrol (event #17) had length=1 minute and was expiring while invasion was still active.
UPDATE `game_event` SET `length` = 2592000 WHERE `eventEntry` = 17;
