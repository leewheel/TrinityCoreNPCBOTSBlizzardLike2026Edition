local addon, ns = ...

-- ============================================================
-- CHINESE NAME OVERRIDE LOADER
-- Applies Chinese names from locale-derived data to
-- the English display databases.
-- ============================================================

-- Creature display Chinese names are auto-applied in CreatureDisplayCN.lua
-- This file is reserved for future mount/pet CN tables.

-- Apply mount/pet/hunterpet Chinese names (when available)
function ns.ApplyChinesePetNames()
    if ns.mountsDB and ns.mountsCN then
        for _, entry in ipairs(ns.mountsDB) do
            local spellID = entry[2]
            if ns.mountsCN[spellID] then
                entry[1] = ns.mountsCN[spellID]
            end
        end
    end
    if ns.petsDB and ns.petsCN then
        for _, entry in ipairs(ns.petsDB) do
            local spellID = entry[2]
            if ns.petsCN[spellID] then
                entry[1] = ns.petsCN[spellID]
            end
        end
    end
    -- Combat pets: auto-translate from CreatureDisplayCN by displayID
    if ns.combatPetsDB and ns.creatureDisplayCN then
        for _, entry in ipairs(ns.combatPetsDB) do
            local displayID = entry[3]
            if ns.creatureDisplayCN[displayID] then
                entry[1] = ns.creatureDisplayCN[displayID]
            end
        end
    end
    -- Also apply combat pet family names to Chinese
    if ns.combatPetsDB and ns.combatPetFamilyCN then
        for _, entry in ipairs(ns.combatPetsDB) do
            local family = entry[2]
            if ns.combatPetFamilyCN[family] then
                entry[2] = ns.combatPetFamilyCN[family]
            end
        end
    end
end
