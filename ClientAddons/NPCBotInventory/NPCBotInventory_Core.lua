-- NPCBotInventory_Core.lua - 共享数据与工具（规避 Lua 200 local 限制）
NPCBotInventory = NPCBotInventory or {}

-- [[ Bot Manager Pro - Authentic Paperdoll Symmetry & Server Sync ]] --
BMU_PREFIX = "BMU"
BMU_DEBUG = false
bmuDebugStats = { B = 0, G = 0, S = 0, E = 0, dropG = 0, bad = 0 }
playerName = nil
searchBox = nil

function GetBotListSearchFilter()
    if searchBox and searchBox.GetText then
        local text = searchBox:GetText()
        if text and text ~= "" then
            return string.lower(text)
        end
    end
    return ""
end

function BMU_Log(msg)
    if BMU_DEBUG then
        print("|cff66ccff[BMU调试]|r " .. tostring(msg))
    end
end

function BMU_ResetDebugStats()
    bmuDebugStats.B = 0
    bmuDebugStats.G = 0
    bmuDebugStats.S = 0
    bmuDebugStats.E = 0
    bmuDebugStats.dropG = 0
    bmuDebugStats.bad = 0
end

function BMU_CountBotsInDb()
    local n = 0
    if not db then return 0 end
    for _, info in pairs(db) do
        if type(info) == "table" and info.name then
            n = n + 1
        end
    end
    return n
end

-- 向服务端请求 REFRESH 前清空机器人列表，保留 minimap 等玩家设置
function BMU_ClearBotListDb()
    if not db then return end
    for entry, info in pairs(db) do
        if type(info) == "table" and info.name then
            db[entry] = nil
        end
    end
end

-- 兼容 arg1=BMU / 整段在 arg2 / 旧服 \tBMU\t 格式
function ExtractBMUPayload(arg1, arg2)
    if arg1 == BMU_PREFIX then
        return arg2 or ""
    end
    if type(arg2) == "string" and arg2 ~= "" then
        local body = arg2:match("^\t" .. BMU_PREFIX .. "\t(.+)$")
        if body then return body end
        body = arg2:match("^" .. BMU_PREFIX .. "\t(.+)$")
        if body then return body end
    end
    if type(arg1) == "string" and arg1:find("^" .. BMU_PREFIX) then
        return arg1:match("^" .. BMU_PREFIX .. "\t(.+)$") or arg1:match("^\t" .. BMU_PREFIX .. "\t(.+)$")
    end
    return nil
end
pendingAction = nil
currentTemplateContext = { itemID = nil, itemLink = nil, slotKey = nil }
pendingInspectEntry = nil
BOT_ENTRY_MIN = 70001

function SendBMUMessage(msg)
    if BMU_DEBUG then
        BMU_Log("发送 -> " .. msg)
    end
    SendAddonMessage(BMU_PREFIX, msg, "WHISPER", playerName)
end

db = nil
templatesDB = nil


-- Native StaticPopup dialog for creating new templates
StaticPopupDialogs["NPCBOT_CREATE_TEMPLATE"] = {
    text = "请输入新装备模板的名称：",
    button1 = "创建",
    button2 = "取消",
    hasEditBox = true,
    OnAccept = function(self)
        local templateName = self.editBox:GetText()
        if templateName and templateName ~= "" then
            templatesDB = GetSelectedTemplateBucket(true)
            local itemID = currentTemplateContext.itemID
            local itemLink = currentTemplateContext.itemLink
            local slotKey = currentTemplateContext.slotKey
            if templatesDB and itemID and slotKey then
                templatesDB[templateName] = templatesDB[templateName] or {}
                SetTemplateSlot(templatesDB[templateName], slotKey, itemID, itemLink)
                print("|cff00ff00机器人管理:|r 已创建模板「" .. templateName .. "」并添加物品。")
            else
                print("|cffff0000机器人管理:|r 错误：请先在列表中选中一名机器人，再创建职业/职责模板。")
            end
        end
    end,
    EditBoxOnEnterPressed = function(self)
        local parent = self:GetParent()
        local templateName = parent.editBox:GetText()
        if templateName and templateName ~= "" then
            templatesDB = GetSelectedTemplateBucket(true)
            local itemID = currentTemplateContext.itemID
            local itemLink = currentTemplateContext.itemLink
            local slotKey = currentTemplateContext.slotKey
            if templatesDB and itemID and slotKey then
                templatesDB[templateName] = templatesDB[templateName] or {}
                SetTemplateSlot(templatesDB[templateName], slotKey, itemID, itemLink)
                print("|cff00ff00机器人管理:|r 已创建模板「" .. templateName .. "」并添加物品。")
            else
                print("|cffff0000机器人管理:|r 错误：请先在列表中选中一名机器人，再创建职业/职责模板。")
            end
        end
        parent:Hide()
    end,
    timeout = 0,
    whileDead = true,
    hideOnEscape = true,
}

