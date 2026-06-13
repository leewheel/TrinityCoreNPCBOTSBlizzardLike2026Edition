local addon, ns = ...

local mainFrame = ns.mainFrame
local settingsTab = mainFrame.tabs.settings
local _, classFileName = UnitClass("player")

local scrollFrame = CreateFrame("ScrollFrame", "$parentSettingsScroll", settingsTab, "UIPanelScrollFrameTemplate")
scrollFrame:SetPoint("TOPLEFT", 8, -8)
scrollFrame:SetPoint("BOTTOMRIGHT", -28, 8)

local scrollChild = CreateFrame("Frame", nil, scrollFrame)
scrollChild:SetSize(scrollFrame:GetWidth(), 794)
scrollFrame:SetScrollChild(scrollChild)

local function applyCardStyle(card)
    card:SetBackdrop({
        bgFile = "Interface\\Buttons\\WHITE8x8",
        edgeFile = "Interface\\Buttons\\WHITE8x8",
        tile = true,
        tileSize = 8,
        edgeSize = 1,
        insets = {left = 1, right = 1, top = 1, bottom = 1}
    })
    card:SetBackdropColor(0.05, 0.055, 0.07, 0.92)
    card:SetBackdropBorderColor(0.56, 0.47, 0.20, 0.75)

    local top = card:CreateTexture(nil, "OVERLAY")
    top:SetTexture("Interface\\Buttons\\WHITE8x8")
    top:SetHeight(1)
    top:SetPoint("TOPLEFT", 1, -1)
    top:SetPoint("TOPRIGHT", -1, -1)
    top:SetVertexColor(1, 0.92, 0.64, 0.22)

    local bottom = card:CreateTexture(nil, "OVERLAY")
    bottom:SetTexture("Interface\\Buttons\\WHITE8x8")
    bottom:SetHeight(1)
    bottom:SetPoint("BOTTOMLEFT", 1, 1)
    bottom:SetPoint("BOTTOMRIGHT", -1, 1)
    bottom:SetVertexColor(0, 0, 0, 0.7)
end

local function createCard(parent, title, y, height)
    local card = CreateFrame("Frame", nil, parent)
    card:SetPoint("TOPLEFT", 10, y)
    card:SetSize(parent:GetWidth() - 20, height)
    applyCardStyle(card)

    local titleText = card:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
    titleText:SetPoint("TOPLEFT", 12, -12)
    titleText:SetText(title)
    titleText:SetTextColor(1.00, 0.83, 0.24)
    titleText:SetFont("Fonts\\FRIZQT__.TTF", 14, "OUTLINE")

    local divider = card:CreateTexture(nil, "OVERLAY")
    divider:SetTexture("Interface\\Buttons\\WHITE8x8")
    divider:SetHeight(1)
    divider:SetPoint("TOPLEFT", 12, -32)
    divider:SetPoint("TOPRIGHT", -12, -32)
    divider:SetVertexColor(0.62, 0.52, 0.22, 0.65)

    card.divider = divider
    return card
end

