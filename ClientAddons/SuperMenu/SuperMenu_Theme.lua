-- 奥术魔典 · 主题（3.3.5）
SuperMenuTheme = SuperMenuTheme or {}
local T = SuperMenuTheme

T.FRAME_W = 780
T.FRAME_H = 540
T.SPINE_W = 82
T.HEADER_H = 46
T.FOOTER_H = 88
T.PAD = 10

T.COLOR = {
    title    = { 1.00, 0.92, 0.55 },
    sub      = { 0.85, 0.78, 0.65 },
    label    = { 0.95, 0.88, 0.70 },
    ink      = { 0.18, 0.12, 0.06 },
    pageBg   = { 0.38, 0.30, 0.20 },
    listBg   = { 0.08, 0.06, 0.04, 0.75 },
    listRow  = { 0.14, 0.10, 0.06, 0.55 },
    listText = { 0.95, 0.93, 0.82 },
    listDim  = { 0.72, 0.68, 0.58 },
    leather  = { 0.06, 0.04, 0.03 },
    tabOff   = { 0.20, 0.14, 0.09 },
    tabOn    = { 0.45, 0.32, 0.12 },
    gold     = { 1.00, 0.85, 0.35 },
}

T.LIST_WIDTH = T.FRAME_W - 28 - T.SPINE_W - 6 - 40

T._scrollSerial = 0

function T.RGBA(c, a)
    if not c then return 1, 1, 1, 1 end
    return c[1], c[2], c[3], a or (c[4] or 1)
end

function T.GetContentWidth()
    return T.FRAME_W - 28 - T.SPINE_W - 6 - 36
end

function T.SectionLabel(parent, text, anchor, x, y)
    local fs = parent:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
    fs:SetPoint(anchor or "TOPLEFT", parent, anchor or "TOPLEFT", x or (T.PAD + 2), y or -4)
    fs:SetText(text)
    fs:SetTextColor(T.RGBA(T.COLOR.label))
    return fs
end

function T.LabelSmall(parent, text)
    local fs = parent:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    fs:SetText(text)
    fs:SetTextColor(T.RGBA(T.COLOR.label))
    return fs
end

function T.ListWidth(parent)
    local w = parent and parent:GetWidth() or 0
    if w and w > 120 then return w - 8 end
    return T.LIST_WIDTH
end

function T.ApplyLeatherBackdrop(frame)
    frame:SetBackdrop({
        bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
        edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
        tile = true, tileSize = 32, edgeSize = 32,
        insets = { left = 9, right = 9, top = 9, bottom = 9 },
    })
    frame:SetBackdropColor(T.RGBA(T.COLOR.leather, 1))
    frame:SetBackdropBorderColor(0.40, 0.30, 0.16, 1)
end

function T.ApplyPageBackdrop(frame)
    frame:SetBackdrop({
        bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        tile = true, tileSize = 16, edgeSize = 12,
        insets = { left = 4, right = 4, top = 4, bottom = 4 },
    })
    frame:SetBackdropColor(T.RGBA(T.COLOR.pageBg, 1))
    frame:SetBackdropBorderColor(0.55, 0.42, 0.22, 1)
end

function T.ApplyListPanel(frame)
    T.ApplyPageBackdrop(frame)
    frame:SetBackdropColor(T.RGBA(T.COLOR.listBg))
    frame:SetBackdropBorderColor(0.45, 0.35, 0.18, 1)
end

function T.CreateTabButton(parent, text, index, frame, tabCount)
    local btn = CreateFrame("Button", "SuperMenuTab" .. index, parent)
    btn:SetSize(T.SPINE_W - 10, 30)
    btn:SetPoint("TOP", parent, "TOP", 0, -4 - (index - 1) * 34)

    local bg = btn:CreateTexture(nil, "BACKGROUND")
    bg:SetAllPoints()
    bg:SetTexture("Interface\\Buttons\\UI-Silver-Button-Up")
    btn.bg = bg

    local hi = btn:CreateTexture(nil, "HIGHLIGHT")
    hi:SetAllPoints()
    hi:SetTexture("Interface\\Buttons\\UI-Panel-Button-Highlight")
    hi:SetBlendMode("ADD")
    hi:SetAlpha(0.4)

    local label = btn:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    label:SetPoint("CENTER")
    label:SetText(text)
    btn.label = label

    btn:SetScript("OnClick", function()
        T.SetActiveTab(frame, index, tabCount)
        if frame.onTabClick then frame.onTabClick(index) end
    end)
    return btn
end