StaticPopupDialogs["NPCBOT_CREATE_TEMPLATE_FROM_GEAR"] = {
    text = "请输入模板名称（将保存当前机器人身上的装备）：",
    button1 = "创建",
    button2 = "取消",
    hasEditBox = true,
    OnAccept = function(self)
        local templateName = self.editBox:GetText()
        if templateName and templateName ~= "" then
            local botData = db[MainFrame.selectedBot]
            templatesDB = GetTemplateBucketForBot(botData, true)
            if not templatesDB then
                print("|cffff0000机器人管理:|r 请先在列表中选中一名机器人，再保存模板。")
                return
            end
            templatesDB[templateName] = {}
            if botData and botData.gear then
                for slotKey, gearData in pairs(botData.gear) do
                    if gearData and gearData.id and gearData.id > 0 then
                        SetTemplateSlot(templatesDB[templateName], slotKey, gearData.id)
                    end
                end
                print("|cff00ff00机器人管理:|r 已根据「" .. (botData.name or "机器人") .. "」的装备创建模板「" .. templateName .. "」。")
            else
                print("|cffff0000机器人管理:|r 该机器人没有可保存的装备。")
            end
        end
    end,
    EditBoxOnEnterPressed = function(self)
        local parent = self:GetParent()
        local templateName = parent.editBox:GetText()
        if templateName and templateName ~= "" then
            local botData = db[MainFrame.selectedBot]
            templatesDB = GetTemplateBucketForBot(botData, true)
            if not templatesDB then
                print("|cffff0000机器人管理:|r 请先在列表中选中一名机器人，再保存模板。")
                parent:Hide()
                return
            end
            templatesDB[templateName] = {}
            if botData and botData.gear then
                for slotKey, gearData in pairs(botData.gear) do
                    if gearData and gearData.id and gearData.id > 0 then
                        SetTemplateSlot(templatesDB[templateName], slotKey, gearData.id)
                    end
                end
                print("|cff00ff00机器人管理:|r 已根据「" .. (botData.name or "机器人") .. "」的装备创建模板「" .. templateName .. "」。")
            else
                print("|cffff0000机器人管理:|r 该机器人没有可保存的装备。")
            end
        end
        parent:Hide()
    end,
    timeout = 0,
    whileDead = true,
    hideOnEscape = true,
}

-- Create standard context menu frame
menuFrame = CreateFrame("Frame", "NPCBotInventoryContextMenu", UIParent, "UIDropDownMenuTemplate")

currentSortMode = "CLASS"
SortModeLabels = {
    CLASS = "职业",
    NAME = "名字",
    ROLE = "职责",
    TALENT = "天赋",
}

