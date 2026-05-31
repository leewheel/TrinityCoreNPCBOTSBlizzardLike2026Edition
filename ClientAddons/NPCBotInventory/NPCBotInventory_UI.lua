-- NPCBotInventory_UI.lua - 界面与装备栏


-- [[ UI CREATION - POLISHED THEME ]] --
MainFrame = CreateFrame("Frame", "BotManagerFrame", UIParent)
MainFrame:SetSize(480, 460)
MainFrame:SetPoint("CENTER")
MainFrame:SetMovable(true)
MainFrame:SetResizable(true)
MainFrame:SetMinResize(480, 460)
MainFrame:SetMaxResize(800, 800)
MainFrame:EnableMouse(true)
MainFrame:RegisterForDrag("LeftButton")
MainFrame:SetScript("OnDragStart", MainFrame.StartMoving)
MainFrame:SetScript("OnDragStop", function(self)
    self:StopMovingOrSizing()
    if BotManager_DockEditorFrame then
        BotManager_DockEditorFrame()
    end
    if BotManager_DockStatsPanel then
        BotManager_DockStatsPanel()
    end
end)

-- Resize Grip in bottom right
local ResizeGrip = CreateFrame("Button", nil, MainFrame)
ResizeGrip:SetSize(16, 16)
ResizeGrip:SetPoint("BOTTOMRIGHT", -4, 4)
ResizeGrip:SetNormalTexture("Interface\\ChatFrame\\UI-ChatIM-SizeGrabber-Up")
ResizeGrip:SetHighlightTexture("Interface\\ChatFrame\\UI-ChatIM-SizeGrabber-Highlight")
ResizeGrip:SetPushedTexture("Interface\\ChatFrame\\UI-ChatIM-SizeGrabber-Down")
ResizeGrip:SetScript("OnMouseDown", function() MainFrame:StartSizing("BOTTOMRIGHT") end)
ResizeGrip:SetScript("OnMouseUp", function()
    MainFrame:StopMovingOrSizing()
    if BotManager_DockEditorFrame then
        BotManager_DockEditorFrame()
    end
    if BotManager_DockStatsPanel then
        BotManager_DockStatsPanel()
    end
end)

-- Clean Classic WoW Gold Dialog Border
MainFrame:SetBackdrop({
    bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
    edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Gold-Border",
    tile = true, tileSize = 32, edgeSize = 32,
    insets = { left = 8, right = 8, top = 8, bottom = 8 }
})
MainFrame:Hide()
MainFrame.inspectMode = false

tinsert(UISpecialFrames, "BotManagerFrame")
tinsert(UISpecialFrames, "BotStatsPanel")

local isHidingBotManagerWindows = false

local function HideBotManagerWindows()
    isHidingBotManagerWindows = true
    if BotManagerEditorFrame then
        BotManagerEditorFrame:Hide()
    end
    if StatsPanel then
        StatsPanel:Hide()
    end
    MainFrame.inspectMode = false
    pendingInspectEntry = nil
    if ApplyInspectModeUI then
        ApplyInspectModeUI(false)
    end
    MainFrame:Hide()
    isHidingBotManagerWindows = false
end

ToggleBotManagerWindows = function()
    if MainFrame:IsShown() or (BotManagerEditorFrame and BotManagerEditorFrame:IsShown()) or (StatsPanel and StatsPanel:IsShown()) then
        HideBotManagerWindows()
    else
        MainFrame:Show()
    end
end

local HotkeyButton = CreateFrame("Button", "BotManagerHotkeyButton", UIParent)
HotkeyButton:RegisterForClicks("AnyDown", "AnyUp")
HotkeyButton:SetScript("OnClick", ToggleBotManagerWindows)

function RegisterBotManagerHotkey()
    local key = "SHIFT-Z"
    local clickAction = "CLICK BotManagerHotkeyButton:LeftButton"

    if ClearOverrideBindings then
        ClearOverrideBindings(HotkeyButton)
    end
    if SetOverrideBindingClick then
        SetOverrideBindingClick(HotkeyButton, true, key, "BotManagerHotkeyButton", "LeftButton")
    end
    if SetBinding then
        SetBinding(key, clickAction)
    elseif SetBindingClick then
        SetBindingClick(key, "BotManagerHotkeyButton", "LeftButton")
    end
end

MainFrame:HookScript("OnHide", function()
    if not isHidingBotManagerWindows then
        HideBotManagerWindows()
    end
end)

-- 主窗口左上角保留空白（插件名由对话框边框显示）
MainFrame.title = MainFrame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
MainFrame.title:SetPoint("TOPLEFT", 22, -14)
MainFrame.title:SetJustifyH("LEFT")
MainFrame.title:SetText("")
MainFrame.title:Hide()

local closeBtn = CreateFrame("Button", nil, MainFrame, "UIPanelCloseButton")
closeBtn:SetPoint("TOPRIGHT", -5, -5)
closeBtn:SetScript("OnClick", HideBotManagerWindows)

-- Sidebar (Clean Dark Frame)
local LEFT_COLUMN_WIDTH = 140
local LEFT_COLUMN_LEFT = 15
local LEFT_COLUMN_BOTTOM = 15
local TOOLS_PANEL_HEIGHT = 104
local TOOLS_PANEL_GAP = 6

local Sidebar = CreateFrame("Frame", nil, MainFrame)
Sidebar:SetWidth(LEFT_COLUMN_WIDTH)
Sidebar:SetPoint("TOPLEFT", MainFrame, "TOPLEFT", LEFT_COLUMN_LEFT, -40)
Sidebar:SetBackdrop({
    bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
    edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 16,
    insets = {left = 4, right = 4, top = 4, bottom = 4}
})
Sidebar:SetBackdropColor(0.1, 0.1, 0.1, 0.9)

local SidebarTools = CreateFrame("Frame", nil, MainFrame)
SidebarTools:SetSize(LEFT_COLUMN_WIDTH, TOOLS_PANEL_HEIGHT)
SidebarTools:SetPoint("BOTTOMLEFT", MainFrame, "BOTTOMLEFT", LEFT_COLUMN_LEFT, LEFT_COLUMN_BOTTOM)
SidebarTools:SetBackdrop({
    bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
    edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 16,
    insets = {left = 4, right = 4, top = 4, bottom = 4}
})
SidebarTools:SetBackdropColor(0.08, 0.08, 0.08, 0.92)

Sidebar:SetPoint("BOTTOMLEFT", SidebarTools, "TOPLEFT", 0, TOOLS_PANEL_GAP)

-- [[ SMART SEARCH BAR ]] --
searchBox = CreateFrame("EditBox", nil, Sidebar, "InputBoxTemplate")
searchBox:SetSize(110, 20)
searchBox:SetPoint("TOP", Sidebar, "TOP", 0, -15)
searchBox:SetAutoFocus(false)
searchBox:SetFontObject("ChatFontSmall")
searchBox.text = searchBox:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
searchBox.text:SetPoint("LEFT", 5, 0)
searchBox.text:SetText("搜索机器人/物品…")

searchBox:SetScript("OnEditFocusGained", function(self) self.text:Hide() end)
searchBox:SetScript("OnEditFocusLost", function(self)
    if self:GetText() == "" then self.text:Show() end
end)
searchBox:SetScript("OnTextChanged", function(self)
    if self:GetText() ~= "" then self.text:Hide() end
    RefreshBotList(self:GetText():lower())
end)

-- [[ SORT BUTTON ]] --
local sortBtn = CreateFrame("Button", nil, Sidebar, "UIPanelButtonTemplate")
sortBtn:SetSize(110, 20)
sortBtn:SetPoint("TOP", searchBox, "BOTTOM", 0, -5)
sortBtn:SetText("分组：" .. SortModeLabels[currentSortMode])
sortBtn:SetScript("OnClick", function(self)
    local menuList = {
        { text = "机器人分组方式", isTitle = true, notCheckable = true },
        {
            text = "职业",
            checked = currentSortMode == "CLASS",
            func = function()
                currentSortMode = "CLASS"
                sortBtn:SetText("分组：" .. SortModeLabels[currentSortMode])
                RefreshBotList(searchBox:GetText():lower())
            end
        },
        {
            text = "名字",
            checked = currentSortMode == "NAME",
            func = function()
                currentSortMode = "NAME"
                sortBtn:SetText("分组：" .. SortModeLabels[currentSortMode])
                RefreshBotList(searchBox:GetText():lower())
            end
        },
        {
            text = "职责",
            checked = currentSortMode == "ROLE",
            func = function()
                currentSortMode = "ROLE"
                sortBtn:SetText("分组：" .. SortModeLabels[currentSortMode])
                RefreshBotList(searchBox:GetText():lower())
            end
        },
        {
            text = "天赋",
            checked = currentSortMode == "TALENT",
            func = function()
                currentSortMode = "TALENT"
                sortBtn:SetText("分组：" .. SortModeLabels[currentSortMode])
                RefreshBotList(searchBox:GetText():lower())
            end
        },
    }
    EasyMenu(menuList, menuFrame, self, 0, 0, "MENU")
end)

-- Refresh Data Button
local syncBtn = CreateFrame("Button", nil, SidebarTools, "UIPanelButtonTemplate")
syncBtn:SetSize(58, 22)
syncBtn:SetPoint("TOPLEFT", SidebarTools, "TOPLEFT", 10, -10)
syncBtn:SetText("同步")
syncBtn:SetFrameLevel(SidebarTools:GetFrameLevel() + 5)
syncBtn:SetScript("OnClick", function()
    BMU_ResetDebugStats()
    SendBMUMessage("REFRESH")
    print("|cff66ccff机器人管理:|r 已请求同步，等待服务器返回…")
    if BMU_DEBUG then
        BMU_Log("点击同步，playerName=" .. tostring(playerName))
    end
end)

-- [[ GEARING SEQUENTIAL QUEUE & PREMIUM PROGRESS BAR UI ]] --
local actionQueue = {}
local currentActionIndex = 0
local totalActionsCount = 0
local queueActive = false
local queueTargetBot = nil
local queueTargetBotName = nil
local queueType = nil

local ProgressBarModal = CreateFrame("Frame", "BotManagerProgressBarModal", MainFrame)
ProgressBarModal:SetAllPoints(MainFrame)
ProgressBarModal:SetFrameLevel(MainFrame:GetFrameLevel() + 20) -- Lock overlay on top
ProgressBarModal:EnableMouse(true) -- Intercept clicks
ProgressBarModal:Hide()

ProgressBarModal:SetBackdrop({
    bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background-Dark",
    edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Gold-Border",
    tile = true, tileSize = 32, edgeSize = 32,
    insets = { left = 8, right = 8, top = 8, bottom = 8 }
})
ProgressBarModal:SetBackdropColor(0.05, 0.05, 0.08, 0.9)

local barTitle = ProgressBarModal:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
barTitle:SetPoint("CENTER", ProgressBarModal, "CENTER", 0, 40)
barTitle:SetText("正在为机器人换装…")
barTitle:SetTextColor(1, 0.82, 0) -- Gold

local barDetail = ProgressBarModal:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
barDetail:SetPoint("TOP", barTitle, "BOTTOM", 0, -10)
barDetail:SetText("准备中…")

local barBG = CreateFrame("Frame", nil, ProgressBarModal)
barBG:SetSize(280, 20)
barBG:SetPoint("TOP", barDetail, "BOTTOM", 0, -15)
barBG:SetBackdrop({
    bgFile = "Interface\\ChatFrame\\ChatFrameBackground",
    edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 12,
    insets = { left = 3, right = 3, top = 3, bottom = 3 }
})
barBG:SetBackdropColor(0, 0, 0, 0.8)
barBG:SetBackdropBorderColor(0.4, 0.4, 0.4, 0.8)

local statusBar = barBG:CreateTexture(nil, "ARTWORK")
statusBar:SetTexture("Interface\\TargetingFrame\\UI-StatusBar")
statusBar:SetVertexColor(0, 0.6, 0.9, 0.9) -- Vibrant Neon Blue / Cyan
statusBar:SetPoint("LEFT", barBG, "LEFT", 3, 0)
statusBar:SetPoint("TOP", barBG, "TOP", 0, -3)
statusBar:SetPoint("BOTTOM", barBG, "BOTTOM", 0, 3)
statusBar:SetWidth(1)

local barPercent = barBG:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
barPercent:SetPoint("CENTER", barBG, "CENTER", 0, 0)
barPercent:SetText("0%")
barPercent:SetTextColor(1, 1, 1)

local function SetGearingProgress(percent, detailText)
    percent = math.max(0, math.min(1, percent))
    local maxBarWidth = 280 - 6
    local width = maxBarWidth * percent
    if width <= 0.1 then
        statusBar:Hide()
    else
        statusBar:Show()
        statusBar:SetWidth(width)
    end
    barPercent:SetText(string.format("%d%%", math.floor(percent * 100)))
    if detailText then
        barDetail:SetText(detailText)
    end
end

local queueTimerFrame = CreateFrame("Frame")
queueTimerFrame:Hide()
queueTimerFrame.elapsed = 0
queueTimerFrame.interval = 0.50 -- Raised to 500ms to maintain reliable sequential server processing and avoid duplications

queueTimerFrame:SetScript("OnUpdate", function(self, elapsed)
    if not queueActive then
        self:Hide()
        return
    end
    self.elapsed = self.elapsed + elapsed
    if self.elapsed >= self.interval then
        self.elapsed = self.elapsed - self.interval
        
        currentActionIndex = currentActionIndex + 1
        if currentActionIndex <= totalActionsCount then
            local action = actionQueue[currentActionIndex]
            local percent = currentActionIndex / totalActionsCount
            
            local targetBot = action.botEntry or queueTargetBot
            local botName = action.botName or queueTargetBotName
            local botData = db[targetBot]
            
            if action.type == "EQUIP" then
                local itemName = action.itemName or ("物品 #" .. action.itemID)
                local progressText = string.format("正在为 %s 装备 %s（%d/%d）…", botName, itemName, currentActionIndex, totalActionsCount)
                SetGearingProgress(percent, progressText)
                
                -- Authoritative update: Optimistic client updates during bulk template applications are omitted
                -- to prevent plate gear from sticking visually on non-plate bots if the server rejects it.
                SendBMUMessage("EQUIP;" .. targetBot .. ";" .. action.itemID .. ";" .. action.slotKey)
                
            elseif action.type == "UNEQUIP" then
                local progressText = string.format("正在卸下 %s 的 %s（%d/%d）…", botName, action.slotKey, currentActionIndex, totalActionsCount)
                SetGearingProgress(percent, progressText)
                
                SendBMUMessage("UNEQUIP;" .. targetBot .. ";" .. action.slotKey)
            end
            
            -- Keep visuals authoritative
            SelectBot(targetBot)
        else
            -- Completed all actions
            queueActive = false
            self:Hide()
            
            local completionMsg = "换装完成！"
            if queueType == "UNEQUIP_ALL" then
                completionMsg = "已全部卸下装备！"
            end
            SetGearingProgress(1, completionMsg)
            
            -- Short delay before closing and final sync
            local finishFrame = CreateFrame("Frame")
            finishFrame.elapsed = 0
            finishFrame:SetScript("OnUpdate", function(ff, elapsedFF)
                ff.elapsed = ff.elapsed + elapsedFF
                if ff.elapsed >= 0.3 then
                    BotManagerProgressBarModal:Hide()
                    SendBMUMessage("REFRESH")
                    print("|cff00ff00机器人管理:|r 换装队列已完成。")
                    ff:SetScript("OnUpdate", nil)
                end
            end)
        end
    end
end)

function StartSequentialActions(actions, targetBot, botName, qType)
    if not actions or #actions == 0 then
        print("|cffff0000机器人管理:|r 没有可执行的操作。")
        return
    end
    
    actionQueue = actions
    currentActionIndex = 0
    totalActionsCount = #actions
    queueActive = true
    queueTargetBot = targetBot
    queueTargetBotName = botName or "机器人"
    queueType = qType
    
    if qType == "UNEQUIP_ALL" then
        barTitle:SetText("正在卸下「" .. queueTargetBotName .. "」的装备…")
    else
        barTitle:SetText("正在为「" .. queueTargetBotName .. "」换装…")
    end
    
    SetGearingProgress(0, "正在初始化换装队列…")
    BotManagerProgressBarModal:Show()
    
    queueTimerFrame.elapsed = 0
    queueTimerFrame:Show()
end

-- [[ STATS TOGGLE BUTTON ]] --
local statsToggleBtn = CreateFrame("Button", nil, SidebarTools, "UIPanelButtonTemplate")
statsToggleBtn:SetSize(120, 22)
statsToggleBtn:SetPoint("TOP", SidebarTools, "TOP", 0, -38)
statsToggleBtn:SetText("属性面板")
statsToggleBtn:SetFrameLevel(SidebarTools:GetFrameLevel() + 5)
statsToggleBtn:SetScript("OnClick", function()
    db.showStats = not db.showStats
    if db.showStats then
        if MainFrame.selectedBot then
            UpdateStatsPanel(MainFrame.selectedBot)
        else
            StatsPanel:Show()
        end
        DockStatsPanel()
        print("|cff00ff00机器人管理:|r 已显示属性面板。")
    else
        StatsPanel:Hide()
        print("|cff00ff00机器人管理:|r 已隐藏属性面板。")
    end
end)

-- [[ EDITOR & CATEGORIES TOGGLE BUTTON ]] --
local editorBtn = CreateFrame("Button", nil, SidebarTools, "UIPanelButtonTemplate")
editorBtn:SetSize(120, 22)
editorBtn:SetPoint("TOP", SidebarTools, "TOP", 0, -64)
editorBtn:SetText("模板与战利品分配")
editorBtn:SetFrameLevel(SidebarTools:GetFrameLevel() + 5)
editorBtn:SetScript("OnClick", function()
    if BotManagerEditorFrame then
        if BotManagerEditorFrame:IsShown() then
            BotManagerEditorFrame:Hide()
        else
            BotManagerEditorFrame:Show()
        end
        DockStatsPanel()
    else
        print("|cffff0000机器人管理:|r 编辑器窗口尚未加载。")
    end
end)