function T.SetActiveTab(frame, activeIndex, tabCount)
    for i = 1, tabCount do
        local tab = frame.tabButtons[i]
        if tab then
            if i == activeIndex then
                tab.bg:SetVertexColor(T.RGBA(T.COLOR.tabOn))
                tab.label:SetTextColor(T.RGBA(T.COLOR.gold))
            else
                tab.bg:SetVertexColor(T.RGBA(T.COLOR.tabOff))
                tab.label:SetTextColor(0.78, 0.70, 0.58)
            end
        end
    end
    if frame.chapterTitle and frame.tabNames then
        frame.chapterTitle:SetText("— " .. (frame.tabNames[activeIndex] or "") .. " —")
    end
    frame.currentTab = activeIndex
end

function T.SetTabVisible(frame, tabIndex, tabCount)
    for i = 1, tabCount do
        local p = frame.panels[i]
        if p then
            if i == tabIndex then p:Show() else p:Hide() end
        end
    end
    T.SetActiveTab(frame, tabIndex, tabCount)
end

function T.CreateEditBox(parent, width)
    local box = CreateFrame("EditBox", nil, parent, "InputBoxTemplate")
    box:SetSize(width or 120, 20)
    box:SetAutoFocus(false)
    if box.SetFontObject then box:SetFontObject(ChatFontNormal) end
    box:SetTextColor(1, 1, 1)
    return box
end

function T.CreateScrollArea(parent, topOffset, bottomOffset, leftOffset, rightOffset)
    topOffset = topOffset or T.PAD
    bottomOffset = bottomOffset or T.PAD
    leftOffset = leftOffset or T.PAD
    rightOffset = rightOffset or (T.PAD + 24)

    T._scrollSerial = T._scrollSerial + 1
    local name = "SuperMenuScroll" .. T._scrollSerial
    local scroll = CreateFrame("ScrollFrame", name, parent, "UIPanelScrollFrameTemplate")
    scroll:SetPoint("TOPLEFT", leftOffset, -topOffset)
    scroll:SetPoint("BOTTOMRIGHT", -rightOffset, bottomOffset)

    local child = CreateFrame("Frame", name .. "Child", scroll)
    child:SetWidth(T.ListWidth(parent))
    scroll:SetScrollChild(child)
    scroll:SetFrameLevel(parent:GetFrameLevel() + 2)
    scroll.child = child
    return scroll, child
end

function T.CreateActionButton(parent, text, w, h, onClick)
    local btn = CreateFrame("Button", nil, parent, "UIPanelButtonTemplate")
    btn:SetSize(w or 130, h or 22)
    if type(text) == "string" and text:sub(1, 2) ~= "|c" then
        text = "|cfffff2cc" .. text .. "|r"
    end
    btn:SetText(text)
    btn:SetScript("OnClick", onClick)
    return btn
end

-- 网格布局：offsetY 向下递增，SetPoint 用负 Y（3.3.5 标准）
function T.LayoutGrid(parent, items, cols, btnW, btnH, gapX, gapY)
    cols = cols or 3
    btnW = btnW or 150
    btnH = btnH or 22
    gapX = gapX or 6
    gapY = gapY or 6
    local x0 = 4
    local maxRow = 0
    for i, item in ipairs(items) do
        local col = (i - 1) % cols
        local row = math.floor((i - 1) / cols)
        maxRow = math.max(maxRow, row)
        local b = T.CreateActionButton(parent, item[1], btnW, btnH, item[2])
        b:SetPoint("TOPLEFT", parent, "TOPLEFT", x0 + col * (btnW + gapX), -(4 + row * (btnH + gapY)))
    end
    parent:SetHeight(4 + (maxRow + 1) * (btnH + gapY) + 4)
    return maxRow + 1
end

-- 分节流式布局（传送/召唤），返回总高度
function T.LayoutSections(parent, sections, opts)
    opts = opts or {}
    local cols = opts.cols or 3
    local btnW = opts.btnW or 168
    local btnH = opts.btnH or 22
    local gapX = opts.gapX or 6
    local gapY = opts.gapY or 6
    local onItem = opts.onItem
    local offsetY = 8

    for _, section in ipairs(sections or {}) do
        local title = parent:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
        title:SetPoint("TOPLEFT", parent, "TOPLEFT", 8, -offsetY)
        title:SetText("|cffffd966" .. (section.name or "") .. "|r")
        offsetY = offsetY + 24

        local col = 0
        for _, item in ipairs(section.items or section.points or {}) do
            local label = item[1]
            local fn = onItem and onItem(item)
            if fn then
                local b = T.CreateActionButton(parent, label, btnW, btnH, fn)
                b:SetPoint("TOPLEFT", parent, "TOPLEFT", 8 + col * (btnW + gapX), -offsetY)
            end
            col = col + 1
            if col >= cols then
                col = 0
                offsetY = offsetY + btnH + gapY
            end
        end
        if col > 0 then
            offsetY = offsetY + btnH + gapY
        end
        offsetY = offsetY + 12
    end
    return math.max(200, offsetY + 8)
end