-- Custom WotLK Class Colors (Including Custom Classes)
classColors = {
    ["Warrior"] = "C79C6E", ["Paladin"] = "F58CBA", ["Hunter"] = "ABD473",
    ["Rogue"] = "FFF569", ["Priest"] = "FFFFFF", ["Death Knight"] = "C41F3B",
    ["Shaman"] = "0070DE", ["Mage"] = "69CCF0", ["Warlock"] = "9482C9", ["Druid"] = "FF7D0A",
    ["Blademaster"] = "A10015", ["Sphynx"] = "29004A", ["Archmage"] = "028a99",
    ["Dreadlord"] = "534161", ["Spellbreaker"] = "CF3C1F", ["Dark Ranger"] = "3E255E",
    ["Necromancer"] = "9900CC", ["Sea Witch"] = "40D7A9", ["Crypt Lord"] = "19782B"
}

ClassNameNormalizer = {
    ["Spell Breaker"] = "Spellbreaker",
    ["Blade Master"] = "Blademaster",
    ["Obsidian Destroyer"] = "Sphynx",
}

function SplitBMUPayload(payload)
    local parts = {}
    if not payload or payload == "" then
        return parts
    end
    local from = 1
    while true do
        local sep = payload:find(";", from, true)
        if not sep then
            table.insert(parts, payload:sub(from))
            break
        end
        table.insert(parts, payload:sub(from, sep - 1))
        from = sep + 1
    end
    return parts
end

function NormalizeClassName(className)
    if not className or className == "" then
        return "Bot"
    end
    return ClassNameNormalizer[className] or className
end

local TEMPLATE_SCHEMA_VERSION = 3
local TEMPLATE_SLOT_KEYS = {"HEAD", "NECK", "SHOULDER", "BACK", "CHEST", "WRIST", "HANDS", "WAIST", "LEGS", "FEET", "FINGER1", "FINGER2", "TRINKET1", "TRINKET2", "MAINHAND", "OFFHAND", "RANGED"}
local TEMPLATE_DEFAULT_ROLE_KEY = "GENERAL"

local function NormalizeTemplateClassName(className)
    if not className or className == "" then return nil end
    return ClassNameNormalizer[className] or className
end

function EnsureTemplateDatabase()
    BotInventoryDB = BotInventoryDB or {}
    if BotInventoryDB.schemaVersion ~= TEMPLATE_SCHEMA_VERSION then
        BotInventoryDB.schemaVersion = TEMPLATE_SCHEMA_VERSION
        BotInventoryDB.classRoleTemplates = {}
        BotInventoryDB.classTemplates = nil
        BotInventoryDB.templates = nil
    else
        BotInventoryDB.classRoleTemplates = BotInventoryDB.classRoleTemplates or {}
        BotInventoryDB.classTemplates = nil
        BotInventoryDB.templates = nil
    end
    return BotInventoryDB.classRoleTemplates
end

function GetTemplateRoleKeyForBot(botData)
    if botData and GetBotRoleCategory then
        return GetBotRoleCategory(botData) or TEMPLATE_DEFAULT_ROLE_KEY
    end
    return TEMPLATE_DEFAULT_ROLE_KEY
end

GetTemplateBucketForBot = function(botData, create)
    local className = NormalizeTemplateClassName(botData and botData.className)
    if not className then return nil end
    local classRoleTemplates = EnsureTemplateDatabase()
    if create and not classRoleTemplates[className] then
        classRoleTemplates[className] = {}
    end
    local classTemplates = classRoleTemplates[className]
    if not classTemplates then return nil end

    local roleKey = GetTemplateRoleKeyForBot(botData)
    if create and not classTemplates[roleKey] then
        classTemplates[roleKey] = {}
    end
    return classTemplates[roleKey]
end

GetSelectedTemplateBucket = function(create)
    local botData = db and MainFrame and MainFrame.selectedBot and db[MainFrame.selectedBot]
    return GetTemplateBucketForBot(botData, create)
end