-- Templates Button
local templatesBtn = CreateFrame("Button", nil, SidebarTools, "UIPanelButtonTemplate")
templatesBtn:SetSize(62, 22)
templatesBtn:SetPoint("TOPRIGHT", SidebarTools, "TOPRIGHT", -10, -10)
templatesBtn:SetText("模板")
templatesBtn:SetFrameLevel(SidebarTools:GetFrameLevel() + 5)
templatesBtn:SetScript("OnClick", function(self)
    -- Show Templates dropdown menu
    local menuList = {
        {
            text = "装备模板",
            isTitle = true,
            notCheckable = true,
        }
    }
    
    -- 1. Apply Template to Current Bot
    if MainFrame.selectedBot then
        local botData = db[MainFrame.selectedBot]
        local activeTemplates = GetTemplateBucketForBot(botData, false)
        local botName = botData and botData.name or "当前机器人"
        local applyMenu = {
            text = "应用到「" .. botName .. "」",
            hasArrow = true,
            notCheckable = true,
            menuList = {}
        }
        local hasTemplates = false
        for tName in pairs(activeTemplates or {}) do
            hasTemplates = true
            local templateName = tName
            local templateData = activeTemplates[templateName]
            table.insert(applyMenu.menuList, {
                    text = templateName,
                    notCheckable = true,
                    func = function()
                        local t = activeTemplates[templateName]
                        if t then
                        local actions = {}
                        
                        -- First, filter out compatibility-blocked swaps
                        local validSwaps = {}
                        for _, slotKey in ipairs(TEMPLATE_SLOT_KEYS) do
                            local itemState = t[slotKey]
                            local itemID = GetTemplateItemID(t, slotKey)
                            if itemID and itemID > 0 then
                                local currentItem = botData and botData.gear and botData.gear[slotKey] and botData.gear[slotKey].id or 0
                                if currentItem ~= itemID then
                                    local canEquip, failReason = CanBotEquipItem(botData.className, itemID, MainFrame.selectedBot)
                                    if canEquip then
                                        table.insert(validSwaps, { slotKey = slotKey, itemID = itemID, itemState = itemState })
                                    elseif failReason == "CACHE" then
                                        print("|cff00ff00机器人管理:|r 正在查询物品数据库，请稍后再试。")
                                    end
                                end
                            end
                        end
                        
                        -- Compile how many copies of each item ID are requested by this template
                        local neededItemCounts = {}
                        for _, swap in ipairs(validSwaps) do
                            neededItemCounts[swap.itemID] = (neededItemCounts[swap.itemID] or 0) + 1
                        end
                        
                        -- Check copies currently in bags/bank. Only pull from other bots if we are short on physical copies!
                        local blockedSwaps = {}
                        for itemID, needed in pairs(neededItemCounts) do
                            local availableCount = GetItemCountInBags(itemID)
                            if availableCount < needed then
                                local neededFromBots = needed - availableCount
                                for otherEntry, otherBotData in pairs(db) do
                                    if type(otherEntry) == "number" and otherEntry ~= MainFrame.selectedBot then
                                        if otherBotData.gear then
                                            for otherSlotKey, otherGearData in pairs(otherBotData.gear) do
                                                if otherGearData and otherGearData.id == itemID then
                                                    if IsBotOnline(otherBotData.name) then
                                                        table.insert(actions, {
                                                            type = "UNEQUIP",
                                                            botEntry = otherEntry,
                                                            botName = otherBotData.name,
                                                            slotKey = otherSlotKey
                                                        })
                                                        neededFromBots = neededFromBots - 1
                                                        if neededFromBots <= 0 then break end
                                                    else
                                                        print("|cffff0000机器人管理警告:|r 无法从离线机器人「" .. (otherBotData.name or "未知") .. "」自动卸下物品，已跳过该交换。")
                                                        blockedSwaps[itemID] = true
                                                    end
                                                end
                                            end
                                        end
                                    end
                                    if neededFromBots <= 0 then break end
                                end
                            end
                        end
                        
                        -- Append final compatible equipping operations
                        for _, swap in ipairs(validSwaps) do
                            local itemID = swap.itemID
                            if not blockedSwaps[itemID] then
                                local slotKey = swap.slotKey
                                local itemName = GetItemInfo(itemID) or ("物品 #" .. itemID)
                                table.insert(actions, {
                                    type = "EQUIP",
                                    botEntry = MainFrame.selectedBot,
                                    botName = botName,
                                    slotKey = slotKey,
                                    itemID = itemID,
                                    enchant = swap.itemState and swap.itemState.enchant or 0,
                                    gems = swap.itemState and swap.itemState.gems or nil,
                                    itemLink = swap.itemState and swap.itemState.link or nil,
                                    itemName = itemName
                                })
                            end
                        end
                        
                        if #actions > 0 then
                            StartSequentialActions(actions, MainFrame.selectedBot, botName, "APPLY_TEMPLATE")
                        else
                            print("|cff00ff00机器人管理:|r 模板「" .. templateName .. "」中所有可用物品已在「" .. botName .. "」身上装备。")
                        end
                        end
                    end
                })
        end
        if #applyMenu.menuList > 0 then
            table.insert(menuList, applyMenu)
        elseif hasTemplates then
            table.insert(menuList, {
                text = "应用到「" .. botName .. "」 - |cff808080无职责模板|r",
                notCheckable = true,
                disabled = true
            })
        else
            table.insert(menuList, {
                text = "应用到「" .. botName .. "」 - |cff808080无模板|r",
                notCheckable = true,
                disabled = true
            })
        end
    else
        table.insert(menuList, {
            text = "应用到机器人 - |cff808080未选择机器人|r",
            notCheckable = true,
            disabled = true
        })
    end
    
    -- 2. Create Template from Current Bot's Gear
    if MainFrame.selectedBot then
        local botData = db[MainFrame.selectedBot]
        local botName = botData and botData.name or "当前机器人"
        table.insert(menuList, {
            text = "将「" .. botName .. "」的装备保存为模板…",
            notCheckable = true,
            func = function()
                CloseDropDownMenus()
                StaticPopup_Show("NPCBOT_CREATE_TEMPLATE_FROM_GEAR")
            end
        })
    else
        table.insert(menuList, {
            text = "保存装备为模板… - |cff808080未选择机器人|r",
            notCheckable = true,
            disabled = true
        })
    end
    
    -- 3. Unequip All Gear from Current Bot
    if MainFrame.selectedBot then
        local botData = db[MainFrame.selectedBot]
        local botName = botData and botData.name or "当前机器人"
        table.insert(menuList, {
            text = "卸下「" .. botName .. "」的全部装备",
            notCheckable = true,
            func = function()
                local slotKeys = {"HEAD", "NECK", "SHOULDER", "BACK", "CHEST", "WRIST", "HANDS", "WAIST", "LEGS", "FEET", "FINGER1", "FINGER2", "TRINKET1", "TRINKET2", "MAINHAND", "OFFHAND", "RANGED"}
                local actions = {}
                for _, slotKey in ipairs(slotKeys) do
                    if botData.gear and botData.gear[slotKey] and botData.gear[slotKey].id > 0 then
                        table.insert(actions, { type = "UNEQUIP", slotKey = slotKey })
                    end
                end
                if #actions > 0 then
                    StartSequentialActions(actions, MainFrame.selectedBot, botName, "UNEQUIP_ALL")
                else
                    print("|cffff0000机器人管理:|r 「" .. botName .. "」当前没有装备。")
                end
            end
        })
    else
        table.insert(menuList, {
            text = "卸下全部装备 - |cff808080未选择机器人|r",
            notCheckable = true,
            disabled = true
        })
    end
    
    -- 4. Delete Template
    local deleteMenu = {
        text = "删除模板",
        hasArrow = true,
        notCheckable = true,
        menuList = {}
    }
    local activeTemplates = GetSelectedTemplateBucket(false)
    local hasTemplates = false
    for tName in pairs(activeTemplates or {}) do
        hasTemplates = true
        local templateName = tName
        table.insert(deleteMenu.menuList, {
            text = templateName,
            notCheckable = true,
            func = function()
                activeTemplates[templateName] = nil
                print("|cff00ff00机器人管理:|r 已删除模板「" .. templateName .. "」。")
                -- Re-open the templates dropdown menu on the next frame to show the updated list instantly!
                local f = CreateFrame("Frame")
                f:SetScript("OnUpdate", function(selfFrame)
                    CloseDropDownMenus()
                    templatesBtn:GetScript("OnClick")(templatesBtn)
                    selfFrame:SetScript("OnUpdate", nil)
                end)
            end
        })
    end
    if hasTemplates then
        table.insert(menuList, deleteMenu)
    else
        table.insert(menuList, {
            text = "删除模板 - |cff808080无模板|r",
            notCheckable = true,
            disabled = true
        })
    end
    
    -- Show EasyMenu at the button
    EasyMenu(menuList, menuFrame, self, 0, 0, "MENU")
end)

-- Scroll Frame
local ScrollFrame = CreateFrame("ScrollFrame", "BotListScroll", Sidebar, "UIPanelScrollFrameTemplate")
ScrollFrame:SetPoint("TOPLEFT", 5, -70)
ScrollFrame:SetPoint("BOTTOMRIGHT", -25, 10)

local ScrollContent = CreateFrame("Frame", nil, ScrollFrame)
ScrollContent:SetSize(100, 1)
ScrollFrame:SetScrollChild(ScrollContent)

local BotButtonsPool = {}
for i = 1, 100 do
    local b = CreateFrame("Button", "BMU_BotButton_" .. i, ScrollContent, "SecureActionButtonTemplate")
    b:SetSize(100, 22)
    b:SetNormalFontObject("GameFontNormalSmall")
    b:SetHighlightTexture("Interface\\Buttons\\UI-Listbox-Highlight")
    b:Hide()
    table.insert(BotButtonsPool, b)
end

-- Gear Container
local GearContainer = CreateFrame("Frame", nil, MainFrame)
GearContainer:SetPoint("TOPLEFT", Sidebar, "TOPRIGHT", 10, 0)
GearContainer:SetPoint("BOTTOMRIGHT", -15, 15) 
GearContainer:SetBackdrop({
    bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
    edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 16,
    insets = {left = 4, right = 4, top = 4, bottom = 4}
})
GearContainer:SetBackdropColor(0.15, 0.15, 0.15, 0.95)

local GEAR_HEADER_HEIGHT = 28

-- [[ BOT STATS PANEL ]] --
StatsPanel = CreateFrame("Frame", "BotStatsPanel", UIParent)
StatsPanel:SetSize(265, 460)
StatsPanel:SetPoint("TOPLEFT", MainFrame, "TOPRIGHT", 10, 0)
StatsPanel:SetMovable(true)
StatsPanel:SetResizable(true)
StatsPanel:SetMinResize(245, 460)
StatsPanel:SetMaxResize(420, 800)
StatsPanel:SetClampedToScreen(true)
StatsPanel:EnableMouse(true)
StatsPanel:RegisterForDrag("LeftButton")
StatsPanel:SetScript("OnDragStart", function(self)
    self:StartMoving()
end)
StatsPanel:SetScript("OnDragStop", function(self)
    self:StopMovingOrSizing()
    self.hasBeenMoved = true
end)
StatsPanel:SetBackdrop({
    bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
    edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Gold-Border",
    tile = true, tileSize = 32, edgeSize = 32,
    insets = { left = 8, right = 8, top = 8, bottom = 8 }
})
StatsPanel:Hide() -- Only show when a bot is selected

local function StopStatsResize()
    StatsPanel:StopMovingOrSizing()
end

DockStatsPanel = function()
    if not StatsPanel or not StatsPanel:IsShown() or StatsPanel.hasBeenMoved then return end

    StatsPanel:ClearAllPoints()
    if BotManagerEditorFrame and BotManagerEditorFrame:IsShown() then
        if not StatsPanel.hasBeenResized then
            StatsPanel:SetHeight(BotManagerEditorFrame:GetHeight() or 460)
        end
        StatsPanel:SetPoint("TOPLEFT", BotManagerEditorFrame, "TOPRIGHT", 10, 0)
    else
        if not StatsPanel.hasBeenResized then
            StatsPanel:SetHeight(MainFrame:GetHeight() or 460)
        end
        StatsPanel:SetPoint("TOPLEFT", MainFrame, "TOPRIGHT", 10, 0)
    end
end

BotManager_DockStatsPanel = DockStatsPanel

local statsResizeGrip = CreateFrame("Button", nil, StatsPanel)
statsResizeGrip:SetSize(16, 16)
statsResizeGrip:SetPoint("BOTTOMRIGHT", StatsPanel, "BOTTOMRIGHT", -4, 4)
statsResizeGrip:SetNormalTexture("Interface\\ChatFrame\\UI-ChatIM-SizeGrabber-Up")
statsResizeGrip:SetHighlightTexture("Interface\\ChatFrame\\UI-ChatIM-SizeGrabber-Highlight")
statsResizeGrip:SetPushedTexture("Interface\\ChatFrame\\UI-ChatIM-SizeGrabber-Down")
statsResizeGrip:SetScript("OnMouseDown", function(self, button)
    if button == "LeftButton" then
        StatsPanel.hasBeenResized = true
        StatsPanel:StartSizing("BOTTOMRIGHT")
    end
end)
statsResizeGrip:SetScript("OnMouseUp", StopStatsResize)
StatsPanel:HookScript("OnHide", StopStatsResize)
StatsPanel:HookScript("OnShow", DockStatsPanel)
MainFrame:HookScript("OnSizeChanged", DockStatsPanel)

-- Stats Panel Close Button
local statsCloseBtn = CreateFrame("Button", nil, StatsPanel, "UIPanelCloseButton")
statsCloseBtn:SetPoint("TOPRIGHT", StatsPanel, "TOPRIGHT", -2, -2)
statsCloseBtn:SetScript("OnClick", function()
    StatsPanel:Hide()
    db.showStats = false
    print("|cff00ff00机器人管理:|r 属性面板已隐藏，可点击「属性面板」重新打开。")
end)

local statsTitle = StatsPanel:CreateFontString(nil, "OVERLAY", "GameFontNormal")
statsTitle:SetPoint("TOP", 0, -15)
statsTitle:SetText("机器人属性")
statsTitle:SetTextColor(1, 0.82, 0)

local statsContextText = StatsPanel:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
statsContextText:SetPoint("TOPLEFT", 20, -34)
statsContextText:SetPoint("RIGHT", StatsPanel, "RIGHT", -26, 0)
statsContextText:SetJustifyH("CENTER")
statsContextText:SetTextColor(0.68, 0.85, 1)
statsContextText:SetText("")

local statsStatusText = StatsPanel:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
statsStatusText:SetPoint("TOPLEFT", 20, -51)
statsStatusText:SetPoint("RIGHT", StatsPanel, "RIGHT", -26, 0)
statsStatusText:SetJustifyH("CENTER")
statsStatusText:SetText("")

local StatsScrollFrame = CreateFrame("ScrollFrame", "BotStatsScrollFrame", StatsPanel, "UIPanelScrollFrameTemplate")
StatsScrollFrame:SetPoint("TOPLEFT", 18, -72)
StatsScrollFrame:SetPoint("BOTTOMRIGHT", -32, 18)

local StatsContent = CreateFrame("Frame", nil, StatsScrollFrame)
StatsContent:SetSize(210, 1)
StatsScrollFrame:SetScrollChild(StatsContent)

local BotSpecInfo = {
    [1] = { classToken = "WARRIOR", specIndex = 1, specName = "武器" },
    [2] = { classToken = "WARRIOR", specIndex = 2, specName = "狂怒" },
    [3] = { classToken = "WARRIOR", specIndex = 3, specName = "防护" },
    [4] = { classToken = "PALADIN", specIndex = 1, specName = "神圣" },
    [5] = { classToken = "PALADIN", specIndex = 2, specName = "防护" },
    [6] = { classToken = "PALADIN", specIndex = 3, specName = "惩戒" },
    [7] = { classToken = "HUNTER", specIndex = 1, specName = "野兽控制" },
    [8] = { classToken = "HUNTER", specIndex = 2, specName = "射击" },
    [9] = { classToken = "HUNTER", specIndex = 3, specName = "生存" },
    [10] = { classToken = "ROGUE", specIndex = 1, specName = "刺杀" },
    [11] = { classToken = "ROGUE", specIndex = 2, specName = "战斗" },
    [12] = { classToken = "ROGUE", specIndex = 3, specName = "敏锐" },
    [13] = { classToken = "PRIEST", specIndex = 1, specName = "戒律" },
    [14] = { classToken = "PRIEST", specIndex = 2, specName = "神圣" },
    [15] = { classToken = "PRIEST", specIndex = 3, specName = "暗影" },
    [16] = { classToken = "DEATHKNIGHT", specIndex = 1, specName = "鲜血" },
    [17] = { classToken = "DEATHKNIGHT", specIndex = 2, specName = "冰霜" },
    [18] = { classToken = "DEATHKNIGHT", specIndex = 3, specName = "邪恶" },
    [19] = { classToken = "SHAMAN", specIndex = 1, specName = "元素" },
    [20] = { classToken = "SHAMAN", specIndex = 2, specName = "增强" },
    [21] = { classToken = "SHAMAN", specIndex = 3, specName = "恢复" },
    [22] = { classToken = "MAGE", specIndex = 1, specName = "奥术" },
    [23] = { classToken = "MAGE", specIndex = 2, specName = "火焰" },
    [24] = { classToken = "MAGE", specIndex = 3, specName = "冰霜" },
    [25] = { classToken = "WARLOCK", specIndex = 1, specName = "痛苦" },
    [26] = { classToken = "WARLOCK", specIndex = 2, specName = "恶魔学识" },
    [27] = { classToken = "WARLOCK", specIndex = 3, specName = "毁灭" },
    [28] = { classToken = "DRUID", specIndex = 1, specName = "平衡" },
    [29] = { classToken = "DRUID", specIndex = 2, specName = "野性" },
    [30] = { classToken = "DRUID", specIndex = 3, specName = "恢复" },
    [31] = { classToken = nil, specIndex = 0, specName = "默认" },
}

local ClassTokenByName = {
    ["Warrior"] = "WARRIOR",
    ["Paladin"] = "PALADIN",
    ["Hunter"] = "HUNTER",
    ["Dark Ranger"] = "HUNTER",
    ["Rogue"] = "ROGUE",
    ["Priest"] = "PRIEST",
    ["Death Knight"] = "DEATHKNIGHT",
    ["Shaman"] = "SHAMAN",
    ["Sea Witch"] = "SHAMAN",
    ["Mage"] = "MAGE",
    ["Archmage"] = "MAGE",
    ["Warlock"] = "WARLOCK",
    ["Necromancer"] = "WARLOCK",
    ["Druid"] = "DRUID",
    ["Sphynx"] = "MAGE",
}

local RoleLabelByCategory = {
    TANK = "坦克",
    HEALER = "治疗",
    MELEE_DPS = "近战输出",
    RANGED_DPS = "远程输出",
    CASTER_DPS = "法系输出",
}

local function BotStatsRound(value, digits)
    local mult = 1
    if digits and digits > 0 then
        mult = 10 ^ digits
    end
    return math.floor((tonumber(value) or 0) * mult + 0.5) / mult
end

local function AddBotStatsTooltipLine(text, r, g, b)
    if text and text ~= "" then
        GameTooltip:AddLine(text, r or 1, g or 1, b or 1, true)
    end
end

local function AddBotStatsPriority(priority)
    if priority == "high" then
        AddBotStatsTooltipLine("对本机器人优先级：高", 0.25, 1.00, 0.25)
    elseif priority == "medium" then
        AddBotStatsTooltipLine("对本机器人优先级：中", 1.00, 0.82, 0.20)
    else
        AddBotStatsTooltipLine("对本机器人优先级：低", 0.60, 0.60, 0.60)
    end
end

local function AddBotStatsCapStatus(label, current, target, suffix)
    current = tonumber(current) or 0
    target = tonumber(target) or 0
    suffix = suffix or ""
    local missing = target - current

    if missing <= 0.05 then
        AddBotStatsTooltipLine(label .. ": capped at " .. tostring(target) .. suffix .. ".", 0.25, 1.00, 0.25)
    else
        AddBotStatsTooltipLine(label .. ": needs about " .. tostring(BotStatsRound(missing, 2)) .. suffix .. " more to reach " .. tostring(target) .. suffix .. ".", 1.00, 0.30, 0.25)
    end
end

local function GetBotStatsContext(botData)
    botData = botData or {}
    local spec = tonumber(botData.spec) or 0
    local specInfo = BotSpecInfo[spec] or BotSpecInfo[31]
    local classToken = specInfo.classToken or ClassTokenByName[botData.className]
    local roleCategory = GetBotRoleCategory and GetBotRoleCategory(botData) or nil
    local level = tonumber(botData.level) or UnitLevel("player") or 80

    return {
        className = botData.className or "未知",
        classToken = classToken,
        level = level,
        specName = specInfo.specName or "默认",
        specIndex = specInfo.specIndex or 0,
        roleCategory = roleCategory,
        roleLabel = RoleLabelByCategory[roleCategory] or "未知职责",
        bossLevel = level + 3,
        bossDefenseCap = (level * 5) + 140,
    }
end