local function createCheckboxRow(parent, label, settingKey, y, tooltip, onToggle)
    local row = CreateFrame("Frame", nil, parent)
    row:SetPoint("TOPLEFT", 12, y)
    row:SetSize(parent:GetWidth() - 24, 34)

    local rowBg = row:CreateTexture(nil, "BACKGROUND")
    rowBg:SetTexture("Interface\\Buttons\\WHITE8x8")
    rowBg:SetAllPoints()
    rowBg:SetVertexColor(1, 1, 1, 0)

    local cb = CreateFrame("CheckButton", "$parentCB_"..settingKey, row)
    cb:SetPoint("LEFT", 8, 0)
    cb:SetSize(22, 22)
    cb:SetNormalTexture("Interface\\Buttons\\UI-CheckBox-Up")
    cb:SetPushedTexture("Interface\\Buttons\\UI-CheckBox-Down")
    cb:SetHighlightTexture("Interface\\Buttons\\UI-CheckBox-Highlight", "ADD")
    cb:SetCheckedTexture("Interface\\Buttons\\UI-CheckBox-Check")
    cb:GetNormalTexture():SetVertexColor(0.80, 0.65, 0.22)
    cb:GetCheckedTexture():SetVertexColor(1.0, 0.82, 0.20)

    local labelText = row:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    labelText:SetPoint("LEFT", cb, "RIGHT", 8, 0)
    labelText:SetText(label)
    labelText:SetTextColor(0.95, 0.88, 0.65)

    cb:SetScript("OnClick", function(self)
        local enabled = self:GetChecked() == 1
        ns.GetSettings()[settingKey] = enabled
        if onToggle then
            onToggle(enabled, self)
        end
        PlaySound("gsTitleOptionOK")
    end)
    cb:SetScript("OnShow", function(self)
        self:SetChecked(ns.GetSettings()[settingKey])
    end)

    row:EnableMouse(true)
    row:SetScript("OnEnter", function(self)
        rowBg:SetVertexColor(0.30, 0.25, 0.10, 0.22)
        if tooltip then
            GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
            GameTooltip:SetText(label, 1, 0.82, 0.20)
            GameTooltip:AddLine(tooltip, 1, 1, 1, true)
            GameTooltip:Show()
        end
    end)
    row:SetScript("OnLeave", function()
        rowBg:SetVertexColor(1, 1, 1, 0)
        GameTooltip:Hide()
    end)
    row:SetScript("OnMouseDown", function()
        cb:Click()
    end)

    return cb
end

local persistenceCard = createCard(scrollChild, "持久化", -14, 180)
createCheckboxRow(persistenceCard, "跨会话保持幻化状态", "saveMorphState", -42, "登录时自动恢复角色的幻化状态")
createCheckboxRow(persistenceCard, "按角色保存坐骑幻化", "saveMountMorph", -76, "为此角色记住坐骑幻化")
createCheckboxRow(persistenceCard, "按角色保存宠物幻化", "savePetMorph", -110, "为此角色记住宠物幻化")
createCheckboxRow(persistenceCard, "按角色保存战斗宠物幻化", "saveCombatPetMorph", -144, "为此角色记住猎人宠物幻化")

local behaviorCard = createCard(scrollChild, "行为", -202, 112)
createCheckboxRow(behaviorCard, "显示术士恶魔形态", "showMetamorphosis", -42, "临时显示恶魔形态（暂停幻化）", function(enabled)
    ns.SendRawMorphCommand("SET:META:"..(enabled and "1" or "0"))
    if classFileName == "WARLOCK" then
        local inForm = GetShapeshiftForm() > 0
        if inForm then
            if enabled and not ns.morphSuspended then
                ns.morphSuspended = true
                if not ns.dbwSuspended and not ns.vehicleSuspended then ns.SendRawMorphCommand("SUSPEND") end
            elseif not enabled and ns.morphSuspended then
                ns.morphSuspended = false
                if not ns.dbwSuspended and not ns.vehicleSuspended then ns.SendRawMorphCommand("RESUME") end
            end
        end
    end
end)
createCheckboxRow(behaviorCard, "变形态时保持幻化", "morphInShapeshift", -76, "变形态时保持幻化效果（如德鲁伊形态等）", function(enabled)
    ns.SendRawMorphCommand("SET:SHAPE:"..(enabled and "1" or "0"))
    if enabled and ns.morphSuspended then
        ns.morphSuspended = false
        if not ns.dbwSuspended and not ns.vehicleSuspended then ns.SendRawMorphCommand("RESUME") end
    elseif not enabled and ns.IsModelChangingForm() and not ns.morphSuspended then
        ns.morphSuspended = true
        if not ns.dbwSuspended and not ns.vehicleSuspended then ns.SendRawMorphCommand("SUSPEND") end
    end
end)