local function ParseItemLinkState(itemID, itemLink)
    local itemString = itemLink and string.match(itemLink, "item:([^|]+)")
    if not itemString then return nil end

    local parts = {}
    for part in string.gmatch(itemString .. ":", "(.-):") do
        table.insert(parts, tonumber(part) or 0)
    end

    return {
        id = parts[1] or itemID,
        enchant = parts[2] or 0,
        gems = { parts[3] or 0, parts[4] or 0, parts[5] or 0, parts[6] or 0 },
        suffix = parts[7] or 0,
        uniqueID = parts[8] or 0,
        linkLevel = parts[9] or 0,
        link = itemLink,
    }
end

MakeTemplateItemState = function(itemID, itemLink)
    local state = ParseItemLinkState(itemID, itemLink) or {
        id = itemID,
        enchant = 0,
        gems = { 0, 0, 0, 0 },
        suffix = 0,
        uniqueID = 0,
        linkLevel = 0,
        link = itemLink,
    }
    state.id = tonumber(state.id) or tonumber(itemID) or 0
    state.enchant = tonumber(state.enchant) or 0
    state.gems = state.gems or {}
    for i = 1, 4 do
        state.gems[i] = tonumber(state.gems[i]) or 0
    end
    state.suffix = tonumber(state.suffix) or 0
    state.uniqueID = tonumber(state.uniqueID) or 0
    state.linkLevel = tonumber(state.linkLevel) or 0
    return state
end

GetTemplateItemID = function(templateData, slotKey)
    local state = templateData and templateData[slotKey]
    if type(state) == "table" then
        return tonumber(state.id) or 0
    end
    return 0
end

SetTemplateSlot = function(templateData, slotKey, itemID, itemLink)
    itemID = tonumber(itemID)
    if type(templateData) ~= "table" or not slotKey or not itemID or itemID <= 0 then return end
    templateData[slotKey] = MakeTemplateItemState(itemID, itemLink)
end

-- WotLK item:entry:enchant:gem1:gem2:gem3:gem4:0:randomProp:suffixFactor:linkLevel
function BuildGearItemLink(gear)
    if not gear or not gear.id or gear.id <= 0 then return nil end
    local gems = gear.gems or {}
    return string.format(
        "item:%d:%d:%d:%d:%d:%d:0:%d:%d:%d",
        gear.id,
        gear.enchant or 0,
        gems[1] or 0,
        gems[2] or 0,
        gems[3] or 0,
        gems[4] or 0,
        gear.randomProp or 0,
        gear.suffixFactor or 0,
        gear.linkLevel or 0
    )
end

function BMU_HasExtendedGearPacket(parts)
    return parts and #parts >= 11
end

function MakeGearStateFromPacket(parts)
    local itemID = tonumber(parts[4]) or 0
    if itemID <= 0 then
        return { id = 0 }
    end
    local state = {
        id = itemID,
        enchant = tonumber(parts[5]) or 0,
        gems = {
            tonumber(parts[6]) or 0,
            tonumber(parts[7]) or 0,
            tonumber(parts[8]) or 0,
            tonumber(parts[9]) or 0,
        },
        randomProp = tonumber(parts[10]) or 0,
        suffixFactor = tonumber(parts[11]) or 0,
        linkLevel = 0,
    }
    state.link = BuildGearItemLink(state)
    return state
end

function MakeGearState(itemID, itemLink)
    local state = MakeTemplateItemState(itemID, itemLink)
    -- ParseItemLinkState: [8]=randomProp, [9]=suffixFactor (见 bot_ai::_AddItemLink)
    state.randomProp = state.uniqueID or 0
    state.suffixFactor = state.linkLevel or 0
    state.link = BuildGearItemLink(state)
    return state
end