function GetBotRolesLabel(roles)
    roles = tonumber(roles) or 0
    local labels = {}

    if bit.band(roles, 1) > 0 then table.insert(labels, "坦克") end
    if bit.band(roles, 2) > 0 then table.insert(labels, "副坦") end
    if bit.band(roles, 8) > 0 then table.insert(labels, "治疗") end
    if bit.band(roles, 16) > 0 then table.insert(labels, "远程") end
    if bit.band(roles, 4) > 0 then table.insert(labels, "输出") end

    if #labels == 0 then
        return "输出"
    end
    return table.concat(labels, "/")
end

function GetBotTalentLabel(botData)
    local spec = tonumber(botData and botData.spec) or 0
    local specInfo = BotSpecInfo[spec] or BotSpecInfo[31]
    return specInfo.specName or "默认"
end

local RaceNamesZh = {
    [1] = "人类", [2] = "兽人", [3] = "矮人", [4] = "暗夜精灵", [5] = "亡灵",
    [6] = "牛头人", [7] = "侏儒", [8] = "巨魔", [10] = "血精灵", [11] = "德莱尼",
}

local ClassNamesZh = {
    ["Warrior"] = "战士", ["Paladin"] = "圣骑士", ["Hunter"] = "猎人",
    ["Rogue"] = "潜行者", ["Priest"] = "牧师", ["Death Knight"] = "死亡骑士",
    ["Shaman"] = "萨满", ["Mage"] = "法师", ["Warlock"] = "术士", ["Druid"] = "德鲁伊",
    ["Blademaster"] = "剑圣", ["Sphynx"] = "斯芬克斯", ["Archmage"] = "大法师",
    ["Dreadlord"] = "恐惧魔王", ["Spellbreaker"] = "破法者", ["Dark Ranger"] = "黑暗游侠",
    ["Necromancer"] = "死灵法师", ["Sea Witch"] = "海女巫", ["Crypt Lord"] = "地穴领主",
}

local GenderNamesZh = { [0] = "男", [1] = "女" }

local function GetBotRaceLabel(raceId)
    return RaceNamesZh[tonumber(raceId) or 0] or "未知种族"
end

local function GetBotClassLabel(className)
    return ClassNamesZh[className] or className or "未知"
end

function GetBotIdentityText(data)
    if not data then return "" end
    local raceLabel = GetBotRaceLabel(data.race)
    local genderLabel = GenderNamesZh[tonumber(data.gender) or 0]
    local classLabel = GetBotClassLabel(data.className)
    local talentLabel = GetBotTalentLabel(data)
    local roleLabel = GetBotRolesLabel(data.roles)
    if genderLabel then
        return string.format("种族：%s(%s)  职业：%s  天赋：%s  职责：%s", raceLabel, genderLabel, classLabel, talentLabel, roleLabel)
    end
    return string.format("种族：%s  职业：%s  天赋：%s  职责：%s", raceLabel, classLabel, talentLabel, roleLabel)
end

function GetBotIdentityInlineText(data)
    if not data then return "" end
    local raceLabel = GetBotRaceLabel(data.race)
    local genderLabel = GenderNamesZh[tonumber(data.gender) or 0]
    local classLabel = GetBotClassLabel(data.className)
    local talentLabel = GetBotTalentLabel(data)
    if genderLabel then
        return string.format("种族：%s(%s)  职业：%s  天赋：%s", raceLabel, genderLabel, classLabel, talentLabel)
    end
    return string.format("种族：%s  职业：%s  天赋：%s", raceLabel, classLabel, talentLabel)
end

local function BotStatsIsTank(ctx)
    return ctx and ctx.roleCategory == "TANK"
end

local function BotStatsIsHealer(ctx)
    return ctx and ctx.roleCategory == "HEALER"
end

local function BotStatsIsCaster(ctx)
    return ctx and (ctx.roleCategory == "CASTER_DPS" or ctx.classToken == "MAGE" or ctx.classToken == "WARLOCK")
end

local function BotStatsIsMelee(ctx)
    return ctx and (ctx.roleCategory == "MELEE_DPS" or ctx.roleCategory == "TANK" or ctx.classToken == "ROGUE" or ctx.classToken == "WARRIOR" or ctx.classToken == "DEATHKNIGHT")
end

local function BotStatsIsRangedPhysical(ctx)
    return ctx and (ctx.roleCategory == "RANGED_DPS" or ctx.classToken == "HUNTER")
end

local function AddBotPrimaryStatAdvice(key, ctx)
    if key == "strength" then
        if ctx.classToken == "WARRIOR" or ctx.classToken == "PALADIN" or ctx.classToken == "DEATHKNIGHT" then
            AddBotStatsPriority("high")
            AddBotStatsTooltipLine("Strength feeds attack power and threat for plate melee and shield tanks.", 0.84, 0.84, 0.84)
        elseif ctx.classToken == "SHAMAN" and ctx.specIndex == 2 then
            AddBotStatsPriority("medium")
            AddBotStatsTooltipLine("Enhancement can use Strength, but Agility, attack power, hit, and expertise often compete strongly.", 0.84, 0.84, 0.84)
        else
            AddBotStatsPriority("low")
            AddBotStatsTooltipLine("Mostly wasted for this class/spec.", 0.84, 0.84, 0.84)
        end
    elseif key == "agility" then
        if ctx.classToken == "HUNTER" or ctx.classToken == "ROGUE" or (ctx.classToken == "DRUID" and ctx.specIndex == 2) then
            AddBotStatsPriority("high")
            AddBotStatsTooltipLine("Agility is a core physical DPS stat here, adding damage value plus crit/avoidance side benefits.", 0.84, 0.84, 0.84)
        elseif ctx.classToken == "SHAMAN" and ctx.specIndex == 2 then
            AddBotStatsPriority("medium")
            AddBotStatsTooltipLine("Enhancement likes Agility for crit and avoidance, but it is not the only throughput lever.", 0.84, 0.84, 0.84)
        elseif BotStatsIsTank(ctx) then
            AddBotStatsPriority("medium")
            AddBotStatsTooltipLine("Useful avoidance/armor value, but tanks still care first about stamina, defense, armor, and reliable threat.", 0.84, 0.84, 0.84)
        else
            AddBotStatsPriority("low")
            AddBotStatsTooltipLine("Mostly a side-effect stat for this role.", 0.84, 0.84, 0.84)
        end
    elseif key == "stamina" then
        if BotStatsIsTank(ctx) then
            AddBotStatsPriority("high")
            AddBotStatsTooltipLine("Stamina is one of the most reliable survival stats for tanks once crit immunity is covered.", 0.84, 0.84, 0.84)
        else
            AddBotStatsPriority("medium")
            AddBotStatsTooltipLine("Every bot needs enough health to live through damage, but DPS/healers usually gain more from role throughput after that.", 0.84, 0.84, 0.84)
        end
    elseif key == "intellect" then
        if BotStatsIsCaster(ctx) or BotStatsIsHealer(ctx) or ctx.classToken == "HUNTER" or ctx.classToken == "PALADIN" or ctx.classToken == "SHAMAN" or ctx.classToken == "DRUID" then
            AddBotStatsPriority("high")
            AddBotStatsTooltipLine("Intellect improves mana and can add crit/value through class scaling. Important for mana users.", 0.84, 0.84, 0.84)
        else
            AddBotStatsPriority("low")
            AddBotStatsTooltipLine("Low value for rage, energy, and runic-power classes.", 0.84, 0.84, 0.84)
        end
    elseif key == "spirit" then
        if BotStatsIsHealer(ctx) or ctx.classToken == "PRIEST" or ctx.classToken == "MAGE" or ctx.classToken == "WARLOCK" or ctx.classToken == "DRUID" then
            AddBotStatsPriority("medium")
            AddBotStatsTooltipLine("Spirit helps mana sustain and may gain extra value through talents.", 0.84, 0.84, 0.84)
        else
            AddBotStatsPriority("low")
            AddBotStatsTooltipLine("Usually weak for this class/spec unless custom scaling adds value.", 0.84, 0.84, 0.84)
        end
    end
end

local function AddBotHitAdvice(ctx, stats)
    local current = stats and stats.hit or 0
    if BotStatsIsCaster(ctx) then
        AddBotStatsPriority("high")
        AddBotStatsCapStatus("Raid boss spell hit", current, 17, "%")
        AddBotStatsTooltipLine("Caster DPS usually wants 17% spell hit before talent/debuff/racial adjustments.", 0.84, 0.84, 0.84)
    elseif BotStatsIsRangedPhysical(ctx) then
        AddBotStatsPriority("high")
        AddBotStatsCapStatus("Raid boss ranged hit", current, 8, "%")
        AddBotStatsTooltipLine("Hunters normally want 8% ranged hit before bonuses.", 0.84, 0.84, 0.84)
    elseif BotStatsIsMelee(ctx) or BotStatsIsTank(ctx) then
        AddBotStatsPriority("high")
        AddBotStatsCapStatus("Special attack hit", current, 8, "%")
        AddBotStatsTooltipLine("8% covers yellow melee specials against a +3 boss. Dual-wield white swings have a much higher cap.", 0.84, 0.84, 0.84)
    else
        AddBotStatsPriority("low")
        AddBotStatsTooltipLine("Hit value depends on how this custom bot actually attacks.", 0.84, 0.84, 0.84)
    end
end

local function AddBotDefenseAdvice(key, ctx, stats)
    if key == "defense" then
        if BotStatsIsTank(ctx) then AddBotStatsPriority("high") else AddBotStatsPriority("low") end
        AddBotStatsCapStatus("Raid boss crit immunity", stats and stats.def or 0, ctx.bossDefenseCap, " defense")
        AddBotStatsTooltipLine("Crit immunity against a +3 raid boss usually means base defense plus 140.", 0.84, 0.84, 0.84)
    elseif key == "armor" then
        if BotStatsIsTank(ctx) then AddBotStatsPriority("high") else AddBotStatsPriority("medium") end
        AddBotStatsTooltipLine("Armor reduces physical damage. Strong for tanks and still useful against heavy melee damage.", 0.84, 0.84, 0.84)
    elseif key == "dodge" then
        if BotStatsIsTank(ctx) then AddBotStatsPriority("high") else AddBotStatsPriority("medium") end
        AddBotStatsTooltipLine("Dodge avoids a full melee swing. Excellent survival value, but streaky.", 0.84, 0.84, 0.84)
    elseif key == "parry" then
        if BotStatsIsTank(ctx) then AddBotStatsPriority("medium") else AddBotStatsPriority("low") end
        AddBotStatsTooltipLine("Parry avoids an attack and can speed the next swing. Best on weapon/shield tanks that can use it.", 0.84, 0.84, 0.84)
    elseif key == "block" then
        if ctx.classToken == "PALADIN" or ctx.classToken == "WARRIOR" then AddBotStatsPriority("medium") else AddBotStatsPriority("low") end
        AddBotStatsTooltipLine("Block smooths smaller physical hits and matters most for shield tanks.", 0.84, 0.84, 0.84)
    end
end

local function AddBotThroughputAdvice(key, ctx, stats)
    if key == "melee_power" then
        if BotStatsIsMelee(ctx) or BotStatsIsTank(ctx) then AddBotStatsPriority("high") else AddBotStatsPriority("low") end
        AddBotStatsTooltipLine("Attack power feeds melee damage and threat for warriors, rogues, death knights, ret paladins, enhancement shamans, and feral druids.", 0.84, 0.84, 0.84)
    elseif key == "crit" then
        if BotStatsIsMelee(ctx) or BotStatsIsRangedPhysical(ctx) or BotStatsIsCaster(ctx) then
            AddBotStatsPriority("high")
        elseif BotStatsIsHealer(ctx) then
            AddBotStatsPriority("medium")
        else
            AddBotStatsPriority("low")
        end
        AddBotStatsTooltipLine("Crit gains value after hit or sustain needs are handled, especially for specs with proc, burst, or resource talents.", 0.84, 0.84, 0.84)
    elseif key == "haste" then
        if BotStatsIsCaster(ctx) or BotStatsIsHealer(ctx) then
            AddBotStatsPriority("high")
        elseif BotStatsIsMelee(ctx) or BotStatsIsRangedPhysical(ctx) then
            AddBotStatsPriority("medium")
        else
            AddBotStatsPriority("low")
        end
        AddBotStatsTooltipLine("Haste improves casts or swings, but value depends heavily on role and resource limits.", 0.84, 0.84, 0.84)
    elseif key == "spell_damage" then
        if BotStatsIsCaster(ctx) or BotStatsIsHealer(ctx) then AddBotStatsPriority("high") else AddBotStatsPriority("low") end
        AddBotStatsTooltipLine("Spell power drives caster damage and many healing profiles. Holy/prot/ret hybrids only want it when their role actually uses spell scaling.", 0.84, 0.84, 0.84)
    elseif key == "expertise" then
        if BotStatsIsMelee(ctx) or BotStatsIsTank(ctx) then AddBotStatsPriority("high") else AddBotStatsPriority("low") end
        AddBotStatsCapStatus("Dodge soft cap", stats and stats.expertise or 0, 26, " expertise")
        AddBotStatsTooltipLine("26 expertise removes a boss's base dodge chance from the front. Useful for melee DPS and tanks.", 0.84, 0.84, 0.84)
    elseif key == "armor_pen" then
        if BotStatsIsMelee(ctx) or BotStatsIsRangedPhysical(ctx) then AddBotStatsPriority("medium") else AddBotStatsPriority("low") end
        AddBotStatsTooltipLine("Armor penetration helps physical damage against armored targets. It does not help spells or healing.", 0.84, 0.84, 0.84)
    elseif key == "spell_pen" then
        if BotStatsIsCaster(ctx) then AddBotStatsPriority("medium") else AddBotStatsPriority("low") end
        AddBotStatsTooltipLine("Spell penetration counters resistances. It is more PvP/specific-target oriented than general PvE throughput.", 0.84, 0.84, 0.84)
    end
end

local function AddBotStatAdvice(key, ctx, stats)
    if key == "health" then
        if BotStatsIsTank(ctx) then AddBotStatsPriority("high") else AddBotStatsPriority("medium") end
        AddBotStatsTooltipLine("Health is the final buffer after stamina, level, buffs, and custom NPCBot scaling.", 0.84, 0.84, 0.84)
    elseif key == "power" then
        if BotStatsIsHealer(ctx) or BotStatsIsCaster(ctx) then AddBotStatsPriority("high") else AddBotStatsPriority("medium") end
        AddBotStatsTooltipLine("Power shows the bot's active resource pool. Mana users care most for long fights.", 0.84, 0.84, 0.84)
    elseif key == "strength" or key == "agility" or key == "stamina" or key == "intellect" or key == "spirit" then
        AddBotPrimaryStatAdvice(key, ctx)
    elseif key == "hit" then
        AddBotHitAdvice(ctx, stats)
    elseif key == "defense" or key == "armor" or key == "dodge" or key == "parry" or key == "block" then
        AddBotDefenseAdvice(key, ctx, stats)
    elseif key == "holy_res" or key == "fire_res" or key == "nature_res" or key == "frost_res" or key == "shadow_res" or key == "arcane_res" then
        AddBotStatsPriority("medium")
        AddBotStatsTooltipLine("Resistance lowers damage from this magic school. High value on specific encounters, lower value elsewhere.", 0.84, 0.84, 0.84)
    else
        AddBotThroughputAdvice(key, ctx, stats)
    end
end

local function FormatBotStatValue(def, value)
    if value == nil then
        return "-"
    end
    if def.percent then
        return string.format("%.2f%%", tonumber(value) or 0)
    end
    return tostring(math.floor((tonumber(value) or 0) + 0.5))
end

local BotStatSections = {
    {
        title = "生存",
        stats = {
            { key = "hp", label = "生命值", tooltipKey = "health" },
            { key = "mp", label = "能量", tooltipKey = "power" },
        },
    },
    {
        title = "基础属性",
        stats = {
            { key = "str", label = "力量", tooltipKey = "strength" },
            { key = "agi", label = "敏捷", tooltipKey = "agility" },
            { key = "sta", label = "耐力", tooltipKey = "stamina" },
            { key = "int", label = "智力", tooltipKey = "intellect" },
            { key = "spi", label = "精神", tooltipKey = "spirit" },
        },
    },
    {
        title = "进攻",
        stats = {
            { key = "ap", label = "攻击强度", tooltipKey = "melee_power" },
            { key = "sp", label = "法术强度", tooltipKey = "spell_damage" },
            { key = "hit", label = "命中", tooltipKey = "hit", percent = true },
            { key = "crit", label = "暴击", tooltipKey = "crit", percent = true },
            { key = "haste", label = "急速", tooltipKey = "haste", percent = true },
            { key = "expertise", label = "精准", tooltipKey = "expertise" },
            { key = "arpen", label = "护甲穿透", tooltipKey = "armor_pen", percent = true },
            { key = "spellPen", label = "法术穿透", tooltipKey = "spell_pen" },
        },
    },
    {
        title = "防御",
        stats = {
            { key = "armor", label = "护甲", tooltipKey = "armor" },
            { key = "def", label = "防御", tooltipKey = "defense" },
            { key = "dodge", label = "躲闪", tooltipKey = "dodge", percent = true },
            { key = "parry", label = "招架", tooltipKey = "parry", percent = true },
            { key = "block", label = "格挡", tooltipKey = "block", percent = true },
        },
    },
    {
        title = "抗性",
        stats = {
            { key = "resHoly", label = "神圣", tooltipKey = "holy_res" },
            { key = "resFire", label = "火焰", tooltipKey = "fire_res" },
            { key = "resNature", label = "自然", tooltipKey = "nature_res" },
            { key = "resFrost", label = "冰霜", tooltipKey = "frost_res" },
            { key = "resShadow", label = "暗影", tooltipKey = "shadow_res" },
            { key = "resArcane", label = "奥术", tooltipKey = "arcane_res" },
        },
    },
}

local statLines = {}
local statRows = {}
local statHeaders = {}

local function LayoutStatsPanel()
    if not StatsPanel or not StatsContent then return end

    local scrollWidth = StatsScrollFrame:GetWidth() or 210
    local rowWidth = math.max(185, scrollWidth - 8)
    local valueWidth = math.min(86, math.max(66, rowWidth * 0.36))
    local nameWidth = math.max(92, rowWidth - valueWidth - 10)

    StatsContent:SetWidth(rowWidth)
    for _, header in ipairs(statHeaders) do
        header:SetWidth(rowWidth)
    end
    for _, row in ipairs(statRows) do
        row:SetWidth(rowWidth)
        if row.name then
            row.name:SetWidth(nameWidth)
        end
        if row.value then
            row.value:SetWidth(valueWidth)
        end
    end
end

local function ShowBotStatTooltip(frame, entry, def)
    local data = db and db[entry]
    if not data or not def then return end

    local stats = data.stats or {}
    local ctx = GetBotStatsContext(data)
    local value = FormatBotStatValue(def, stats[def.key])

    GameTooltip:SetOwner(frame, "ANCHOR_RIGHT")
    GameTooltip:ClearLines()
    AddBotStatsTooltipLine(def.label, 1.00, 0.82, 0.00)
    AddBotStatsTooltipLine("当前值：" .. value, 1.00, 1.00, 1.00)
    AddBotStatsTooltipLine(ctx.specName .. " " .. ctx.className .. " - " .. ctx.roleLabel, 0.68, 0.85, 1.00)
    AddBotStatsTooltipLine("等级 " .. tostring(ctx.level) .. "；团本首领判定按等级 " .. tostring(ctx.bossLevel) .. " 计算。", 0.62, 0.62, 0.62)
    AddBotStatsTooltipLine(" ")
    AddBotStatAdvice(def.tooltipKey or def.key, ctx, stats)
    AddBotStatsTooltipLine(" ")
    AddBotStatsTooltipLine("以下为根据职业、天赋与职责给出的配装参考，并非精确模拟器结果。", 0.55, 0.55, 0.55)
    GameTooltip:Show()
