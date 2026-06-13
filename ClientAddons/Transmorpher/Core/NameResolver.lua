local addon, ns = ...

-- ============================================================
-- LOCALIZED NAME RESOLVER
-- Uses game client APIs to get localized names.
-- No hardcoded translations — works with ANY client language.
-- Loads after all database files.
-- ============================================================

-- Get localized spell name (for mounts, pets)
local function GetLocalizedSpellName(spellID)
    local name = GetSpellInfo(spellID)
    if name and name ~= "" then return name end
    return nil
end

-- Resolve item names from client API
-- ItemsDB stores: { {itemID}, {"|cffCOLOREnglishName|r"} }
-- Replace with localized name preserving color code
function ns.ResolveLocalizedItemName(itemID, colorCodedEnglishName)
    local localizedName = GetItemInfo(itemID)
    if localizedName then
        -- Preserve original color code: |cffXXXXXX
        local colorCode = ""
        if colorCodedEnglishName then
            colorCode = colorCodedEnglishName:match("^(|c%x+)")
        end
        if colorCode then
            return colorCode .. localizedName .. "|r"
        end
        return localizedName
    end
    return colorCodedEnglishName or ("Item " .. itemID)
end

-- Process ItemsDB: replace English names with localized ones
local function LocalizeItemsDB()
    if not ns.items then return end
    for _, subclasses in pairs(ns.items) do
        for _, entries in pairs(subclasses) do
            for _, entry in ipairs(entries) do
                local itemID = entry[1] and entry[1][1]
                local names = entry[2]
                if itemID and itemID > 0 and names then
                    local localized = GetItemInfo(itemID)
                    if localized then
                        -- Preserve color code from original name
                        local colorCode = ""
                        if names[1] then
                            colorCode = names[1]:match("^(|c%x+)") or ""
                        end
                        names[1] = colorCode .. localized .. "|r"
                    end
                end
            end
        end
    end
end

-- Process ItemSetsDB: try to localize set names from item tooltip
local function LocalizeItemSetsDB()
    if not ns.itemSetsDB then return end
    -- Attempt to resolve via one of the set's items
    -- In 3.3.5a there's no direct GetItemSetInfo(setID) API,
    -- but GetItemInfo(itemID) returns itemSetID as 17th return value.
    -- We store the English name as-is but note it can be localized
    -- per-client by inspecting tooltips.
end

-- Apply localized names to mount/pet databases
local function LocalizeMountsPetsDB()
    if ns.mountsDB then
        for _, entry in ipairs(ns.mountsDB) do
            local name = GetLocalizedSpellName(entry[2])
            if name then entry[1] = name end
        end
    end
    if ns.petsDB then
        for _, entry in ipairs(ns.petsDB) do
            local name = GetLocalizedSpellName(entry[2])
            if name then entry[1] = name end
        end
    end
    if ns.combatPetsDB and ns.creatureDisplayDB then
        for _, entry in ipairs(ns.combatPetsDB) do
            if ns.creatureDisplayDB[entry[3]] then
                entry[1] = ns.creatureDisplayDB[entry[3]]
            end
        end
    end
end

-- Master function
local function Initialize()
    LocalizeMountsPetsDB()
end

-- Run after all database globals are populated
-- Use PLAYER_ENTERING_WORLD to ensure item cache is warm
local frame = CreateFrame("Frame")
frame:RegisterEvent("PLAYER_ENTERING_WORLD")
frame:SetScript("OnEvent", function()
    Initialize()
    frame:SetScript("OnEvent", nil)
end)