EquipLocToSlot = {
    INVTYPE_HEAD = "HEAD", INVTYPE_NECK = "NECK", INVTYPE_SHOULDER = "SHOULDER",
    INVTYPE_CLOAK = "BACK", INVTYPE_CHEST = "CHEST", INVTYPE_ROBE = "CHEST",
    INVTYPE_WRIST = "WRIST", INVTYPE_HAND = "HANDS", INVTYPE_WAIST = "WAIST",
    INVTYPE_LEGS = "LEGS", INVTYPE_FEET = "FEET", INVTYPE_FINGER = "FINGER1",
    INVTYPE_TRINKET = "TRINKET1", INVTYPE_WEAPONMAINHAND = "MAINHAND", INVTYPE_2HWEAPON = "MAINHAND",
    INVTYPE_WEAPONOFFHAND = "OFFHAND", INVTYPE_SHIELD = "OFFHAND", INVTYPE_HOLDABLE = "OFFHAND",
    INVTYPE_RANGED = "RANGED", INVTYPE_THROWN = "RANGED", INVTYPE_RANGEDRIGHT = "RANGED", INVTYPE_RELIC = "RANGED",
    INVTYPE_WEAPON = "MAINHAND"
}

function GetTargetSlotForEquipLoc(botEntry, equipLoc)
    local targetSlot = EquipLocToSlot[equipLoc]
    if not targetSlot then return nil end
    local botData = db[botEntry]
    if not botData then return targetSlot end
    
    if targetSlot == "FINGER1" then
        if botData.gear and botData.gear.FINGER1 and botData.gear.FINGER1.id > 0 then
            return "FINGER2"
        end
        return "FINGER1"
    elseif targetSlot == "TRINKET1" then
        if botData.gear and botData.gear.TRINKET1 and botData.gear.TRINKET1.id > 0 then
            return "TRINKET2"
        end
        return "TRINKET1"
    elseif targetSlot == "MAINHAND" then
        -- Standard 1H weapons can go offhand if mainhand is filled
        if equipLoc == "INVTYPE_WEAPON" then
            if botData.gear and botData.gear.MAINHAND and botData.gear.MAINHAND.id > 0 then
                return "OFFHAND"
            end
        end
        return "MAINHAND"
    end
    return targetSlot
end

-- [[ WOTLK COMPATIBLE RECYCLABLE DELAY TIMER ]] --
local delayFrame = CreateFrame("Frame")
delayFrame:Hide()
delayFrame.timer = 0
delayFrame.delay = 0
delayFrame:SetScript("OnUpdate", function(self, elapsed)
    self.timer = self.timer + elapsed
    if self.timer >= self.delay then
        SendBMUMessage("REFRESH")
        self:Hide()
    end
end)

function RequestServerRefreshAfterDelay(delay)
    delayFrame.timer = 0
    delayFrame.delay = delay
    delayFrame:Show()
end

-- [[ BOTTOM-LEVEL HELPERS ]] --
function IsBotOnline(name)
    if not name then return false end
    if UnitExists("target") and UnitName("target") == name then return true end
    if UnitExists("focus") and UnitName("focus") == name then return true end
    for i = 1, 4 do
        if UnitExists("party" .. i) and UnitName("party" .. i) == name then return true end
    end
    for i = 1, 40 do
        if UnitExists("raid" .. i) and UnitName("raid" .. i) == name then return true end
    end
    return false
end

-- Scans inventory bags (0..4) and optionally bank bags if open (-1, 5..11) to count copies of an item ID
function GetItemCountInBags(targetItemID)
    local count = 0
    -- Scan standard inventory bags
    for bag = 0, 4 do
        local slotsNum = GetContainerNumSlots(bag)
        if slotsNum and slotsNum > 0 then
            for slot = 1, slotsNum do
                local id = GetContainerItemID(bag, slot)
                if id == targetItemID then
                    count = count + 1
                end
            end
        end
    end
    -- Scan bank containers (only populated when banker is open)
    local bankBags = {-1, 5, 6, 7, 8, 9, 10, 11}
    for _, bag in ipairs(bankBags) do
        local slotsNum = GetContainerNumSlots(bag)
        if slotsNum and slotsNum > 0 then
            for slot = 1, slotsNum do
                local id = GetContainerItemID(bag, slot)
                if id == targetItemID then
                    count = count + 1
                end
            end
        end
    end
    return count
end