end

local function CreateStatLine(def, yOffset)
    local frame = CreateFrame("Frame", nil, StatsContent)
    frame:SetSize(205, 16)
    frame:SetPoint("TOPLEFT", 0, yOffset)
    frame:EnableMouse(true)
    frame.statDef = def

    local name = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    name:SetPoint("LEFT", frame, "LEFT", 2, 0)
    name:SetWidth(128)
    name:SetJustifyH("LEFT")
    name:SetText(def.label .. ":")

    local val = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    val:SetPoint("RIGHT", frame, "RIGHT", -2, 0)
    val:SetWidth(74)
    val:SetJustifyH("RIGHT")
    val:SetText("-")

    frame.value = val
    frame.name = name
    frame.key = def.key
    frame:SetScript("OnEnter", function(self)
        ShowBotStatTooltip(self, MainFrame.selectedBot, self.statDef)
    end)
    frame:SetScript("OnLeave", function() GameTooltip:Hide() end)

    statLines[def.key] = { val, def }
    table.insert(statRows, frame)
    return frame
end

local yOffset = -2
for _, section in ipairs(BotStatSections) do
    local header = StatsContent:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    header:SetPoint("TOPLEFT", 0, yOffset)
    header:SetWidth(205)
    header:SetJustifyH("LEFT")
    header:SetText(section.title)
    header:SetTextColor(1, 0.82, 0)
    table.insert(statHeaders, header)
    yOffset = yOffset - 18

    for _, def in ipairs(section.stats) do
        CreateStatLine(def, yOffset)
        yOffset = yOffset - 17
    end
    yOffset = yOffset - 8
end
StatsContent:SetHeight(math.abs(yOffset) + 8)
LayoutStatsPanel()
StatsPanel:HookScript("OnSizeChanged", LayoutStatsPanel)

UpdateStatsPanel = function(entry)
    local data = db[entry]
    if data and db.showStats then
        local ctx = GetBotStatsContext(data)
        StatsPanel:Show()
        statsTitle:SetText(data.name or "机器人属性")
        statsContextText:SetText(GetBotIdentityInlineText and GetBotIdentityInlineText(data) or "")
        statsStatusText:SetText(data.stats and "换装后会刷新最新属性" or "等待服务器发送属性数据")

        local stats = data.stats or {}
        for _, row in ipairs(statRows) do
            local def = row.statDef
            local value = stats[def.key]
            row.value:SetText(FormatBotStatValue(def, value))
            if value == nil then
                row.value:SetTextColor(0.55, 0.55, 0.55)
            else
                row.value:SetTextColor(1, 0.82, 0)
            end
        end
    else
        StatsPanel:Hide()
    end
end


-- [[ 3D MODEL VIEWER & SMART DROP ZONE ]] --
-- PlayerModel + SetUnit(跟随 bot)：3.3.5 下带贴图且种族/装备与游戏内一致。
-- DressUpModel 对 NPC 会白模；SetModel+TryOn 身体无贴图。

local BotModel = CreateFrame("PlayerModel", "BotManager3DModel", GearContainer)
BotModel:SetPoint("TOPLEFT", 60, -(GEAR_HEADER_HEIGHT + 8))
BotModel:SetPoint("BOTTOMRIGHT", -60, 55)
BotModel:EnableMouse(true)
BotModel:EnableMouseWheel(true)
BotModel:SetFrameLevel(GearContainer:GetFrameLevel() + 1)

-- Dark inner border around the model viewport
BotModel:SetBackdrop({
    bgFile = "Interface\\ChatFrame\\ChatFrameBackground",
    edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 12,
    insets = {left = 3, right = 3, top = 3, bottom = 3}
})
BotModel:SetBackdropColor(0.05, 0.05, 0.08, 0.95)
BotModel:SetBackdropBorderColor(0.4, 0.4, 0.4, 0.8)

-- 装备区标题（置于模型框之上，避免被 3D 视口遮挡）
local GearTitleBar = CreateFrame("Frame", nil, GearContainer)
GearTitleBar:SetPoint("TOPLEFT", GearContainer, "TOPLEFT", 52, -6)
GearTitleBar:SetPoint("TOPRIGHT", GearContainer, "TOPRIGHT", -52, -6)
GearTitleBar:SetHeight(22)
GearTitleBar:SetFrameLevel(BotModel:GetFrameLevel() + 5)

GearContainer.title = GearTitleBar:CreateFontString(nil, "OVERLAY", "GameFontNormal")
GearContainer.title:SetPoint("TOPLEFT", GearTitleBar, "TOPLEFT", 0, 0)
GearContainer.title:SetPoint("TOPRIGHT", GearTitleBar, "TOPRIGHT", 0, 0)
GearContainer.title:SetHeight(20)
GearContainer.title:SetJustifyH("CENTER")
GearContainer.title:SetWordWrap(false)
GearContainer.title:SetText("")

function UpdateMainFrameTitle(data)
    if not GearContainer or not GearContainer.title then return end
    if not data or not data.name or data.name == "" then
        GearContainer.title:SetText("")
        return
    end
    local cColor = classColors[data.className] or "FFFFFF"
    local identity = GetBotIdentityInlineText and GetBotIdentityInlineText(data) or ""
    if identity ~= "" then
        GearContainer.title:SetText("|cff" .. cColor .. data.name .. "|r |cffC0C0C0" .. identity .. "|r")
    else
        GearContainer.title:SetText("|cff" .. cColor .. data.name .. "|r")
    end
end

-- Left/Right Navigation Arrows
local PrevBotBtn = CreateFrame("Button", nil, BotModel, "SecureActionButtonTemplate")
PrevBotBtn:SetSize(32, 32)
PrevBotBtn:SetPoint("LEFT", 0, 0)
PrevBotBtn:SetNormalTexture("Interface\\Buttons\\UI-SpellbookIcon-PrevPage-Up")
PrevBotBtn:SetPushedTexture("Interface\\Buttons\\UI-SpellbookIcon-PrevPage-Down")
PrevBotBtn:SetDisabledTexture("Interface\\Buttons\\UI-SpellbookIcon-PrevPage-Disabled")
PrevBotBtn:SetHighlightTexture("Interface\\Buttons\\UI-Common-MouseHilight", "ADD")
PrevBotBtn:SetFrameLevel(BotModel:GetFrameLevel() + 5)

local NextBotBtn = CreateFrame("Button", nil, BotModel, "SecureActionButtonTemplate")
NextBotBtn:SetSize(32, 32)
NextBotBtn:SetPoint("RIGHT", 0, 0)
NextBotBtn:SetNormalTexture("Interface\\Buttons\\UI-SpellbookIcon-NextPage-Up")
NextBotBtn:SetPushedTexture("Interface\\Buttons\\UI-SpellbookIcon-NextPage-Down")
NextBotBtn:SetDisabledTexture("Interface\\Buttons\\UI-SpellbookIcon-NextPage-Disabled")
NextBotBtn:SetHighlightTexture("Interface\\Buttons\\UI-Common-MouseHilight", "ADD")
NextBotBtn:SetFrameLevel(BotModel:GetFrameLevel() + 5)

-- Track model rotation via mouse drag
BotModel.rotation = 0
BotModel.isRotating = false
local ApplyBotModelTransform

-- [[ HIDE HELMET TOGGLE BUTTON ]] --
local HideHelmetBtn = CreateFrame("Button", nil, BotModel)
HideHelmetBtn:SetSize(20, 20)
HideHelmetBtn:SetPoint("BOTTOMRIGHT", -6, 6)
HideHelmetBtn:SetFrameLevel(BotModel:GetFrameLevel() + 5)
HideHelmetBtn:SetNormalTexture("Interface\\Icons\\INV_Helmet_08")
HideHelmetBtn:SetHighlightTexture("Interface\\Buttons\\UI-Common-MouseHilight", "ADD")
HideHelmetBtn:EnableMouse(true)

-- Crossed-out overlay for when helmet is hidden
HideHelmetBtn.disabledOverlay = HideHelmetBtn:CreateTexture(nil, "OVERLAY")
HideHelmetBtn.disabledOverlay:SetAllPoints()
HideHelmetBtn.disabledOverlay:SetTexture("Interface\\RAIDFRAME\\ReadyCheck-NotReady")
HideHelmetBtn.disabledOverlay:SetAlpha(0.85)

function UpdateHideHelmetVisual()
    if db.hideHelmet then
        HideHelmetBtn.disabledOverlay:Show()
        HideHelmetBtn:GetNormalTexture():SetDesaturated(true)
    else
        HideHelmetBtn.disabledOverlay:Hide()
        HideHelmetBtn:GetNormalTexture():SetDesaturated(false)
    end
end


HideHelmetBtn:SetScript("OnClick", function()
    db.hideHelmet = not db.hideHelmet
    UpdateHideHelmetVisual()
    if db.hideHelmet then
        print("|cff00ff00机器人管理:|r 已在 3D 模型中隐藏头盔。")
    else
        print("|cff00ff00机器人管理:|r 已在 3D 模型中显示头盔。")
    end
    -- Re-render current bot model to apply change (preserve camera position)
    if MainFrame.selectedBot then
        local savedZoom = BotModel.zoom
        local savedPanX = BotModel.panX
        local savedPanY = BotModel.panY
        local savedRotation = BotModel.rotation
        UpdateBotModel(MainFrame.selectedBot)
        BotModel.zoom = savedZoom or 0
        BotModel.panX = savedPanX or 0
        BotModel.panY = savedPanY or 0
        BotModel.rotation = savedRotation or 0
        ApplyBotModelTransform(BotModel)
        if BotModel.SetRotation then
            BotModel:SetRotation(BotModel.rotation)
        end
    end
end)

HideHelmetBtn:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_LEFT")
    GameTooltip:SetText("切换头盔显示")
    if db.hideHelmet then
        GameTooltip:AddLine("头盔当前|cffff4444已隐藏|r", 1, 1, 1)
    else
        GameTooltip:AddLine("头盔当前|cff44ff44已显示|r", 1, 1, 1)
    end
    GameTooltip:AddLine("|cff888888点击切换|r", 1, 1, 1)
    GameTooltip:Show()
end)
HideHelmetBtn:SetScript("OnLeave", function() GameTooltip:Hide() end)



-- Helper: Update the 3D model for the selected bot (must be defined before SelectBot calls it)
-- WotLK 3.3.5a DressUpModel slot IDs for UndressSlot()
-- Maps our addon slot keys to the game's internal equipment slot IDs
local SlotToModelSlot = {
    HEAD = 1, NECK = 2, SHOULDER = 3, BACK = 15, CHEST = 5,
    WRIST = 9, HANDS = 10, WAIST = 6, LEGS = 7, FEET = 8,
    FINGER1 = 11, FINGER2 = 12, TRINKET1 = 13, TRINKET2 = 14,
    MAINHAND = 16, OFFHAND = 17, RANGED = 18
}

-- Unified WotLK-compatible playable race display ID mapping.
-- Maps all standard and custom races to base playable character display IDs.
-- This ensures 100% reliable DressUpModel:TryOn() visual gear rendering and avoids white models.
local RaceGenderPlayerDisplayId = {
    -- [raceID] = { [0] = maleDisplayId, [1] = femaleDisplayId }
    [1]  = { [0] = 49,    [1] = 50    }, -- Human
    [2]  = { [0] = 51,    [1] = 52    }, -- Orc
    [3]  = { [0] = 53,    [1] = 54    }, -- Dwarf
    [4]  = { [0] = 55,    [1] = 56    }, -- Night Elf
    [5]  = { [0] = 57,    [1] = 58    }, -- Undead
    [6]  = { [0] = 59,    [1] = 60    }, -- Tauren
    [7]  = { [0] = 1563,  [1] = 1564  }, -- Gnome
    [8]  = { [0] = 1478,  [1] = 1479  }, -- Troll
    [9]  = { [0] = 1563,  [1] = 1564  }, -- Goblin -> Gnome
    [10] = { [0] = 15475, [1] = 15476 }, -- Blood Elf
    [11] = { [0] = 16125, [1] = 16126 }, -- Draenei
    -- Custom races mapped directly to standard player models for 100% TryOn item rendering
    [12] = { [0] = 15475, [1] = 15476 }, -- Void Elf (Blood Elf)
    [13] = { [0] = 1563,  [1] = 1564  }, -- Vulpera (Gnome)
    [14] = { [0] = 15475, [1] = 15476 }, -- High Elf (Blood Elf)
    [15] = { [0] = 53,    [1] = 54    }, -- Pandaren (Dwarf)
    [16] = { [0] = 49,    [1] = 50    }, -- Worgen (Human)
    [17] = { [0] = 16125, [1] = 16126 }, -- Eredar (Draenei)
    [18] = { [0] = 1478,  [1] = 1479  }, -- Zandalari (Troll)
    [19] = { [0] = 16125, [1] = 16126 }, -- Lightforged Draenei (Draenei)
    [20] = { [0] = 55,    [1] = 56    }, -- DH Alliance (Night Elf)
    [21] = { [0] = 15475, [1] = 15476 }, -- DH Horde (Blood Elf)
}

-- 3.3.5 DressUpModel 可用 SetModel 加载玩家种族 mesh（SetDisplayID 不可用，SetUnit(NPC) 会白模）
local RaceGenderModelPath = {
    [1]  = { [0] = "Character\\Human\\Male\\HumanMale.m2",       [1] = "Character\\Human\\Female\\HumanFemale.m2" },
    [2]  = { [0] = "Character\\Orc\\Male\\OrcMale.m2",           [1] = "Character\\Orc\\Female\\OrcFemale.m2" },
    [3]  = { [0] = "Character\\Dwarf\\Male\\DwarfMale.m2",       [1] = "Character\\Dwarf\\Female\\DwarfFemale.m2" },
    [4]  = { [0] = "Character\\NightElf\\Male\\NightElfMale.m2", [1] = "Character\\NightElf\\Female\\NightElfFemale.m2" },
    [5]  = { [0] = "Character\\Scourge\\Male\\ScourgeMale.m2",   [1] = "Character\\Scourge\\Female\\ScourgeFemale.m2" },
    [6]  = { [0] = "Character\\Tauren\\Male\\TaurenMale.m2",     [1] = "Character\\Tauren\\Female\\TaurenFemale.m2" },
    [7]  = { [0] = "Character\\Gnome\\Male\\GnomeMale.m2",       [1] = "Character\\Gnome\\Female\\GnomeFemale.m2" },
    [8]  = { [0] = "Character\\Troll\\Male\\TrollMale.m2",       [1] = "Character\\Troll\\Female\\TrollFemale.m2" },
    [9]  = { [0] = "Character\\Gnome\\Male\\GnomeMale.m2",       [1] = "Character\\Gnome\\Female\\GnomeFemale.m2" },
    [10] = { [0] = "Character\\BloodElf\\Male\\BloodElfMale.m2", [1] = "Character\\BloodElf\\Female\\BloodElfFemale.m2" },
    [11] = { [0] = "Character\\Draenei\\Male\\DraeneiMale.m2",   [1] = "Character\\Draenei\\Female\\DraeneiFemale.m2" },
    [12] = { [0] = "Character\\BloodElf\\Male\\BloodElfMale.m2", [1] = "Character\\BloodElf\\Female\\BloodElfFemale.m2" },
    [13] = { [0] = "Character\\Gnome\\Male\\GnomeMale.m2",       [1] = "Character\\Gnome\\Female\\GnomeFemale.m2" },
    [14] = { [0] = "Character\\BloodElf\\Male\\BloodElfMale.m2", [1] = "Character\\BloodElf\\Female\\BloodElfFemale.m2" },
    [15] = { [0] = "Character\\Dwarf\\Male\\DwarfMale.m2",       [1] = "Character\\Dwarf\\Female\\DwarfFemale.m2" },
    [16] = { [0] = "Character\\Human\\Male\\HumanMale.m2",       [1] = "Character\\Human\\Female\\HumanFemale.m2" },
    [17] = { [0] = "Character\\Draenei\\Male\\DraeneiMale.m2",   [1] = "Character\\Draenei\\Female\\DraeneiFemale.m2" },
    [18] = { [0] = "Character\\Troll\\Male\\TrollMale.m2",       [1] = "Character\\Troll\\Female\\TrollFemale.m2" },
    [19] = { [0] = "Character\\Draenei\\Male\\DraeneiMale.m2",   [1] = "Character\\Draenei\\Female\\DraeneiFemale.m2" },
    [20] = { [0] = "Character\\NightElf\\Male\\NightElfMale.m2", [1] = "Character\\NightElf\\Female\\NightElfFemale.m2" },
    [21] = { [0] = "Character\\BloodElf\\Male\\BloodElfMale.m2", [1] = "Character\\BloodElf\\Female\\BloodElfFemale.m2" },
}


local function SetModelDisplay(displayId)
    local ok = pcall(function() BotModel:SetDisplayID(displayId) end)
    if not ok then
        ok = pcall(function() BotModel:SetDisplayInfo(displayId) end)
    end
    return ok
end

local function ClearBotModel()
    if BotModel.ClearModel then
        pcall(function() BotModel:ClearModel() end)
    end
end

local function SetBotModelUnit(unit)
    ClearBotModel()
    local ok = pcall(function() BotModel:SetUnit(unit) end)
    return ok
end

local function SetBotModelFromRace(race, gender)
    ClearBotModel()
    local paths = RaceGenderModelPath[race]
    if not paths then
        return false
    end
    local path = paths[gender] or paths[0]
    if not path then
        return false
    end
    return pcall(function() BotModel:SetModel(path) end)
end

local function ApplyGearTryOnToModel(data)
    if not data or not data.gear then return end
    pcall(function() BotModel:Undress() end)
    pcall(function() BotModel:UndressSlot(16) end)
    pcall(function() BotModel:UndressSlot(17) end)
    pcall(function() BotModel:UndressSlot(18) end)
    for slotKey, gearData in pairs(data.gear) do
        if gearData and gearData.id and gearData.id > 0 then
            if slotKey == "HEAD" and db.hideHelmet then
                -- skip
            else
                local showItem = true
                if slotKey == "RANGED" then
                    local isArcher = (data.className == "Hunter" or data.className == "Dark Ranger" or data.className == "Sea Witch")
                    if not isArcher then
                        showItem = false
                    end
                end
                if showItem then
                    local itemLink = BuildGearItemLink(gearData) or ("item:" .. gearData.id .. ":0:0:0:0:0:0:0")
                    pcall(function() BotModel:TryOn(itemLink) end)
                end
            end
        end
    end
end

local function SafeSetAttribute(button, name, value)
    if InCombatLockdown() then
        button.pendingAttributes = button.pendingAttributes or {}
        button.pendingAttributes[name] = value
    else
        button:SetAttribute(name, value)
    end
end

local function UpdateNavigationButtons()
    local prevName, nextName
    if MainFrame.sortedBots and MainFrame.selectedBot then
        for i, info in ipairs(MainFrame.sortedBots) do
            if info.entry == MainFrame.selectedBot then
                if i > 1 then
                    prevName = MainFrame.sortedBots[i-1].name
                end
                if i < #MainFrame.sortedBots then
                    nextName = MainFrame.sortedBots[i+1].name
                end
                break
            end
        end
    end
    
    if prevName then
        SafeSetAttribute(PrevBotBtn, "type", "macro")
        SafeSetAttribute(PrevBotBtn, "macrotext", "/target " .. prevName)
        PrevBotBtn:Enable()
    else
        SafeSetAttribute(PrevBotBtn, "type", nil)
        SafeSetAttribute(PrevBotBtn, "macrotext", nil)
        PrevBotBtn:Disable()
    end
    
    if nextName then
        SafeSetAttribute(NextBotBtn, "type", "macro")
        SafeSetAttribute(NextBotBtn, "macrotext", "/target " .. nextName)
        NextBotBtn:Enable()
    else
        SafeSetAttribute(NextBotBtn, "type", nil)
        SafeSetAttribute(NextBotBtn, "macrotext", nil)
        NextBotBtn:Disable()
    end