local syncCard = createCard(scrollChild, "多人同步", -322, 80)
createCheckboxRow(syncCard, "启用世界同步", "enableWorldSync", -42, "与其他玩家同步幻化效果。关闭后立即移除其他玩家的幻化（你的幻化不受影响）。", function(enabled)
    if ns.P2PToggleWorldSync then
        ns.P2PToggleWorldSync(enabled)
    end
end)

local interfaceCard = createCard(scrollChild, "界面", -406, 112)
createCheckboxRow(interfaceCard, "显示小地图按钮", "showMinimapButton", -42, "在小地图上显示幻化按钮。", function(enabled)
    if ns.UpdateMinimapButton then ns.UpdateMinimapButton() end
end)
createCheckboxRow(interfaceCard, "隐藏角色信息按钮", "hidePaperdollButton", -76, "在角色信息面板上隐藏幻化按钮。", function(enabled)
    if ns.UpdatePaperdollButtonVisibility then ns.UpdatePaperdollButtonVisibility() end
end)

local statusCard = createCard(scrollChild, "系统状态", -526, 102)
local statusIcon = statusCard:CreateTexture(nil, "ARTWORK")
statusIcon:SetSize(30, 30)
statusIcon:SetPoint("TOPLEFT", 14, -50)
local statusTitle = statusCard:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
statusTitle:SetPoint("TOPLEFT", statusIcon, "TOPRIGHT", 10, 2)
local statusDesc = statusCard:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
statusDesc:SetPoint("TOPLEFT", statusTitle, "BOTTOMLEFT", 0, -4)
statusDesc:SetPoint("RIGHT", -12, 0)
statusDesc:SetJustifyH("LEFT")
statusDesc:SetTextColor(0.85, 0.85, 0.85)

local function UpdateDLLStatus()
    if ns.IsMorpherReady() then
        statusIcon:SetTexture("Interface\\RAIDFRAME\\ReadyCheck-Ready")
        statusTitle:SetText("|cff4ACC4A幻化DLL：已加载|r")
        statusDesc:SetText("幻化引擎正在运行，可以开始改变你的角色外观了。")
        statusCard:SetBackdropBorderColor(0.29, 0.80, 0.29, 0.8)
    else
        statusIcon:SetTexture("Interface\\RAIDFRAME\\ReadyCheck-NotReady")
        statusTitle:SetText("|cffff0000幻化DLL：未加载|r")
        statusDesc:SetText("请将dinput8.dll放入您的WoW目录以启用幻化功能。")
        statusCard:SetBackdropBorderColor(0.80, 0.29, 0.29, 0.8)
    end
end

local aboutCard = createCard(scrollChild, "关于", -668, 160)
local infoText = aboutCard:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
infoText:SetPoint("TOPLEFT", 12, -42)
infoText:SetPoint("BOTTOMRIGHT", -12, 12)
infoText:SetJustifyH("LEFT")
infoText:SetJustifyV("TOP")
infoText:SetTextColor(0.95, 0.88, 0.65)
infoText:SetText("幻化大师 v"..ns.VERSION.."\n\n适用于魔兽世界巫妖王之怒 3.3.5a 的幻化与外观系统。\n改变你的角色外观、装备、坐骑、宠物、附魔和称号。\n包含可选的多人同步功能，与其他用户共享视觉效果。\n需要 dinput8.dll 才能正常运行。")

settingsTab:SetScript("OnShow", function()
    UpdateDLLStatus()
end)
settingsTab:SetScript("OnUpdate", function(self, elapsed)
    if not self.statusUpdateTimer then return end
    self.statusUpdateTimer = self.statusUpdateTimer + elapsed
    if self.statusUpdateTimer >= 2 then
        UpdateDLLStatus()
        self.statusUpdateTimer = 0
    end
end)
settingsTab:HookScript("OnShow", function(self)
    self.statusUpdateTimer = 0
end)
settingsTab:SetScript("OnHide", function(self)
    self.statusUpdateTimer = nil
end)