function T.CreatePage(parent, hasFooter)
    local page = CreateFrame("Frame", nil, parent)
    page:SetAllPoints(parent)
    T.ApplyPageBackdrop(page)

    page.body = CreateFrame("Frame", nil, page)
    if hasFooter then
        page.body:SetPoint("TOPLEFT", 2, -2)
        page.body:SetPoint("BOTTOMRIGHT", 2, T.FOOTER_H + 2)
        page.footer = CreateFrame("Frame", nil, page)
        page.footer:SetPoint("BOTTOMLEFT", 4, 4)
        page.footer:SetPoint("BOTTOMRIGHT", -4, 4)
        page.footer:SetHeight(T.FOOTER_H)
        T.ApplyPageBackdrop(page.footer)
        page.footer:SetBackdropColor(0.82, 0.76, 0.64, 1)
    else
        page.body:SetPoint("TOPLEFT", 2, -2)
        page.body:SetPoint("BOTTOMRIGHT", -2, 2)
    end
    return page
end

function T.PlayOpen(frame)
    frame:Show()
    frame:SetAlpha(0)
    UIFrameFadeIn(frame, 0.2, 0, 1)
end

function T.CreateMainShell(tabNames)
    if SuperMenu.mainFrame and SuperMenu.mainFrame.uiVersion == 26 then
        return SuperMenu.mainFrame
    end
    if SuperMenu.mainFrame then
        SuperMenu.mainFrame:Hide()
        SuperMenu.mainFrame = nil
    end

    local f = CreateFrame("Frame", "SuperMenuGrimoire", UIParent)
    f:SetSize(T.FRAME_W, T.FRAME_H)
    f:SetPoint("CENTER")
    f:SetFrameStrata("FULLSCREEN_DIALOG")
    f:SetFrameLevel(120)
    f:SetMovable(true)
    f:EnableMouse(true)
    f:RegisterForDrag("LeftButton")
    f:SetScript("OnDragStart", f.StartMoving)
    f:SetScript("OnDragStop", f.StopMovingOrSizing)
    T.ApplyLeatherBackdrop(f)

    local blocker = CreateFrame("Frame", nil, f)
    blocker:SetAllPoints(f)
    blocker:SetFrameLevel(f:GetFrameLevel())
    local blockerTex = blocker:CreateTexture(nil, "BACKGROUND")
    blockerTex:SetAllPoints()
    blockerTex:SetTexture(0, 0, 0, 0.72)

    local header = CreateFrame("Frame", nil, f)
    header:SetPoint("TOPLEFT", 12, -12)
    header:SetPoint("TOPRIGHT", -12, -12)
    header:SetHeight(T.HEADER_H)
    header:SetFrameLevel(f:GetFrameLevel() + 2)

    local title = header:CreateFontString(nil, "OVERLAY", "GameFontNormalHuge")
    title:SetPoint("TOP", 0, -2)
    title:SetText("奥术魔典")
    title:SetTextColor(T.RGBA(T.COLOR.title))

    local sub = header:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    sub:SetPoint("TOP", title, "BOTTOM", 0, -1)
    sub:SetText("Arcane Grimoire")
    sub:SetTextColor(T.RGBA(T.COLOR.sub))

    f.chapterTitle = header:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    f.chapterTitle:SetPoint("RIGHT", header, "RIGHT", -40, -6)
    f.chapterTitle:SetTextColor(T.RGBA(T.COLOR.gold))

    local close = CreateFrame("Button", nil, f, "UIPanelCloseButton")
    close:SetPoint("TOPRIGHT", -6, -6)
    close:SetScript("OnClick", function() SuperMenu.Hide() end)

    local body = CreateFrame("Frame", nil, f)
    body:SetPoint("TOPLEFT", 16, -12 - T.HEADER_H)
    body:SetPoint("BOTTOMRIGHT", -16, 16)
    body:SetFrameLevel(f:GetFrameLevel() + 2)

    local spine = CreateFrame("Frame", "SuperMenuSpine", body)
    spine:SetPoint("TOPLEFT", 0, 0)
    spine:SetPoint("BOTTOMLEFT", 0, 0)
    spine:SetWidth(T.SPINE_W)
    T.ApplyPageBackdrop(spine)
    spine:SetBackdropColor(0.18, 0.13, 0.09, 1)
    f.spine = spine

    f.pageHost = CreateFrame("Frame", nil, body)
    f.pageHost:SetPoint("TOPLEFT", spine, "TOPRIGHT", 8, 0)
    f.pageHost:SetPoint("BOTTOMRIGHT", 0, 0)
    T.ApplyPageBackdrop(f.pageHost)
    f.pageHost:SetBackdropColor(T.RGBA(T.COLOR.pageBg, 1))
    f.pageHost:SetBackdropBorderColor(0.50, 0.38, 0.20, 1)

    f.tabNames = tabNames
    f.tabButtons = {}
    f.panels = {}
    f.hasSummonTab = true
    f.uiVersion = 26
    f:Hide()
    SuperMenu.mainFrame = f
    return f
end