end

local function FindUnitIdByName(name)
    if not name then return nil end
    if UnitExists("target") and UnitName("target") == name then
        return "target"
    end
    if UnitExists("focus") and UnitName("focus") == name then
        return "focus"
    end
    for i = 1, 4 do
        local unit = "party" .. i
        if UnitExists(unit) and UnitName(unit) == name then
            return unit
        end
    end
    for i = 1, 40 do
        local unit = "raid" .. i
        if UnitExists(unit) and UnitName(unit) == name then
            return unit
        end
    end
    return nil
end

local function GetBotCreatureEntryFromUnit(unit)
    local guid = UnitGUID(unit)
    if not guid then return nil end
    local unitType, _, _, _, _, id = strsplit("-", guid)
    if unitType ~= "Creature" then return nil end
    return tonumber(id)
end

local function FindLiveBotUnit(entry, name)
    if entry then
        for i = 1, 4 do
            local unit = "party" .. i
            if UnitExists(unit) and GetBotCreatureEntryFromUnit(unit) == entry then
                return unit
            end
        end
        for i = 1, 40 do
            local unit = "raid" .. i
            if UnitExists(unit) and GetBotCreatureEntryFromUnit(unit) == entry then
                return unit
            end
        end
        if UnitExists("target") and GetBotCreatureEntryFromUnit("target") == entry then
            return "target"
        end
    end
    return FindUnitIdByName(name)
end

UpdateBotModel = function(entry)
    if not entry then return end
    local data = db[entry]
    if not data then return end
    
    BotModel.rotation = 0.35
    BotModel.zoom = 0
    BotModel.panX = 0
    BotModel.panY = 0.05
    
    local unit = FindLiveBotUnit(entry, data.name)
    local success = false

    if unit then
        success = SetBotModelUnit(unit)
    end
    if not success and UnitExists("target") and GetBotCreatureEntryFromUnit("target") == entry then
        success = SetBotModelUnit("target")
    end
    if not success then
        success = SetBotModelUnit("player")
    end

    if success and unit then
        BotModel.refreshUnit = unit
        BotModel.refreshUntil = GetTime() + 0.35
    else
        BotModel.refreshUnit = nil
    end
    
    BotModel:SetFacing(0.35)
    if BotModel.SetRotation then
        BotModel:SetRotation(BotModel.rotation)
    end
    ApplyBotModelTransform(BotModel)
    
    UpdateMainFrameTitle(data)
end

-- [[ UNIVERSAL SMART DROP ZONE & ROTATION ]] --
local function AddItemToTemplate(templateName, itemID, slotKey, itemLink)
    templatesDB = GetSelectedTemplateBucket(true)
    local t = templatesDB and templatesDB[templateName]
    if not t then return end
    if slotKey == "FINGER1" then
        if GetTemplateItemID(t, "FINGER1") > 0 then
            SetTemplateSlot(t, "FINGER2", itemID, itemLink)
        else
            SetTemplateSlot(t, "FINGER1", itemID, itemLink)
        end
    elseif slotKey == "TRINKET1" then
        if GetTemplateItemID(t, "TRINKET1") > 0 then
            SetTemplateSlot(t, "TRINKET2", itemID, itemLink)
        else
            SetTemplateSlot(t, "TRINKET1", itemID, itemLink)
        end
    elseif slotKey == "MAINHAND" then
        -- Main hand or offhand
        local _, _, _, _, _, _, _, _, itemEquipLoc = GetItemInfo(itemID)
        if itemEquipLoc == "INVTYPE_WEAPON" then
            if GetTemplateItemID(t, "MAINHAND") > 0 then
                SetTemplateSlot(t, "OFFHAND", itemID, itemLink)
            else
                SetTemplateSlot(t, "MAINHAND", itemID, itemLink)
            end
        else
            SetTemplateSlot(t, "MAINHAND", itemID, itemLink)
        end
    else
        SetTemplateSlot(t, slotKey, itemID, itemLink)
    end
    print("|cff00ff00机器人管理:|r 已将物品加入模板「" .. templateName .. "」。")
end

local function EquipItemOnBot(botEntry, itemID, slotKey, itemName)
    local botData = db[botEntry]
    if botData then
        if not botData.gear then botData.gear = {} end
        botData.gear[slotKey] = MakeGearState(itemID)
    end
    if botEntry == MainFrame.selectedBot then
        SelectBot(botEntry)
    end
    SendBMUMessage("EQUIP;" .. botEntry .. ";" .. itemID .. ";" .. slotKey)
    pendingAction = "已在「" .. (botData and botData.name or "机器人") .. "」装备 |cffffd700" .. itemName .. "|r"
end

local function BotModel_HandleDrop(self)
    if MainFrame.inspectMode then return end
    local ok, err = pcall(function()
        local cursorType, id = GetCursorInfo()
        if cursorType == "item" then
            local itemName, itemLink, _, _, itemMinLevel, _, _, _, itemEquipLoc = GetItemInfo(id)
            if itemEquipLoc and itemEquipLoc ~= "" then
                ClearCursor()
                itemName = itemName or ("物品 #" .. id)
                itemLink = itemLink or itemName
                
                local targetSlot = EquipLocToSlot[itemEquipLoc]
                if not targetSlot then
                    print("|cffff0000机器人管理:|r 无法将该物品对应到装备栏位。")
                    return
                end
                
                local function ShowDropMenu()
                    -- Build EasyMenu options dynamically
                    local menuList = {
                        {
                            text = itemLink,
                            isTitle = true,
                            notCheckable = true,
                        }
                    }
                    
                    -- 1. Equip on Current Bot
                    if MainFrame.selectedBot then
                        local botData = db[MainFrame.selectedBot]
                        if botData then
                            local botName = botData.name or "Selected Bot"
                            local canEquip, failReason = CanBotEquipItem(botData.className, id, MainFrame.selectedBot)
                            local optionText = "装备到当前机器人（" .. botName .. "）"
                            
                            if canEquip then
                                local activeSlot = GetTargetSlotForEquipLoc(MainFrame.selectedBot, itemEquipLoc)
                                table.insert(menuList, {
                                    text = optionText,
                                    notCheckable = true,
                                    func = function()
                                        local actions = {}
                                        local finalSlot = activeSlot or targetSlot
                                        local blocked = false
                                        
                                        -- Smart cross-bot unequip check: Skip if we have another copy in our bags!
                                        local availableCount = GetItemCountInBags(id)
                                        if availableCount <= 0 then
                                            for otherEntry, otherBotData in pairs(db) do
                                                if type(otherEntry) == "number" and otherEntry ~= MainFrame.selectedBot then
                                                    if otherBotData.gear then
                                                        for otherSlotKey, otherGearData in pairs(otherBotData.gear) do
                                                            if otherGearData and otherGearData.id == id then
                                                                if IsBotOnline(otherBotData.name) then
                                                                    table.insert(actions, {
                                                                        type = "UNEQUIP",
                                                                        botEntry = otherEntry,
                                                                        botName = otherBotData.name,
                                                                        slotKey = otherSlotKey
                                                                    })
                                                                else
                                                                    print("|cffff0000机器人管理警告:|r 无法从离线机器人「" .. (otherBotData.name or "未知") .. "」自动卸下物品，请先手动卸下。")
                                                                    blocked = true
                                                                end
                                                            end
                                                        end
                                                    end
                                                end
                                            end
                                        end
                                        
                                        if not blocked then
                                            table.insert(actions, {
                                                type = "EQUIP",
                                                botEntry = MainFrame.selectedBot,
                                                botName = botName,
                                                slotKey = finalSlot,
                                                itemID = id,
                                                itemName = itemName
                                            })
                                            
                                            StartSequentialActions(actions, MainFrame.selectedBot, botName, "EQUIP_ITEM")
                                        end
                                    end
                                })
                            else
                                local reasonStr = "无法装备"
                                if failReason == "LEVEL" then
                                    reasonStr = "需要等级 " .. (itemMinLevel or "?")
                                elseif failReason == "CACHE" then
                                    reasonStr = "加载中…"
                                    print("|cff00ff00机器人管理:|r 正在查询物品「" .. itemName .. "」的数据，请稍后再试。")
                                end
                                table.insert(menuList, {
                                    text = optionText .. " - |cffff0000" .. reasonStr .. "|r",
                                    notCheckable = true,
                                    disabled = true
                                })
                            end
                        end
                    else
                        table.insert(menuList, {
                            text = "装备到当前机器人 - |cff808080未选择机器人|r",
                            notCheckable = true,
                            disabled = true
                        })
                    end
                    
                    -- 2. Auto-Route by Role Submenu
                    local autoRouteMenu = {
                        text = "按职责自动分配",
                        hasArrow = true,
                        notCheckable = true,
                        menuList = {}
                    }
                    
                    local rolesList = {
                        { role = "TANK", label = "坦克" },
                        { role = "HEALER", label = "治疗" },
                        { role = "MELEE_DPS", label = "近战输出" },
                        { role = "CASTER_DPS", label = "法系输出" },
                        { role = "RANGED_DPS", label = "远程输出" }
                    }
                    
                    local anyRoleEligible = false
                    for _, rInfo in ipairs(rolesList) do
                        local bestBot = GetBestBotForRole(id, rInfo.role)
                        if bestBot then
                            anyRoleEligible = true
                            local scoreStr = (bestBot.upgrade >= 0) and ("+" .. math.floor(bestBot.upgrade)) or tostring(math.floor(bestBot.upgrade))
                            table.insert(autoRouteMenu.menuList, {
                                text = rInfo.label .. ": " .. bestBot.name .. " (" .. scoreStr .. ")",
                                notCheckable = true,
                                func = function()
                                    local actions = {}
                                    local blocked = false
                                    
                                    -- Smart cross-bot unequip check: Skip if we have another copy in our bags!
                                    local availableCount = GetItemCountInBags(id)
                                    if availableCount <= 0 then
                                        for otherEntry, otherBotData in pairs(db) do
                                            if type(otherEntry) == "number" and otherEntry ~= bestBot.entry then
                                                if otherBotData.gear then
                                                    for otherSlotKey, otherGearData in pairs(otherBotData.gear) do
                                                        if otherGearData and otherGearData.id == id then
                                                            if IsBotOnline(otherBotData.name) then
                                                                table.insert(actions, {
                                                                    type = "UNEQUIP",
                                                                    botEntry = otherEntry,
                                                                    botName = otherBotData.name,
                                                                    slotKey = otherSlotKey
                                                                })
                                                            else
                                                                print("|cffff0000机器人管理警告:|r 无法从离线机器人「" .. (otherBotData.name or "未知") .. "」自动卸下物品，请先手动卸下。")
                                                                blocked = true
                                                            end
                                                        end
                                                    end
                                                end
                                            end
                                        end
                                    end
                                    
                                    if not blocked then
                                        table.insert(actions, {
                                            type = "EQUIP",
                                            botEntry = bestBot.entry,
                                            botName = bestBot.name,
                                            slotKey = bestBot.targetSlot,
                                            itemID = id,
                                            itemName = itemName
                                        })
                                        
                                        StartSequentialActions(actions, bestBot.entry, bestBot.name, "EQUIP_ITEM")
                                    end
                                end
                            })
                        else
                            table.insert(autoRouteMenu.menuList, {
                                text = rInfo.label .. "：无合适机器人",
                                notCheckable = true,
                                disabled = true
                            })
                        end
                    end
                    
                    autoRouteMenu.disabled = not anyRoleEligible
                    table.insert(menuList, autoRouteMenu)
                    
                    -- 3. Add to Template Submenu
                    local addToTemplateMenu = {
                        text = "加入模板",
                        hasArrow = true,
                        notCheckable = true,
                        menuList = {
                            {
                                text = "|cff00ff00创建新模板…|r",
                                notCheckable = true,
                                func = function()
                                    CloseDropDownMenus()
                                    currentTemplateContext.itemID = id
                                    currentTemplateContext.itemLink = itemLink
                                    currentTemplateContext.slotKey = targetSlot
                                    StaticPopup_Show("NPCBOT_CREATE_TEMPLATE")
                                end
                            }
                        }
                    }
                    
                    local activeTemplates = GetSelectedTemplateBucket(false)
                    for tName in pairs(activeTemplates or {}) do
                        local templateName = tName
                        table.insert(addToTemplateMenu.menuList, {
                            text = templateName,
                            notCheckable = true,
                            func = function()
                                AddItemToTemplate(templateName, id, targetSlot, itemLink)
                            end
                        })
                    end
                    table.insert(menuList, addToTemplateMenu)
                    
                    -- 4. Apply Template to Bot Submenu
                    if MainFrame.selectedBot then
                        local selectedBotData = db[MainFrame.selectedBot]
                        local applyTemplateMenu = {
                            text = "将模板应用到机器人",
                            hasArrow = true,
                            notCheckable = true,
                            menuList = {}
                        }
                        local hasTemplates = false
                        local activeTemplates = GetTemplateBucketForBot(selectedBotData, false)
                        for tName in pairs(activeTemplates or {}) do
                            hasTemplates = true
                            local templateName = tName
                            table.insert(applyTemplateMenu.menuList, {
                                    text = templateName,
                                    notCheckable = true,
                                    func = function()
                                        local t = activeTemplates[templateName]
                                        if t then
                                        local botData = db[MainFrame.selectedBot]
                                        local botName = botData and botData.name or "Bot"
                                        local actions = {}
                                        
                                        -- Filter compatibility checks
                                        local validSwaps = {}
                                        for _, slotKey in ipairs(TEMPLATE_SLOT_KEYS) do
                                            local itemState = t[slotKey]
                                            local itemID = GetTemplateItemID(t, slotKey)
                                            if itemID and itemID > 0 then
                                                local currentItem = botData and botData.gear and botData.gear[slotKey] and botData.gear[slotKey].id or 0
                                                if currentItem ~= itemID then
                                                    local canEquip, failReason = CanBotEquipItem(botData.className, itemID, MainFrame.selectedBot)
                                                    if canEquip then
                                                        table.insert(validSwaps, { slotKey = slotKey, itemID = itemID, itemState = itemState })
                                                    elseif failReason == "CACHE" then
                                                        print("|cff00ff00机器人管理:|r 正在查询物品数据库，请稍后再试。")
                                                    end
                                                end
                                            end
                                        end
                                        
                                        -- Count how many copies of each item ID are requested
                                        local neededItemCounts = {}
                                        for _, swap in ipairs(validSwaps) do
                                            neededItemCounts[swap.itemID] = (neededItemCounts[swap.itemID] or 0) + 1
                                        end
                                        
                                        -- Only resolve unequip swaps from online bots if we are short on physical copies!
                                        local blockedSwaps = {}
                                        for itemID, needed in pairs(neededItemCounts) do
                                            local availableCount = GetItemCountInBags(itemID)
                                            if availableCount < needed then
                                                local neededFromBots = needed - availableCount
                                                for otherEntry, otherBotData in pairs(db) do
                                                    if type(otherEntry) == "number" and otherEntry ~= MainFrame.selectedBot then
                                                        if otherBotData.gear then
                                                            for otherSlotKey, otherGearData in pairs(otherBotData.gear) do
                                                                if otherGearData and otherGearData.id == itemID then
                                                                    if IsBotOnline(otherBotData.name) then
                                                                        table.insert(actions, {
                                                                            type = "UNEQUIP",
                                                                            botEntry = otherEntry,
                                                                            botName = otherBotData.name,
                                                                            slotKey = otherSlotKey
                                                                        })
                                                                        neededFromBots = neededFromBots - 1
                                                                        if neededFromBots <= 0 then break end
                                                                    else
                                                                        print("|cffff0000机器人管理警告:|r 无法从离线机器人「" .. (otherBotData.name or "未知") .. "」自动卸下物品，已跳过该交换。")
                                                                        blockedSwaps[itemID] = true
                                                                    end
                                                                end
                                                            end
                                                        end
                                                    end
                                                    if neededFromBots <= 0 then break end
                                                end
                                            end
                                        end
                                        
                                        -- Apply template equipments
                                        for _, swap in ipairs(validSwaps) do
                                            local itemID = swap.itemID
                                            if not blockedSwaps[itemID] then
                                                local slotKey = swap.slotKey
                                                local itemName = GetItemInfo(itemID) or ("物品 #" .. itemID)
                                                table.insert(actions, {
                                                    type = "EQUIP",
                                                    botEntry = MainFrame.selectedBot,
                                                    botName = botName,
                                                    slotKey = slotKey,
                                                    itemID = itemID,
                                                    enchant = swap.itemState and swap.itemState.enchant or 0,
                                                    gems = swap.itemState and swap.itemState.gems or nil,
                                                    itemLink = swap.itemState and swap.itemState.link or nil,
                                                    itemName = itemName
                                                })
                                            end
                                        end
                                        
                                        if #actions > 0 then
                                            StartSequentialActions(actions, MainFrame.selectedBot, botName, "APPLY_TEMPLATE")
                                        else
                                            print("|cff00ff00机器人管理:|r 模板「" .. templateName .. "」中所有可用物品已在「" .. botName .. "」身上装备。")
                                        end
                                        end
                                    end
                                })
                        end
                        if #applyTemplateMenu.menuList > 0 then
                            table.insert(menuList, applyTemplateMenu)
                        elseif hasTemplates then
                            table.insert(menuList, {
                                text = "将模板应用到机器人 - |cff808080无职责模板|r",
                                notCheckable = true,
                                disabled = true
                            })
                        else
                            table.insert(menuList, {
                                text = "将模板应用到机器人 - |cff808080无模板|r",
                                notCheckable = true,
                                disabled = true
                            })
                        end
                    else
                        table.insert(menuList, {
                            text = "将模板应用到机器人 - |cff808080未选择机器人|r",
                            notCheckable = true,
                            disabled = true
                        })
                    end
                    
                    -- 5. Delete Template Submenu
                    local deleteTemplateMenu = {
                        text = "删除模板",
                        hasArrow = true,
                        notCheckable = true,
                        menuList = {}
                    }
                    local hasTemplates = false
                    local activeTemplates = GetSelectedTemplateBucket(false)
                    for tName in pairs(activeTemplates or {}) do
                        hasTemplates = true
                        local templateName = tName
                        table.insert(deleteTemplateMenu.menuList, {
                            text = templateName,
                            notCheckable = true,
                            func = function()
                                activeTemplates[templateName] = nil
                                print("|cff00ff00机器人管理:|r 已删除模板「" .. templateName .. "」。")
                                -- Re-open the menu to show the updated list instantly!
                                local f = CreateFrame("Frame")
                                f:SetScript("OnUpdate", function(selfFrame)
                                    CloseDropDownMenus()
                                    ShowDropMenu()
                                    selfFrame:SetScript("OnUpdate", nil)
                                end)
                            end
                        })
                    end
                    if hasTemplates then
                        table.insert(menuList, deleteTemplateMenu)
                    else
                        table.insert(menuList, {
                            text = "删除模板 - |cff808080无模板|r",
                            notCheckable = true,
                            disabled = true
                        })
                    end
                    
                    -- Show EasyMenu at the cursor
                    EasyMenu(menuList, menuFrame, "cursor", 0, 0, "MENU")
                end
                
                -- Show the initial drop context menu
                ShowDropMenu()
            else
                print("|cffff0000机器人管理:|r 无法装备该物品。")
                ClearCursor()
            end
        end
    end)
    if not ok then
        print("|cffff0000机器人管理错误:|r " .. tostring(err))
    end
end

BotModel:EnableMouse(true)
BotModel:RegisterForDrag("LeftButton", "RightButton")
BotModel:EnableMouseWheel(true)
BotModel.zoom = 0
BotModel.panX = 0
BotModel.panY = 0

local function MakeFrameAcceptDrop(frame)
    if not frame then return end
    frame:EnableMouse(true)
    pcall(function() frame:RegisterForDrag("LeftButton") end)
    pcall(function()
        frame:HookScript("OnReceiveDrag", BotModel_HandleDrop)
    end)
    pcall(function()
        frame:HookScript("OnMouseUp", function(self, button)
            if button == "LeftButton" and CursorHasItem() then
                BotModel_HandleDrop(self)
            end
        end)
    end)
end

local function BotButton_HandleDrop(self)
    if self.botEntry then
        SelectBot(self.botEntry)
        BotModel_HandleDrop(self)
    end
end

local function SetupBotListButton(b)
    if not b then return end
    b:EnableMouse(true)
    pcall(function() b:RegisterForDrag("LeftButton") end)
    pcall(function()
        b:HookScript("OnReceiveDrag", BotButton_HandleDrop)
    end)
    pcall(function()
        b:HookScript("OnMouseUp", function(self, button)
            if button == "LeftButton" and CursorHasItem() then
                BotButton_HandleDrop(self)
            end
        end)
    end)
    pcall(function()
        b:HookScript("PostClick", function(self)
            if CursorHasItem() then
                BotModel_HandleDrop(self)
            end
        end)
    end)
end

-- Apply drag-and-drop auto-equip to all main panels and containers
MakeFrameAcceptDrop(MainFrame)
MakeFrameAcceptDrop(Sidebar)
MakeFrameAcceptDrop(SidebarTools)
MakeFrameAcceptDrop(GearContainer)
MakeFrameAcceptDrop(StatsPanel)
MakeFrameAcceptDrop(ScrollFrame)
MakeFrameAcceptDrop(ScrollContent)
MakeFrameAcceptDrop(searchBox)
MakeFrameAcceptDrop(sortBtn)
MakeFrameAcceptDrop(syncBtn)
MakeFrameAcceptDrop(statsToggleBtn)
MakeFrameAcceptDrop(editorBtn)
MakeFrameAcceptDrop(templatesBtn)

-- Apply SetupBotListButton to all pre-created buttons in the pool
for _, b in ipairs(BotButtonsPool) do
    SetupBotListButton(b)
end

-- Hook Prev/Next buttons to auto-equip when clicked while holding an item
PrevBotBtn:HookScript("PostClick", function()
    if CursorHasItem() then
        BotModel_HandleDrop(PrevBotBtn)
    end
end)
NextBotBtn:HookScript("PostClick", function()
    if CursorHasItem() then
        BotModel_HandleDrop(NextBotBtn)
    end
end)

BotModel:SetScript("OnReceiveDrag", BotModel_HandleDrop)

BotModel:SetScript("OnMouseDown", function(self, button)
    if button == "LeftButton" then
        if CursorHasItem() then
            return
        end
        self.isRotating = true
        self.cursorStartX = GetCursorPosition()
        self.rotationStart = self.rotation or 0
    elseif button == "RightButton" then
        -- Start panning
        self.isPanning = true
        local cx, cy = GetCursorPosition()
        local scale = UIParent:GetEffectiveScale()
        self.panStartCursorX = cx / scale
        self.panStartCursorY = cy / scale
        self.panStartX = self.panX or 0
        self.panStartY = self.panY or 0
    end
end)

BotModel:SetScript("OnMouseUp", function(self, button)
    if button == "LeftButton" then
        self.isRotating = false
        if CursorHasItem() then
            BotModel_HandleDrop(self)
        end
    elseif button == "RightButton" then
        self.isPanning = false
    end
end)

local function StopModelInteraction()
    BotModel.isRotating = false
    BotModel.isPanning = false
end
MainFrame:HookScript("OnMouseUp", StopModelInteraction)
GearContainer:HookScript("OnMouseUp", StopModelInteraction)

-- Pan limits in model-space units (how far the model can be dragged)
local PAN_LIMIT_X = 2.0
local PAN_LIMIT_Y = 1.75
local ZOOM_MIN = -1.5
local ZOOM_MAX = 2.5
local ZOOM_STEP = 0.25
local ZOOM_FOCUS_PAN_FACTOR = 1.85
local PAN_DRAG_SPEED = 0.0045
local PAN_DRAG_MIN_SPEED = 0.0025
local PAN_DRAG_ZOOM_SLOWDOWN = 0.00045
local CAMERA_DISTANCE_BASE = 1.0
local CAMERA_DISTANCE_PER_ZOOM = 0.18
local CAMERA_DISTANCE_MIN = 0.45
local CAMERA_DISTANCE_MAX = 1.6
-- PlayerModel SetUnit：depth 负值=拉远，正值=拉近（过近会只剩一张脸）
local MODEL_VIEW_Z_BASE = -1.35
local MODEL_VIEW_Z_STEP = 0.22

local function ClampValue(value, minValue, maxValue)
    if value < minValue then return minValue end
    if value > maxValue then return maxValue end
    return value
end

ApplyBotModelTransform = function(model)
    if not model then return end
    local zoom = model.zoom or 0
    local panX = model.panX or 0
    local panY = model.panY or 0

    -- PlayerModel + SetUnit：用 SetPosition 控制远近（3.3.5 最可靠）
    local depth = MODEL_VIEW_Z_BASE - (zoom * MODEL_VIEW_Z_STEP)
    pcall(model.SetPosition, model, depth, panX, panY * 0.35)

    if model.SetCamDistanceScale then
        local distanceScale = CAMERA_DISTANCE_BASE - (zoom * CAMERA_DISTANCE_PER_ZOOM)
        distanceScale = ClampValue(distanceScale, CAMERA_DISTANCE_MIN, CAMERA_DISTANCE_MAX)
        pcall(model.SetCamDistanceScale, model, distanceScale)
    end
end

local function GetModelPanDragSpeed(model)
    local zoom = math.max(0, model and model.zoom or 0)
    return math.max(PAN_DRAG_MIN_SPEED, PAN_DRAG_SPEED - (zoom * PAN_DRAG_ZOOM_SLOWDOWN))
end

local function GetCursorPositionInFrame(targetFrame)
    if not targetFrame then return nil, nil, false end
    local scale = UIParent:GetEffectiveScale() or 1
    local cx, cy = GetCursorPosition()
    cx = cx / scale
    cy = cy / scale
    local left = targetFrame:GetLeft() or 0
    local right = targetFrame:GetRight() or 0
    local bottom = targetFrame:GetBottom() or 0
    local top = targetFrame:GetTop() or 0
    local width = right - left
    local height = top - bottom
    if width <= 0 or height <= 0 then return nil, nil, false end

    local normX = (cx - left) / width
    local normY = (cy - bottom) / height
    local inside = normX >= 0 and normX <= 1 and normY >= 0 and normY <= 1
    return ClampValue(normX, 0, 1), ClampValue(normY, 0, 1), inside
end

local function ZoomBotModelAtCursor(delta)
    local normX, normY, inside = GetCursorPositionInFrame(BotModel)
    if not inside then return end

    -- Center-relative: -0.5 to 0.5
    local relX = normX - 0.5
    local relY = normY - 0.5
    
    local oldZoom = BotModel.zoom or 0
    local newZoom = oldZoom + (delta * ZOOM_STEP)
    -- Clamp zoom
    newZoom = ClampValue(newZoom, ZOOM_MIN, ZOOM_MAX)
    
    -- Adjust pan so the point under the cursor stays fixed
    -- The zoom factor change causes a visual shift; compensate with pan
    local zoomDelta = newZoom - oldZoom
    local panX = (BotModel.panX or 0) - relX * zoomDelta * ZOOM_FOCUS_PAN_FACTOR
    local panY = (BotModel.panY or 0) + relY * zoomDelta * ZOOM_FOCUS_PAN_FACTOR
    
    -- Clamp pan within limits
    panX = ClampValue(panX, -PAN_LIMIT_X, PAN_LIMIT_X)
    panY = ClampValue(panY, -PAN_LIMIT_Y, PAN_LIMIT_Y)
    
    BotModel.zoom = newZoom
    BotModel.panX = panX
    BotModel.panY = panY
    ApplyBotModelTransform(BotModel)
end

BotModel:SetScript("OnMouseWheel", function(self, delta)
    ZoomBotModelAtCursor(delta)
end)

GearContainer:EnableMouseWheel(true)
GearContainer:HookScript("OnMouseWheel", function(self, delta)
    ZoomBotModelAtCursor(delta)
end)

BotModel:SetScript("OnUpdate", function(self, elapsed)
    if self.refreshUnit and GetTime() < (self.refreshUntil or 0) then
        SetBotModelUnit(self.refreshUnit)
        ApplyBotModelTransform(self)
        if self.SetRotation then
            self:SetRotation(self.rotation or 0.35)
        end
    elseif self.refreshUnit then
        self.refreshUnit = nil
        self.refreshUntil = nil
    end
    -- Handle left-click rotation
    if self.isRotating then
        if IsMouseButtonDown and not IsMouseButtonDown("LeftButton") then
            self.isRotating = false
        else
            local cursorX = GetCursorPosition()
            local diff = (cursorX - self.cursorStartX) * 0.015
            self.rotation = self.rotationStart + diff
            self:SetRotation(self.rotation)
        end
    end
    -- Handle right-click panning
    if self.isPanning then
        if IsMouseButtonDown and not IsMouseButtonDown("RightButton") then
            self.isPanning = false
        else
            local cx, cy = GetCursorPosition()
            local scale = UIParent:GetEffectiveScale()
            cx = cx / scale
            cy = cy / scale
            local panDragSpeed = GetModelPanDragSpeed(self)
            local dx = (cx - self.panStartCursorX) * panDragSpeed
            local dy = (cy - self.panStartCursorY) * panDragSpeed
            local newPanX = self.panStartX + dx
            local newPanY = self.panStartY + dy
            -- Clamp within limits
            newPanX = ClampValue(newPanX, -PAN_LIMIT_X, PAN_LIMIT_X)
            newPanY = ClampValue(newPanY, -PAN_LIMIT_Y, PAN_LIMIT_Y)
            self.panX = newPanX
            self.panY = newPanY
            ApplyBotModelTransform(self)
        end
    end
end)

-- Armor subclass proficiency per class (0=Misc, 1=Cloth, 2=Leather, 3=Mail, 4=Plate, 6=Shield)
ArmorProficiency = {
    ["Warrior"] = {[0]=true,[1]=true,[2]=true,[3]=true,[4]=true,[6]=true},
    ["Paladin"] = {[0]=true,[1]=true,[2]=true,[3]=true,[4]=true,[6]=true,[7]=true},
    ["Death Knight"] = {[0]=true,[1]=true,[2]=true,[3]=true,[4]=true,[10]=true},
    ["Hunter"] = {[0]=true,[1]=true,[2]=true,[3]=true},
    ["Shaman"] = {[0]=true,[1]=true,[2]=true,[3]=true,[6]=true,[9]=true},
    ["Rogue"] = {[0]=true,[1]=true,[2]=true},
    ["Druid"] = {[0]=true,[1]=true,[2]=true,[8]=true},
    ["Priest"] = {[0]=true,[1]=true},
    ["Mage"] = {[0]=true,[1]=true},
    ["Warlock"] = {[0]=true,[1]=true},
    ["Blademaster"] = {[0]=true,[1]=true,[2]=true,[3]=true,[4]=true},
    ["Sphynx"] = {[0]=true,[3]=true,[4]=true},
    ["Archmage"] = {[0]=true,[1]=true},
    ["Dreadlord"] = {[0]=true,[4]=true},
    ["Spellbreaker"] = {[0]=true,[3]=true,[4]=true,[6]=true},
    ["Dark Ranger"] = {[0]=true,[1]=true,[2]=true},
    ["Necromancer"] = {[0]=true,[1]=true},
    ["Sea Witch"] = {[0]=true,[1]=true},
    ["Crypt Lord"] = {[0]=true,[3]=true,[4]=true}
}

-- Armor subtype string to subclass ID mapping (3.3.5 GetItemInfo returns localized strings)
ArmorSubtypeToID = {
    ["Miscellaneous"] = 0, ["Cloth"] = 1, ["Leather"] = 2, ["Mail"] = 3, ["Plate"] = 4,
    ["Shields"] = 6, ["Librams"] = 7, ["Idols"] = 8, ["Totems"] = 9, ["Sigils"] = 10
}

-- Weapon subclass proficiency per class (0=One-Handed Axes, 1=Two-Handed Axes, 2=Bows, 3=Guns, 4=One-Handed Maces, 5=Two-Handed Maces, 6=Polearms, 7=One-Handed Swords, 8=Two-Handed Swords, 10=Staves, 13=Fist Weapons, 15=Daggers, 16=Thrown, 18=Crossbows, 19=Wands, 20=Fishing Poles)
WeaponProficiency = {
    ["Warrior"] = {[0]=true,[1]=true,[2]=true,[3]=true,[4]=true,[5]=true,[6]=true,[7]=true,[8]=true,[10]=true,[13]=true,[15]=true,[16]=true,[18]=true,[20]=true},
    ["Paladin"] = {[0]=true,[1]=true,[4]=true,[5]=true,[6]=true,[7]=true,[8]=true,[20]=true},
    ["Death Knight"] = {[0]=true,[1]=true,[4]=true,[5]=true,[6]=true,[7]=true,[8]=true,[20]=true},
    ["Hunter"] = {[0]=true,[1]=true,[2]=true,[3]=true,[4]=true,[5]=true,[6]=true,[7]=true,[8]=true,[10]=true,[13]=true,[15]=true,[16]=true,[18]=true,[20]=true},
    ["Shaman"] = {[0]=true,[1]=true,[4]=true,[5]=true,[10]=true,[13]=true,[15]=true,[20]=true},
    ["Rogue"] = {[0]=true,[2]=true,[3]=true,[4]=true,[7]=true,[13]=true,[15]=true,[16]=true,[18]=true,[20]=true},
    ["Druid"] = {[4]=true,[5]=true,[6]=true,[10]=true,[15]=true,[20]=true},
    ["Priest"] = {[4]=true,[10]=true,[15]=true,[19]=true,[20]=true},
    ["Mage"] = {[7]=true,[10]=true,[15]=true,[19]=true,[20]=true},
    ["Warlock"] = {[7]=true,[10]=true,[15]=true,[19]=true,[20]=true},
    -- Custom classes
    ["Blademaster"] = {[0]=true,[1]=true,[4]=true,[5]=true,[6]=true,[7]=true,[8]=true,[10]=true,[13]=true,[15]=true,[20]=true},
    ["Sphynx"] = {[10]=true,[15]=true,[19]=true,[20]=true},
    ["Archmage"] = {[7]=true,[10]=true,[15]=true,[19]=true,[20]=true},
    ["Dreadlord"] = {[0]=true,[1]=true,[4]=true,[5]=true,[7]=true,[8]=true,[20]=true},
    ["Spellbreaker"] = {[6]=true,[7]=true,[8]=true,[15]=true,[20]=true},
    ["Dark Ranger"] = {[2]=true,[7]=true,[15]=true,[18]=true,[20]=true},
    ["Necromancer"] = {[10]=true,[15]=true,[19]=true,[20]=true},
    ["Sea Witch"] = {[2]=true,[6]=true,[15]=true,[18]=true,[20]=true},
    ["Crypt Lord"] = {[0]=true,[1]=true,[4]=true,[5]=true,[6]=true,[7]=true,[8]=true,[20]=true}
}

-- Weapon subtype string to subclass ID mapping (3.3.5 GetItemInfo returns localized strings)
WeaponSubtypeToID = {
    ["One-Handed Axes"] = 0, ["Two-Handed Axes"] = 1, ["Bows"] = 2, ["Guns"] = 3,
    ["One-Handed Maces"] = 4, ["Two-Handed Maces"] = 5, ["Polearms"] = 6,
    ["One-Handed Swords"] = 7, ["Two-Handed Swords"] = 8, ["Staves"] = 10,
    ["Fist Weapons"] = 13, ["Daggers"] = 15, ["Thrown"] = 16, ["Crossbows"] = 18,
    ["Wands"] = 19, ["Fishing Poles"] = 20
}

-- Check if a bot class can use an item based on its item subclass and type
-- Check if a bot class can use an item based on its item subclass, type, and class restrictions
CanBotEquipItem = function(botClassName, itemID, botEntry)
    if not botClassName or not itemID then return true end
    local _, itemLink, _, _, itemMinLevel, itemType, itemSubType, _, itemEquipLoc = GetItemInfo(itemID)
    
    -- Cache Guard: If the item is not loaded into the local client cache, block prediction to prevent class check bypasses
    if not itemType or not itemLink then
        BMUScannerTooltip:ClearLines()
        BMUScannerTooltip:SetOwner(WorldFrame, "ANCHOR_NONE")
        BMUScannerTooltip:SetHyperlink("item:"..itemID..":0:0:0:0:0:0:0")
        return false, "CACHE"
    end

    -- Check item level requirement against bot level
    if botEntry and itemMinLevel and itemMinLevel > 0 then
        local botData = db[botEntry]
        if botData and botData.level and botData.level > 0 and botData.level < itemMinLevel then
            return false, "LEVEL"
        end
    end

    -- Custom classes parent mapping for client-side tooltip class scan checks
    local CustomClassParentMap = {
        ["Necromancer"] = "Warlock",
        ["Sea Witch"] = "Shaman", -- and/or Hunter
        ["Blademaster"] = "Warrior",
        ["Sphynx"] = "Mage",
        ["Archmage"] = "Mage",
        ["Dreadlord"] = "Death Knight",
        ["Spellbreaker"] = "Warrior", -- or Paladin
        ["Crypt Lord"] = "Death Knight",
        ["Dark Ranger"] = "Hunter"
    }

    -- Check for class-restricted items (e.g. Tier Sets)
    BMUScannerTooltip:ClearLines()
    BMUScannerTooltip:SetOwner(WorldFrame, "ANCHOR_NONE")
    BMUScannerTooltip:SetHyperlink(itemLink)
    
    for i = 2, BMUScannerTooltip:NumLines() do
        local line = _G["BMUScannerTooltipTextLeft" .. i]
        if line then
            local text = line:GetText()
            if text and text:find("Classes:") then
                local parentClass = CustomClassParentMap[botClassName]
                if not text:find(botClassName) and (not parentClass or not text:find(parentClass)) then
                    return false -- Item is restricted to another class
                end
            end
        end
    end

    -- Armor check
    if itemType == "Armor" then
        local subClassID = ArmorSubtypeToID[itemSubType]
        if subClassID then
            local prof = ArmorProficiency[botClassName]
            if prof and not prof[subClassID] then
                return false
            end
        end
    elseif itemType == "Weapon" then
        local subClassID = WeaponSubtypeToID[itemSubType]
        if subClassID then
            local prof = WeaponProficiency[botClassName]
            if prof and not prof[subClassID] then
                return false
            end
        end
    end

    return true -- let server handle additional class mask checks
end

-- [[ CLASS & ROLE AWARE STAT SCORING ]] --
-- Bot roles in the NPCBots C++ source:
-- tank(1), off-tank(2), dps(4), heal(8), ranged(16)
local BOT_ROLE_TANK = 1
local BOT_ROLE_TANK_OFF = 2
local BOT_ROLE_DPS = 4
local BOT_ROLE_HEAL = 8
local BOT_ROLE_RANGED = 16

local PhysicalRangedDpsClasses = {
    ["Hunter"] = true,
    ["Dark Ranger"] = true
}

local CasterDpsRoleClasses = {
    ["Mage"] = true, ["Warlock"] = true, ["Priest"] = true,
    ["Druid"] = true, ["Shaman"] = true, ["Necromancer"] = true,
    ["Archmage"] = true, ["Sea Witch"] = true, ["Sphynx"] = true
}

local SpecRoleCategory = {
    [1] = "MELEE_DPS", -- Warrior Arms
    [2] = "MELEE_DPS", -- Warrior Fury
    [3] = "TANK",      -- Warrior Protection
    [4] = "HEALER",    -- Paladin Holy
    [5] = "TANK",      -- Paladin Protection
    [6] = "MELEE_DPS", -- Paladin Retribution
    [7] = "RANGED_DPS", [8] = "RANGED_DPS", [9] = "RANGED_DPS",
    [10] = "MELEE_DPS", [11] = "MELEE_DPS", [12] = "MELEE_DPS",
    [13] = "HEALER", [14] = "HEALER", [15] = "CASTER_DPS",
    [19] = "CASTER_DPS", [20] = "MELEE_DPS", [21] = "HEALER",
    [22] = "CASTER_DPS", [23] = "CASTER_DPS", [24] = "CASTER_DPS",
    [25] = "CASTER_DPS", [26] = "CASTER_DPS", [27] = "CASTER_DPS",
    [28] = "CASTER_DPS", [30] = "HEALER"
}

local RoleSensitiveTankSpecs = {
    [16] = true, [17] = true, [18] = true, -- Death Knight Blood/Frost/Unholy
    [29] = true -- Druid Feral
}

function DecodeBotRoles(roles)
    roles = tonumber(roles) or 0
    local isTank = bit.band(roles, BOT_ROLE_TANK) > 0 or bit.band(roles, BOT_ROLE_TANK_OFF) > 0
    local isHealer = bit.band(roles, BOT_ROLE_HEAL) > 0
    local isRanged = bit.band(roles, BOT_ROLE_RANGED) > 0
    local isDPS = bit.band(roles, BOT_ROLE_DPS) > 0

    if not isTank and not isHealer and not isRanged and not isDPS then
        isDPS = true
    end

    return isTank, isHealer, isRanged, isDPS
end

local function GetFallbackRoleCategory(className, roles)
    local isTank, isHealer, isRanged, isDPS = DecodeBotRoles(roles)
    if isTank then return "TANK" end
    if isHealer then return "HEALER" end
    if not isDPS then return nil end

    if PhysicalRangedDpsClasses[className] then
        return "RANGED_DPS"
    elseif isRanged and CasterDpsRoleClasses[className] then
        return "CASTER_DPS"
    elseif isRanged then
        return "RANGED_DPS"
    elseif CasterDpsRoleClasses[className] and className ~= "Druid" and className ~= "Shaman" then
        return "CASTER_DPS"
    end

    return "MELEE_DPS"
end

GetBotRoleCategory = function(botDataOrClassName, roles, spec)
    local className = botDataOrClassName
    if type(botDataOrClassName) == "table" then
        className = botDataOrClassName.className
        roles = botDataOrClassName.roles
        spec = botDataOrClassName.spec
    end

    spec = tonumber(spec) or 0
    if RoleSensitiveTankSpecs[spec] then
        local isTank = DecodeBotRoles(roles)
        if isTank then return "TANK" end
        return "MELEE_DPS"
    end

    if SpecRoleCategory[spec] then
        return SpecRoleCategory[spec]
    end

    return GetFallbackRoleCategory(className, roles)
end

function GetRoleMaskForCategory(categoryKey)
    if categoryKey == "TANK" then
        return BOT_ROLE_TANK
    elseif categoryKey == "HEALER" then
        return BOT_ROLE_HEAL
    elseif categoryKey == "RANGED_DPS" or categoryKey == "CASTER_DPS" then
        return BOT_ROLE_DPS + BOT_ROLE_RANGED
    elseif categoryKey == "MELEE_DPS" then
        return BOT_ROLE_DPS
    end
    return nil
end

local function RoleCategoryToScoreRoleType(categoryKey)
    if categoryKey == "TANK" then
        return "TANK"
    elseif categoryKey == "HEALER" then
        return "HEALER"
    elseif categoryKey == "RANGED_DPS" or categoryKey == "CASTER_DPS" then
        return "RANGED_DPS"
    elseif categoryKey == "MELEE_DPS" then
        return "MELEE_DPS"
    end
    return nil
end

-- Cache for item stat scoring to prevent heavy frame-hitching with large bot lists
local itemScoreCache = {}

function ScoreItemForClass(itemID, className, roles, spec)
    if not itemID or not className then return 0, false end
    roles = roles or 0
    spec = spec or 0
    
    local cacheKey = itemID .. "_" .. className .. "_" .. roles .. "_" .. spec
    if itemScoreCache[cacheKey] then
        return itemScoreCache[cacheKey].score, itemScoreCache[cacheKey].relevant
    end
    
    local isTank, isHealer, isRanged, isDPS = DecodeBotRoles(roles)
    local resolvedRoleCategory = GetBotRoleCategory(className, roles, spec)

    local function getWeightsForRole(roleType)
        local weights = {}
        local primaryStats = {}
        local function markPrimary(...)
            for i = 1, select("#", ...) do
                primaryStats[select(i, ...)] = true
            end
        end

        if roleType == "TANK" then
            weights["stamina"] = 3
            weights["defense"] = 2.5
            weights["dodge"] = 2
            weights["parry"] = 2
            weights["block"] = 1.5
            weights["armor"] = 1
            weights["expertise"] = 1.5
            weights["hit rating"] = 1
            if className == "Paladin" then
                weights["strength"] = 1.5
                weights["spell power"] = 1
            elseif className == "Death Knight" then
                weights["strength"] = 1.5
            elseif className == "Druid" then
                weights["agility"] = 2
            else -- Warrior / default tank
                weights["strength"] = 1.5
                weights["agility"] = 1
            end
            markPrimary("stamina", "defense", "dodge", "parry", "block")
        elseif roleType == "HEALER" then
            weights["intellect"] = 2.5
            weights["spell power"] = 2.5
            weights["mana per 5"] = 2
            weights["spirit"] = 2
            weights["haste"] = 1.5
            weights["critical strike"] = 1
            weights["stamina"] = 0.5
            markPrimary("intellect", "spell power", "mana per 5", "spirit")
        elseif roleType == "RANGED_DPS" then
            if className == "Hunter" or className == "Dark Ranger" then
                weights["agility"] = 2.5
                weights["attack power"] = 2
                weights["armor penetration"] = 2
                weights["critical strike"] = 1.5
                weights["hit rating"] = 1.5
                weights["haste"] = 1
                weights["stamina"] = 0.5
                weights["intellect"] = 0.5
                markPrimary("agility", "attack power", "armor penetration", "hit rating")
            else -- Caster DPS
                weights["intellect"] = 2.5
                weights["spell power"] = 2.5
                weights["hit rating"] = 2
                weights["haste"] = 2
                weights["critical strike"] = 1.5
                weights["spirit"] = 1
                weights["stamina"] = 0.5
                markPrimary("intellect", "spell power", "hit rating")
            end
        elseif roleType == "MELEE_DPS" then
            if className == "Rogue" or className == "Druid" or className == "Shaman" then
                weights["agility"] = 2.5
                weights["attack power"] = 2
            else
                weights["strength"] = 2.5
                weights["attack power"] = 2
                if className == "Paladin" then weights["spell power"] = 1 end
            end
            weights["armor penetration"] = 2
            weights["expertise"] = 2
            weights["hit rating"] = 1.5
            weights["critical strike"] = 1.5
            weights["haste"] = 1
            weights["stamina"] = 0.8
            markPrimary("strength", "agility", "attack power", "armor penetration", "expertise", "hit rating")
        end
        return weights, primaryStats
    end

    local activeRoleTypes = {}
    local resolvedScoreRoleType = RoleCategoryToScoreRoleType(resolvedRoleCategory)
    if resolvedScoreRoleType then
        table.insert(activeRoleTypes, resolvedScoreRoleType)
    else
        if isTank then table.insert(activeRoleTypes, "TANK") end
        if isHealer then table.insert(activeRoleTypes, "HEALER") end
        if isRanged and isDPS then table.insert(activeRoleTypes, "RANGED_DPS") end
        if isDPS and not isRanged then 
            if className == "Hunter" or className == "Mage" or className == "Warlock" or className == "Priest" or className == "Necromancer" or className == "Archmage" or className == "Dark Ranger" then
                table.insert(activeRoleTypes, "RANGED_DPS") -- Auto fix for casters that only have DPS bit but no Ranged bit
            else
                table.insert(activeRoleTypes, "MELEE_DPS") 
            end
        end
    end

    local _, itemLink = GetItemInfo(itemID)
    if not itemLink then return 0, true end -- not cached

    BMUScannerTooltip:ClearLines()
    BMUScannerTooltip:SetOwner(WorldFrame, "ANCHOR_NONE")
    BMUScannerTooltip:SetHyperlink(itemLink)

    local parsedItemStats = {}
    for i = 2, BMUScannerTooltip:NumLines() do
        local line = _G["BMUScannerTooltipTextLeft" .. i]
        if line then
            local text = line:GetText()
            if text then
                table.insert(parsedItemStats, text:lower())
            end
        end
    end

    local maxScore = 0
    local anyRelevantAcrossAllRoles = false

    for _, roleType in ipairs(activeRoleTypes) do
        local roleWeights, rolePrimaryStats = getWeightsForRole(roleType)
        local currentRoleScore = 0
        local currentRoleRelevant = false
        local currentRolePrimaryRelevant = false

        for _, lowerText in ipairs(parsedItemStats) do
            for statKey, weight in pairs(roleWeights) do
                if lowerText:find(statKey:lower()) then
                    local value = tonumber(lowerText:match("(%d+)")) or 10
                    currentRoleScore = currentRoleScore + (value * weight)
                    currentRoleRelevant = true
                    if rolePrimaryStats[statKey] then
                        currentRolePrimaryRelevant = true
                    end
                end
            end
        end

        if currentRoleRelevant and currentRolePrimaryRelevant and currentRoleScore >= maxScore then
            maxScore = currentRoleScore
            anyRelevantAcrossAllRoles = true
        end
    end

    -- Fallback: If item inherently has NO stats (e.g. white items, tabards, some starter gear), allow it.
    local hasAnyStatLine = false
    local allKnownStats = {"stamina", "intellect", "agility", "strength", "spirit", "spell power", "mana per 5", "attack power", "critical", "haste", "hit", "defense", "dodge", "parry", "block", "armor penetration", "expertise"}
    for _, lowerText in ipairs(parsedItemStats) do
        for _, stat in ipairs(allKnownStats) do
            if lowerText:find(stat) then
                hasAnyStatLine = true
                break
            end
        end
    end

    if not hasAnyStatLine then
        anyRelevantAcrossAllRoles = true
    end

    itemScoreCache[cacheKey] = { score = maxScore, relevant = anyRelevantAcrossAllRoles }
    return maxScore, anyRelevantAcrossAllRoles
end

-- Smart role routing: search all bots to find the best upgrade for a given role (TANK, HEALER, etc.)
GetBestBotForRole = function(itemID, roleType)
    if not itemID or not roleType then return nil end
    local _, itemLink, _, _, _, _, _, _, itemEquipLoc = GetItemInfo(itemID)
    if not itemEquipLoc then return nil end
    
    local targetSlot = EquipLocToSlot[itemEquipLoc]
    if not targetSlot then return nil end
    
    -- Handle fingers/trinkets slot defaults
    if targetSlot == "FINGER1" or targetSlot == "FINGER2" then
        targetSlot = "FINGER1"
    elseif targetSlot == "TRINKET1" or targetSlot == "TRINKET2" then
        targetSlot = "TRINKET1"
    end
    
    local bestBot = nil
    local maxUpgrade = -999999
    
    for entry, botData in pairs(db) do
        -- Skip configuration keys in DB
        if type(entry) == "number" and botData and botData.className then
            -- Verify if the bot can equip this item
            local canEquip, failReason = CanBotEquipItem(botData.className, itemID, entry)
            if canEquip then
                -- Match the bot's current talent spec first, falling back to the live role mask.
                local match = GetBotRoleCategory(botData) == roleType
                
                if match then
                    -- Calculate item stats score for this class+role
                    local newItemScore = ScoreItemForClass(itemID, botData.className, botData.roles, botData.spec)
                    
                    -- Check current item in slot
                    local currentItemID = nil
                    if botData.gear and botData.gear[targetSlot] then
                        currentItemID = botData.gear[targetSlot].id
                    end
                    
                    -- Handle rings and trinkets selection dynamically to optimize weaker slot
                    local actualSlot = targetSlot
                    if targetSlot == "FINGER1" and botData.gear then
                        local f1 = botData.gear["FINGER1"] and botData.gear["FINGER1"].id
                        local f2 = botData.gear["FINGER2"] and botData.gear["FINGER2"].id
                        local f1Score = f1 and ScoreItemForClass(f1, botData.className, botData.roles, botData.spec) or 0
                        local f2Score = f2 and ScoreItemForClass(f2, botData.className, botData.roles, botData.spec) or 0
                        if f1Score < f2Score then
                            currentItemID = f1
                            actualSlot = "FINGER1"
                        else
                            currentItemID = f2
                            actualSlot = "FINGER2"
                        end
                    elseif targetSlot == "TRINKET1" and botData.gear then
                        local t1 = botData.gear["TRINKET1"] and botData.gear["TRINKET1"].id
                        local t2 = botData.gear["TRINKET2"] and botData.gear["TRINKET2"].id
                        local t1Score = t1 and ScoreItemForClass(t1, botData.className, botData.roles, botData.spec) or 0
                        local t2Score = t2 and ScoreItemForClass(t2, botData.className, botData.roles, botData.spec) or 0
                        if t1Score < t2Score then
                            currentItemID = t1
                            actualSlot = "TRINKET1"
                        else
                            currentItemID = t2
                            actualSlot = "TRINKET2"
                        end
                    end
                    
                    local currentItemScore = currentItemID and ScoreItemForClass(currentItemID, botData.className, botData.roles, botData.spec) or 0
                    local upgrade = newItemScore - currentItemScore
                    
                    -- If upgrade margins are better than current max
                    if upgrade > maxUpgrade then
                        maxUpgrade = upgrade
                        bestBot = {
                            entry = entry,
                            name = botData.name,
                            className = botData.className,
                            upgrade = upgrade,
                            targetSlot = actualSlot
                        }
                    end
                end
            end
        end
    end
    
    return bestBot
end

-- AUTHENTIC SYMMETRY: Includes non-functional Shirt & Tabard slots to create a perfect 8x8 balance.
local slotPositions = {
    -- Left Side (8 items: Head to Wrist)
    HEAD     = { "TOPLEFT", 20, -20, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Head" },
    NECK     = { "TOPLEFT", 20, -59, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Neck" },
    SHOULDER = { "TOPLEFT", 20, -98, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Shoulder" },
    BACK     = { "TOPLEFT", 20, -137, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Chest" },
    CHEST    = { "TOPLEFT", 20, -176, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Chest" },
    SHIRT    = { "TOPLEFT", 20, -215, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Shirt", true },  -- Fake Slot
    TABARD   = { "TOPLEFT", 20, -254, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Tabard", true }, -- Fake Slot
    WRIST    = { "TOPLEFT", 20, -293, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Wrists" },

    -- Right Side (8 items: Hands to Trinket2)
    HANDS    = { "TOPRIGHT", -20, -20, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Hands" },
    WAIST    = { "TOPRIGHT", -20, -59, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Waist" },
    LEGS     = { "TOPRIGHT", -20, -98, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Legs" },
    FEET     = { "TOPRIGHT", -20, -137, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Feet" },
    FINGER1  = { "TOPRIGHT", -20, -176, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Finger" },
    FINGER2  = { "TOPRIGHT", -20, -215, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Finger" },
    TRINKET1 = { "TOPRIGHT", -20, -254, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Trinket" },
    TRINKET2 = { "TOPRIGHT", -20, -293, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Trinket" },

    -- Weapons at the bottom
    MAINHAND = { "BOTTOM", -40, 20, "Interface\\Paperdoll\\UI-PaperDoll-Slot-MainHand" },
    OFFHAND  = { "BOTTOM", 0, 20, "Interface\\Paperdoll\\UI-PaperDoll-Slot-SecondaryHand" },
    RANGED   = { "BOTTOM", 40, 20, "Interface\\Paperdoll\\UI-PaperDoll-Slot-Ranged" },
}

local slots = {}



local function CreateItemSlot(key, data, parent)
    local btn = CreateFrame("Button", "BMU_Slot_" .. key, parent, "ItemButtonTemplate")
    btn:SetSize(37, 37)
    btn:SetPoint(data[1], data[2], data[3])
    -- Ensure slot buttons sit ABOVE the 3D model so precise targeting wins
    btn:SetFrameLevel(BotModel:GetFrameLevel() + 5)
    
    btn.icon = _G[btn:GetName() .. "IconTexture"]
    btn.emptyTexture = data[4]
    btn.icon:SetTexture(btn.emptyTexture)
    
    btn.isFake = data[5] -- Marks Shirt & Tabard as visually inactive

    -- [[ ITEM RARITY BORDER ]] --
    btn.rarityGlow = btn:CreateTexture(nil, "OVERLAY")
    btn.rarityGlow:SetTexture("Interface\\Buttons\\UI-ActionButton-Border")
    btn.rarityGlow:SetBlendMode("ADD")
    btn.rarityGlow:SetPoint("CENTER", btn, "CENTER", 0, 1)
    btn.rarityGlow:SetWidth(btn:GetWidth() * 1.8)
    btn.rarityGlow:SetHeight(btn:GetHeight() * 1.8)
    btn.rarityGlow:Hide()

    -- Handle the non-functional empty slots uniquely
    if btn.isFake then
        btn.icon:SetVertexColor(0.4, 0.4, 0.4) -- Darken the empty texture to show it's disabled
        btn:SetScript("OnEnter", function(self)
            GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
            if key == "SHIRT" then GameTooltip:SetText("衬衣栏位") else GameTooltip:SetText("战袍栏位") end
            GameTooltip:AddLine("NPC 机器人不使用此栏位。", 1, 0, 0)
            GameTooltip:Show()
        end)
        btn:SetScript("OnLeave", function() GameTooltip:Hide() end)
        return btn
    end
    
    -- Normal functioning slots
    btn:SetScript("OnEnter", function(self)
        if self.itemID and self.itemID > 0 then
            GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
            local gear = MainFrame.selectedBot and db[MainFrame.selectedBot] and db[MainFrame.selectedBot].gear[key]
            local link = (gear and BuildGearItemLink(gear)) or self.itemLink or ("item:" .. self.itemID .. ":0:0:0:0:0:0:0")
            GameTooltip:SetHyperlink(link)
            if not MainFrame.inspectMode then
                GameTooltip:AddLine(" ")
                GameTooltip:AddLine("|cff00ff00右键点击卸下|r")
            end
            GameTooltip:Show()
        end
    end)
    btn:SetScript("OnLeave", function() GameTooltip:Hide() end)
    
    btn:RegisterForClicks("AnyUp")
    
    local function HandleItemDrop(self)
        if MainFrame.inspectMode or not MainFrame.selectedBot then return end
        local type, id, info = GetCursorInfo()
        if type == "item" then
            local _, _, _, _, _, _, _, _, itemEquipLoc = GetItemInfo(id)
            local targetSlot = itemEquipLoc and EquipLocToSlot[itemEquipLoc] or nil
            
            -- Check class proficiency and level
            local botData = db[MainFrame.selectedBot]
            local canEquip, failReason = CanBotEquipItem(botData and botData.className, id, MainFrame.selectedBot)
            if botData and not canEquip then
                if failReason == "LEVEL" then
                    local _, _, _, _, itemMinLevel = GetItemInfo(id)
                    print("|cffff0000机器人管理:|r 「" .. (botData.name or "该机器人") .. "」等级为 " .. (botData.level or "?") .. "，该物品需要等级 " .. (itemMinLevel or "?") .. "。")
                elseif failReason == "CACHE" then
                    print("|cff00ff00机器人管理:|r 正在查询物品数据库，请稍后再试。")
                else
                    print("|cffff0000机器人管理:|r 「" .. (botData.name or "该机器人") .. "」（" .. (botData.className or "未知") .. "）无法穿戴该物品。")
                end
                return
            end
            
            -- Basic visual slot match (backend handles actual validation)
             if targetSlot == key or (targetSlot == "MAINHAND" and key == "OFFHAND") or (targetSlot == "FINGER1" and (key == "FINGER1" or key == "FINGER2")) or (targetSlot == "TRINKET1" and (key == "TRINKET1" or key == "TRINKET2")) or not targetSlot then
                  local itemName = GetItemInfo(id) or ("物品 #" .. id)
                  local actions = {}
                  local finalSlot = key
                  local blocked = false
                  
                  -- Smart cross-bot unequip check: Skip if we have another copy in our bags!
                  local availableCount = GetItemCountInBags(id)
                  if availableCount <= 0 then
                      for otherEntry, otherBotData in pairs(db) do
                          if type(otherEntry) == "number" and otherEntry ~= MainFrame.selectedBot then
                              if otherBotData.gear then
                                  for otherSlotKey, otherGearData in pairs(otherBotData.gear) do
                                      if otherGearData and otherGearData.id == id then
                                          if IsBotOnline(otherBotData.name) then
                                              table.insert(actions, {
                                                  type = "UNEQUIP",
                                                  botEntry = otherEntry,
                                                  botName = otherBotData.name,
                                                  slotKey = otherSlotKey
                                              })
                                          else
                                              print("|cffff0000机器人管理警告:|r 无法从离线机器人「" .. (otherBotData.name or "未知") .. "」自动卸下物品，请先手动卸下。")
                                              blocked = true
                                          end
                                      end
                                  end
                              end
                          end
                      end
                  end
                  
                  if not blocked then
                      table.insert(actions, {
                          type = "EQUIP",
                          botEntry = MainFrame.selectedBot,
                          botName = botData.name,
                          slotKey = finalSlot,
                          itemID = id,
                          itemName = itemName
                      })
                      
                      StartSequentialActions(actions, MainFrame.selectedBot, botData.name, "EQUIP_ITEM")
                      ClearCursor()
                  else
                      ClearCursor()
                  end
             else
                  print("|cffff0000机器人管理:|r 无法将物品放入 " .. key .. " 栏位。")
                  -- Clear cursor anyway so they aren't stuck holding it
                  ClearCursor()
             end
        end
    end

    btn:SetScript("OnReceiveDrag", HandleItemDrop)
    
    btn:SetScript("OnClick", function(self, button)
        if MainFrame.inspectMode then
            if self.itemID and self.itemID > 0 then
                GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
                local gear = MainFrame.selectedBot and db[MainFrame.selectedBot] and db[MainFrame.selectedBot].gear[key]
                GameTooltip:SetHyperlink((gear and BuildGearItemLink(gear)) or self.itemLink or ("item:" .. self.itemID .. ":0:0:0:0:0:0:0"))
                GameTooltip:Show()
            end
            return
        end
        if CursorHasItem() then
            HandleItemDrop(self)
        elseif button == "RightButton" and MainFrame.selectedBot and self.itemID then
            local botEntry = MainFrame.selectedBot
            local botData = db[botEntry]
            local itemName = GetItemInfo(self.itemID) or ("物品 #" .. self.itemID)
            if not db[botEntry].gear then db[botEntry].gear = {} end
            db[botEntry].gear[key] = nil
            self.itemID = nil
            SelectBot(botEntry)
            SendBMUMessage("UNEQUIP;" .. botEntry .. ";" .. key)
            pendingAction = "已从「" .. (botData and botData.name or "机器人") .. "」的 " .. key .. " 栏位卸下 |cffffd700" .. itemName .. "|r"
            RequestServerRefreshAfterDelay(0.6)
        end
    end)
    
    return btn
end

for key, data in pairs(slotPositions) do
    slots[key] = CreateItemSlot(key, data, GearContainer)
end

ApplyInspectModeUI = function(enabled)
    MainFrame.inspectMode = enabled and true or false
    if enabled then
        local data = MainFrame.selectedBot and db[MainFrame.selectedBot]
        UpdateMainFrameTitle(data)
        Sidebar:Hide()
        SidebarTools:Hide()
        if PrevBotBtn then PrevBotBtn:Hide() end
        if NextBotBtn then NextBotBtn:Hide() end
        db.showStats = true
        if StatsPanel and MainFrame.selectedBot then
            StatsPanel:Show()
        end
    else
        local data = MainFrame.selectedBot and db[MainFrame.selectedBot]
        UpdateMainFrameTitle(data)
        Sidebar:Show()
        SidebarTools:Show()
        if PrevBotBtn then PrevBotBtn:Show() end
        if NextBotBtn then NextBotBtn:Show() end
    end
end

function NPCBotInventory_OpenInspect(botEntry)
    if not botEntry then return end
    pendingInspectEntry = botEntry
    MainFrame.selectedBot = botEntry
    ApplyInspectModeUI(true)
    SendBMUMessage("QUERY;" .. botEntry)
    MainFrame:Show()
end

local function GetCreatureEntryFromUnit(unit)
    local guid = UnitGUID(unit)
    if not guid then return nil end
    local unitType, _, _, _, _, id = strsplit("-", guid)
    if unitType ~= "Creature" then return nil end
    return tonumber(id)
end

local function IsLikelyNPCBotUnit(unit)
    if not unit or not UnitExists(unit) or UnitIsPlayer(unit) then return false end
    local entry = GetCreatureEntryFromUnit(unit)
    return entry and entry >= BOT_ENTRY_MIN
end

-- [[ DATA LOGIC ]] --
function SelectBot(entry)
    MainFrame.selectedBot = entry
    local data = db[entry]
    if not data then return end
    
    -- Update 3D Model
    UpdateBotModel(entry)
    
    for key, btn in pairs(slots) do
        if not btn.isFake then
            local item = data.gear and data.gear[key]
            if item and item.id > 0 then
                local itemLink = BuildGearItemLink(item)
                if itemLink and not GetItemInfo(itemLink) then
                    BMUScannerTooltip:SetOwner(WorldFrame, "ANCHOR_NONE")
                    BMUScannerTooltip:SetHyperlink(itemLink)
                end
                -- Get Item Rarity from WoW Cache (full link shows enchant in tooltip)
                local _, _, itemRarity, _, _, _, _, _, _, tex = GetItemInfo(itemLink or item.id)
                btn.icon:SetTexture(tex or "Interface\\Icons\\INV_Misc_QuestionMark")
                btn.itemID = item.id
                btn.itemLink = itemLink

                -- Apply Rarity Border Colors
                if itemRarity and itemRarity > 1 then -- Greater than common (Uncommon/Rare/Epic/Legendary)
                    local r, g, b = GetItemQualityColor(itemRarity)
                    btn.rarityGlow:SetVertexColor(r, g, b, 1)
                    btn.rarityGlow:Show()
                else
                    btn.rarityGlow:Hide()
                end
            else
                btn.icon:SetTexture(btn.emptyTexture)
                btn.itemID = nil
                btn.itemLink = nil
                btn.rarityGlow:Hide()
            end
        end
    end
    
    UpdateStatsPanel(entry)
    UpdateNavigationButtons()
    
    -- Live update the layout if the editor frame happens to be open!
    if BotManagerEditorFrame and BotManagerEditorFrame:IsShown() and BotManagerEditorFrame.RefreshAll then
        BotManagerEditorFrame:RefreshAll()
    end
end

-- Must be global so XML/Search boxes can call it
function RefreshBotList(filterText)
    local children = { ScrollContent:GetChildren() }
    for _, child in ipairs(children) do child:Hide() end
    
    local sorted = {}
    for entry, info in pairs(db) do 
        if type(info) == "table" and info.name then
            local match = true
            
            -- [[ SMART SEARCH LOGIC ]] --
            if filterText and filterText ~= "" then
                match = false
                
                -- Check Bot Name
                if info.name:lower():find(filterText) then match = true end
                
                -- Check Bot Class
                if info.className and info.className:lower():find(filterText) then match = true end
                
                -- Check Gear (Name, Type, Subtype)
                if not match and info.gear then
                    for _, gearData in pairs(info.gear) do
                        if gearData.id > 0 then
                            local itemName, _, _, _, _, itemType, itemSubType = GetItemInfo(gearData.id)
                            if itemName and itemName:lower():find(filterText) then match = true; break end
                            if itemType and itemType:lower():find(filterText) then match = true; break end
                            if itemSubType and itemSubType:lower():find(filterText) then match = true; break end
                        end
                    end
                end
            end
            
            if match then table.insert(sorted, info) end
        end
    end
    
    -- Apply Sorting
    table.sort(sorted, function(a,b) 
        if currentSortMode == "CLASS" then
            if a.className == b.className then return a.name < b.name end
            return (a.className or "") < (b.className or "")
        elseif currentSortMode == "ROLE" then
            local aRole = GetBotRolesLabel(a.roles)
            local bRole = GetBotRolesLabel(b.roles)
            if aRole == bRole then
                if a.className == b.className then return a.name < b.name end
                return (a.className or "") < (b.className or "")
            end
            return aRole < bRole
        elseif currentSortMode == "TALENT" then
            local aTalent = GetBotTalentLabel(a)
            local bTalent = GetBotTalentLabel(b)
            if aTalent == bTalent then
                if a.className == b.className then return a.name < b.name end
                return (a.className or "") < (b.className or "")
            end
            return aTalent < bTalent
        else
            return a.name < b.name 
        end
    end)
    
    -- Save sorted list for navigation arrows
    MainFrame.sortedBots = sorted
    
    -- Arrow Click Logic
    PrevBotBtn:SetScript("PostClick", function()
        for i, info in ipairs(MainFrame.sortedBots) do
            if info.entry == MainFrame.selectedBot then
                if i > 1 then SelectBot(MainFrame.sortedBots[i-1].entry) end
                return
            end
        end
    end)
    
    NextBotBtn:SetScript("PostClick", function()
        for i, info in ipairs(MainFrame.sortedBots) do
            if info.entry == MainFrame.selectedBot then
                if i < #MainFrame.sortedBots then SelectBot(MainFrame.sortedBots[i+1].entry) end
                return
            end
        end
    end)
    
    -- Auto-Select Logic
    if #sorted > 0 then
        local found = false
        if MainFrame.selectedBot then
            for _, info in ipairs(sorted) do
                if info.entry == MainFrame.selectedBot then
                    found = true
                    break
                end
            end
        end
        if not found then
            SelectBot(sorted[1].entry)
        end
    end
    
    -- Hide all buttons first
    for _, b in ipairs(BotButtonsPool) do
        b:ClearAllPoints()
        b:Hide()
    end
    
    -- Draw Buttons
    local y = 0
    for i, info in ipairs(sorted) do
        local botEntry = info.entry
        local botName = info.name
        local botClassName = info.className
        local b = BotButtonsPool[i]
        if not b then
            if not InCombatLockdown() then
                b = CreateFrame("Button", "BMU_BotButton_" .. i, ScrollContent, "SecureActionButtonTemplate")
                b:SetSize(100, 22)
                b:SetNormalFontObject("GameFontNormalSmall")
                b:SetHighlightTexture("Interface\\Buttons\\UI-Listbox-Highlight")
                SetupBotListButton(b)
                table.insert(BotButtonsPool, b)
            else
                break
            end
        end
        
        b.botEntry = botEntry
        b:SetPoint("TOPLEFT", 5, y)
        
        -- Apply Class Colors to the list
        local cColor = classColors[botClassName] or "FFFFFF"
        b:SetText("|cff" .. cColor .. botName .. "|r")
        
        -- Configure secure action
        SafeSetAttribute(b, "type", "macro")
        SafeSetAttribute(b, "macrotext", "/target " .. botName)
        
        -- Set script
        b:SetScript("PostClick", function(self)
            SelectBot(self.botEntry)
        end)
        
        b:Show()
        y = y - 24
    end

    ScrollContent:SetHeight(math.max(1, -y + 4))
    
    if MainFrame.selectedBot and MainFrame:IsShown() then
        SelectBot(MainFrame.selectedBot)
    end
end

-- [[ MINIMAP BUTTON ]] --
local MinimapBtn = CreateFrame("Button", "BotManagerMinimapButton", Minimap)
MinimapBtn:SetSize(32, 32)
MinimapBtn:SetFrameStrata("MEDIUM")
MinimapBtn:SetFrameLevel(8)
MinimapBtn:SetNormalTexture("Interface\\Icons\\INV_Misc_Robot_01") 
MinimapBtn:SetPushedTexture("Interface\\Icons\\INV_Misc_Robot_01")
MinimapBtn:SetHighlightTexture("Interface\\Minimap\\UI-Minimap-ZoomButton-Highlight")
MinimapBtn:RegisterForClicks("LeftButtonUp", "RightButtonUp")
MinimapBtn:RegisterForDrag("LeftButton")

function UpdateMinimapPosition()
    local rad = math.rad(db.minimap.angle)
    MinimapBtn:SetPoint("TOPLEFT", Minimap, "TOPLEFT", 52 - (80 * math.cos(rad)), (80 * math.sin(rad)) - 52)
end

function UpdateMinimapVisibility()
    if db and db.minimap and db.minimap.hide then
        MinimapBtn:Hide()
    else
        MinimapBtn:Show()
    end
end

MinimapBtn:SetScript("OnDragStart", function(self)
    self:LockHighlight()
    self:SetScript("OnUpdate", function(self)
        local xpos, ypos = GetCursorPosition()
        local xmin, ymin = Minimap:GetLeft(), Minimap:GetBottom()
        db.minimap.angle = math.deg(math.atan2(ypos/UIParent:GetScale() - ymin - 70, xmin - xpos/UIParent:GetScale() + 70))
        UpdateMinimapPosition()
    end)
end)

MinimapBtn:SetScript("OnDragStop", function(self)
    self:SetScript("OnUpdate", nil)
    self:UnlockHighlight()
end)

MinimapBtn:SetScript("OnClick", function()
    ToggleBotManagerWindows()
end)

-- [[ INTERFACE OPTIONS ]] --
local OptionsPanel = CreateFrame("Frame", "BotManagerOptionsPanel", UIParent)
OptionsPanel.name = "NPCBotInventory"

local optionsTitle = OptionsPanel:CreateFontString(nil, "ARTWORK", "GameFontNormalLarge")
optionsTitle:SetPoint("TOPLEFT", 16, -16)
optionsTitle:SetText("NPC 机器人装备管理")

local optionsSubtitle = OptionsPanel:CreateFontString(nil, "ARTWORK", "GameFontHighlightSmall")
optionsSubtitle:SetPoint("TOPLEFT", optionsTitle, "BOTTOMLEFT", 0, -8)
optionsSubtitle:SetText("机器人管理窗口与小地图按钮设置。")

local minimapCheck = CreateFrame("CheckButton", "BotManagerOptionsMinimapCheck", OptionsPanel, "InterfaceOptionsCheckButtonTemplate")
minimapCheck:SetPoint("TOPLEFT", optionsSubtitle, "BOTTOMLEFT", 0, -18)
_G[minimapCheck:GetName() .. "Text"]:SetText("显示小地图按钮")
minimapCheck:SetScript("OnClick", function(self)
    db.minimap = db.minimap or { angle = 45 }
    db.minimap.hide = not self:GetChecked()
    UpdateMinimapVisibility()
end)

local hotkeyText = OptionsPanel:CreateFontString(nil, "ARTWORK", "GameFontHighlight")
hotkeyText:SetPoint("TOPLEFT", minimapCheck, "BOTTOMLEFT", 0, -18)
hotkeyText:SetText("快捷键：Shift+Z")

local openBtn = CreateFrame("Button", nil, OptionsPanel, "UIPanelButtonTemplate")
openBtn:SetSize(160, 24)
openBtn:SetPoint("TOPLEFT", hotkeyText, "BOTTOMLEFT", 0, -18)
openBtn:SetText("打开机器人管理")
openBtn:SetScript("OnClick", function()
    MainFrame:Show()
end)

local resetBtn = CreateFrame("Button", nil, OptionsPanel, "UIPanelButtonTemplate")
resetBtn:SetSize(180, 24)
resetBtn:SetPoint("LEFT", openBtn, "RIGHT", 10, 0)
resetBtn:SetText("重置窗口布局")
resetBtn:SetScript("OnClick", function()
    MainFrame:ClearAllPoints()
    MainFrame:SetSize(480, 460)
    MainFrame:SetPoint("CENTER")

    if BotManagerEditorFrame then
        BotManagerEditorFrame.hasBeenMoved = nil
        BotManagerEditorFrame.hasBeenResized = nil
        BotManagerEditorFrame:SetSize(620, MainFrame:GetHeight() or 460)
        if BotManager_DockEditorFrame then
            BotManager_DockEditorFrame()
        end
    end

    if StatsPanel then
        StatsPanel.hasBeenMoved = nil
        StatsPanel.hasBeenResized = nil
        StatsPanel:SetSize(265, MainFrame:GetHeight() or 460)
        if BotManager_DockStatsPanel then
            BotManager_DockStatsPanel()
        end
    end
end)

OptionsPanel:SetScript("OnShow", function()
    db = db or {}
    db.minimap = db.minimap or { angle = 45 }
    minimapCheck:SetChecked(not db.minimap.hide)
end)

local optionsRegistered = false
function RegisterBotManagerOptions()
    if optionsRegistered then return end
    if InterfaceOptions_AddCategory then
        InterfaceOptions_AddCategory(OptionsPanel)
        optionsRegistered = true
    end
end

-- [[ EXPORT UTILITIES FOR EDITOR ]] --
BotManager_StartSequentialActions = StartSequentialActions
BotManager_ScoreItemForClass = ScoreItemForClass
BotManager_DecodeBotRoles = DecodeBotRoles
BotManager_GetBotRoleCategory = GetBotRoleCategory
BotManager_GetRoleMaskForCategory = GetRoleMaskForCategory
BotManager_GetDb = function() return db end
BotManager_CanBotEquipItem = CanBotEquipItem
BotManager_ArmorProficiency = ArmorProficiency
BotManager_ArmorSubtypeToID = ArmorSubtypeToID
BotManager_WeaponProficiency = WeaponProficiency
BotManager_WeaponSubtypeToID = WeaponSubtypeToID
BotManager_GetItemCountInBags = GetItemCountInBags
BotManager_IsBotOnline = IsBotOnline
BotManager_GetTemplateBucketForBot = GetTemplateBucketForBot
BotManager_GetTemplateRoleKeyForBot = GetTemplateRoleKeyForBot
BotManager_GetSelectedTemplateBucket = GetSelectedTemplateBucket
BotManager_MakeTemplateItemState = MakeTemplateItemState
BotManager_GetTemplateItemID = GetTemplateItemID
BotManager_SetTemplateSlot = SetTemplateSlot
BotManager_TemplateSlotKeys = TEMPLATE_SLOT_KEYS

