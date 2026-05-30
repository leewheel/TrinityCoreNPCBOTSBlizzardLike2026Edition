SuperMenu = SuperMenu or {}

local T = SuperMenuTheme
local TAB_NAMES = { "常用", "传送", "任务", "附魔", "召唤", "GM" }
local TAB_COUNT = #TAB_NAMES

local function Btn(parent, text, w, h, fn)
    return T.CreateActionButton(parent, text, w, h, fn)
end

function SuperMenu.FillEnchantList(slotIndex)
    local panel = SuperMenu.mainFrame and SuperMenu.mainFrame.enchantPanel
    if not panel or not panel.listChild then return end

    local menu = SuperMenu.EnchantSlotMenus and SuperMenu.EnchantSlotMenus[slotIndex]
    if not menu then return end

    local child = panel.listChild
    for _, w in ipairs(child.widgets or {}) do
        w:Hide()
        w:SetParent(nil)
    end
    child.widgets = {}

    local offsetY = 4
    local btnW = child:GetWidth() - 4
    for _, enc in ipairs(menu.enchants) do
        local b = Btn(child, enc.name, btnW, 22, function()
            SendSMUEnchant(enc.id, menu.slot)
        end)
        b:SetPoint("TOPLEFT", child, "TOPLEFT", 2, -offsetY)
        table.insert(child.widgets, b)
        offsetY = offsetY + 24
    end
    child:SetHeight(math.max(200, offsetY + 8))

    if panel.slotTitle then
        panel.slotTitle:SetText(menu.name .. "  ·  " .. #menu.enchants .. " 项")
    end
end

function SuperMenu.BuildEnchantPanel(page)
    local body = page.body

    local left = CreateFrame("Frame", nil, body)
    left:SetPoint("TOPLEFT", T.PAD, -T.PAD)
    left:SetPoint("BOTTOMLEFT", T.PAD, T.PAD)
    left:SetWidth(96)
    T.ApplyPageBackdrop(left)
    left:SetBackdropColor(0.80, 0.74, 0.62, 1)

    T.SectionLabel(left, "部位", "TOPLEFT", 6, -6)

    T._scrollSerial = (T._scrollSerial or 0) + 1
    local slotName = "SuperMenuEnchantSlots" .. T._scrollSerial
    local slotScroll = CreateFrame("ScrollFrame", slotName, left, "UIPanelScrollFrameTemplate")
    slotScroll:SetPoint("TOPLEFT", 4, -22)
    slotScroll:SetPoint("BOTTOMRIGHT", -22, 4)
    local slotChild = CreateFrame("Frame", slotName .. "Child", slotScroll)
    slotChild:SetWidth(72)
    slotScroll:SetScrollChild(slotChild)

    page.slotButtons = {}
    local offsetY = 4
    for i, menu in ipairs(SuperMenu.EnchantSlotMenus or {}) do
        local b = Btn(slotChild, menu.name, 72, 22, function()
            SuperMenu.FillEnchantList(i)
            for j, bb in ipairs(page.slotButtons) do
                if j == i then
                    bb:LockHighlight()
                else
                    bb:UnlockHighlight()
                end
            end
        end)
        b:SetPoint("TOPLEFT", slotChild, "TOPLEFT", 0, -offsetY)
        page.slotButtons[i] = b
        offsetY = offsetY + 24
    end
    slotChild:SetHeight(math.max(100, offsetY + 8))

    local right = CreateFrame("Frame", nil, body)
    right:SetPoint("TOPLEFT", left, "TOPRIGHT", 6, 0)
    right:SetPoint("BOTTOMRIGHT", -T.PAD, T.PAD)

    page.slotTitle = right:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    page.slotTitle:SetPoint("TOPLEFT", 4, -6)
    page.slotTitle:SetTextColor(T.COLOR.label[1], T.COLOR.label[2], T.COLOR.label[3])
    page.slotTitle:SetText("—")

    local scroll, child = T.CreateScrollArea(right, 28, T.PAD, T.PAD, T.PAD + 22)
    child.widgets = {}
    page.listChild = child
    page.enchantPanel = page

    SuperMenu.FillEnchantList(1)
    if page.slotButtons[1] then page.slotButtons[1]:LockHighlight() end
end

function SuperMenu.BuildQuickPanel(page)
    local body = page.body
    T.SectionLabel(body, "常用咒文", "TOPLEFT", T.PAD, -6)

    local gridHost = CreateFrame("Frame", nil, body)
    gridHost:SetPoint("TOPLEFT", T.PAD, -26)
    gridHost:SetPoint("TOPRIGHT", -T.PAD, -26)
    gridHost:SetHeight(128)
    T.ApplyListPanel(gridHost)
    gridHost:SetBackdropColor(0.16, 0.12, 0.09, 0.92)

    local items = {}
    for _, act in ipairs(SuperMenu.QuickActions or {}) do
        table.insert(items, { act[1], function() SendSMUAction(act[2]) end })
    end
    T.LayoutGrid(gridHost, items, 4, 152, 22, 6, 5)

    if page.footer then
        T.SectionLabel(page.footer, "坐标书签", "TOPLEFT", T.PAD + 2, -8)

        local function AddBookmarkSlot(slot, col, row)
            local x0 = T.PAD + col * 200
            local y0 = -28 - row * 30

            local num = page.footer:CreateFontString(nil, "OVERLAY", "GameFontNormal")
            num:SetPoint("TOPLEFT", page.footer, "TOPLEFT", x0, y0)
            num:SetWidth(20)
            num:SetText(slot .. ".")
            num:SetTextColor(T.RGBA(T.COLOR.gold))

            Btn(page.footer, "铭刻", 58, 22, function()
                SendSMU("MARK;" .. slot .. ";SET")
            end):SetPoint("TOPLEFT", page.footer, "TOPLEFT", x0 + 22, y0)

            Btn(page.footer, "折跃", 58, 22, function()
                local b = SuperMenu_GetBookmarks()[slot]
                if b and b.mapId then
                    SendSMUTeleport(b.mapId, b.x, b.y, b.z, b.o)
                else
                    print("|cffff6666奥术魔典:|r 书签 " .. slot .. " 未铭刻。")
                end
            end):SetPoint("TOPLEFT", page.footer, "TOPLEFT", x0 + 84, y0)
        end

        -- 上排 1-3，下排 4-5
        for slot = 1, 3 do
            AddBookmarkSlot(slot, slot - 1, 0)
        end
        AddBookmarkSlot(4, 0, 1)
        AddBookmarkSlot(5, 1, 1)
    end
end

function SuperMenu.BuildTelePanel(page)
    local body = page.body
    local listW = T.LIST_WIDTH

    local listFrame = CreateFrame("Frame", nil, body)
    listFrame:SetPoint("TOPLEFT", T.PAD, -T.PAD)
    listFrame:SetPoint("BOTTOMRIGHT", -T.PAD, T.PAD)
    T.ApplyListPanel(listFrame)

    T._scrollSerial = (T._scrollSerial or 0) + 1
    local scrollName = "SuperMenuTeleList" .. T._scrollSerial
    local scroll = CreateFrame("ScrollFrame", scrollName, listFrame, "UIPanelScrollFrameTemplate")
    scroll:SetPoint("TOPLEFT", 6, -6)
    scroll:SetPoint("BOTTOMRIGHT", -28, 6)

    local child = CreateFrame("Frame", scrollName .. "Child", scroll)
    child:SetWidth(listW)
    scroll:SetScrollChild(child)

    local height = T.LayoutSections(child, SuperMenu.TeleCategories, {
        cols = 3,
        btnW = 168,
        onItem = function(pt)
            local mapId, x, yv, z, o = pt[2], pt[3], pt[4], pt[5], pt[6]
            return function()
                SendSMUTeleport(mapId, x, yv, z, o)
            end
        end,
    })
    child:SetHeight(height)
end

function SuperMenu.RefreshQuestScroll()
    local frame = SuperMenu.mainFrame
    if not frame or not frame.questScrollChild then return end
    local child = frame.questScrollChild
    local listW = T.ListWidth(child)

    for _, row in ipairs(child.rows or {}) do
        if row.line then row.line:Hide() row.line:SetParent(nil) end
    end
    child.rows = {}

    local filter = ""
    if frame.questFilterBox then
        filter = string.lower(frame.questFilterBox:GetText() or "")
    end

    local offsetY = 4
    local rowIndex = 0
    for _, row in ipairs(SuperMenu.questRows) do
        if filter == "" or string.find(string.lower(row.key), filter, 1, true) then
            rowIndex = rowIndex + 1
            local line = CreateFrame("Frame", nil, child)
            line:SetSize(listW, 24)
            line:SetPoint("TOPLEFT", child, "TOPLEFT", 0, -offsetY)

            local bg = line:CreateTexture(nil, "BACKGROUND")
            bg:SetAllPoints()
            if rowIndex % 2 == 0 then
                bg:SetTexture(T.RGBA(T.COLOR.listRow))
            else
                bg:SetTexture(0.05, 0.04, 0.03, 0.35)
            end

            local cb = CreateFrame("CheckButton", nil, line, "UICheckButtonTemplate")
            cb:SetSize(22, 22)
            cb:SetPoint("LEFT", 4, 0)
            cb.questId = row.id
            cb:SetChecked(SuperMenu.questSelected[row.id] or false)
            cb:SetScript("OnClick", function(self)
                SuperMenu.questSelected[self.questId] = self:GetChecked() and true or false
            end)

            local stateHex = (row.state == "R") and "88ff88" or "ffdd66"
            local title = row.title or ""
            title = title:gsub("|", "||")
            local text = string.format(
                "|cff%s%s|r |cffffffff#%d|r  |cffe8e0d0%s|r",
                stateHex,
                (row.state == "R") and "已交" or "完成",
                row.id,
                title
            )

            local fs = line:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
            fs:SetPoint("LEFT", cb, "RIGHT", 8, 0)
            fs:SetWidth(listW - 36)
            fs:SetJustifyH("LEFT")
            fs:SetText(text)

            table.insert(child.rows, { line = line })
            offsetY = offsetY + 24
        end
    end
    child:SetHeight(math.max(280, offsetY + 8))
end

function SuperMenu.BuildQuestPanel(page)
    local body = page.body
    local listW = T.LIST_WIDTH

    local toolbar = CreateFrame("Frame", nil, body)
    toolbar:SetPoint("TOPLEFT", T.PAD, -T.PAD)
    toolbar:SetPoint("TOPRIGHT", -T.PAD, -T.PAD)
    toolbar:SetHeight(58)
    T.ApplyListPanel(toolbar)
    toolbar:SetBackdropColor(0.12, 0.09, 0.06, 0.9)

    local filterLabel = T.LabelSmall(toolbar, "筛选任务")
    filterLabel:SetPoint("TOPLEFT", 8, -8)

    local filterBox = T.CreateEditBox(toolbar, 180)
    filterBox:SetPoint("TOPLEFT", 8, -28)

    local addLabel = T.LabelSmall(toolbar, "接取 ID")
    addLabel:SetPoint("TOPLEFT", 200, -8)

    local addBox = T.CreateEditBox(toolbar, 72)
    addBox:SetPoint("TOPLEFT", 200, -28)
    addBox:SetNumeric(true)

    Btn(toolbar, "接取", 58, 22, function()
        SendSMUQuestAdd(addBox:GetText())
    end):SetPoint("TOPLEFT", 278, -28)

    Btn(toolbar, "刷新列表", 88, 22, function()
        SuperMenu_ClearQuestUI()
        SendSMUQuestList()
    end):SetPoint("TOPRIGHT", -96, -8)

    Btn(toolbar, "重置选中", 88, 22, function()
        local ids = {}
        for qid, sel in pairs(SuperMenu.questSelected) do
            if sel then table.insert(ids, qid) end
        end
        SendSMUQuestReset(ids)
    end):SetPoint("TOPRIGHT", -4, -8)

    filterBox:SetScript("OnTextChanged", function() SuperMenu.RefreshQuestScroll() end)
    page.questFilterBox = filterBox

    local listFrame = CreateFrame("Frame", nil, body)
    listFrame:SetPoint("TOPLEFT", T.PAD, -68)
    listFrame:SetPoint("BOTTOMRIGHT", -T.PAD, T.PAD)
    T.ApplyListPanel(listFrame)

    T._scrollSerial = (T._scrollSerial or 0) + 1
    local scrollName = "SuperMenuQuestList" .. T._scrollSerial
    local scroll = CreateFrame("ScrollFrame", scrollName, listFrame, "UIPanelScrollFrameTemplate")
    scroll:SetPoint("TOPLEFT", 6, -6)
    scroll:SetPoint("BOTTOMRIGHT", -28, 6)

    local child = CreateFrame("Frame", scrollName .. "Child", scroll)
    child:SetWidth(listW)
    scroll:SetScrollChild(child)
    child.rows = {}
    page.questScrollChild = child
    page.questListFrame = listFrame
end

function SuperMenu.BuildSummonPanel(page)
    local body = page.body
    local listW = T.LIST_WIDTH

    local listFrame = CreateFrame("Frame", nil, body)
    listFrame:SetPoint("TOPLEFT", T.PAD, -T.PAD)
    listFrame:SetPoint("BOTTOMRIGHT", -T.PAD, T.PAD)
    T.ApplyListPanel(listFrame)
    listFrame:SetBackdropColor(0.14, 0.11, 0.08, 0.95)

    T._scrollSerial = (T._scrollSerial or 0) + 1
    local scrollName = "SuperMenuSummonList" .. T._scrollSerial
    local scroll = CreateFrame("ScrollFrame", scrollName, listFrame, "UIPanelScrollFrameTemplate")
    scroll:SetPoint("TOPLEFT", 6, -6)
    scroll:SetPoint("BOTTOMRIGHT", -28, 6)
    scroll:SetFrameLevel(listFrame:GetFrameLevel() + 2)

    local child = CreateFrame("Frame", scrollName .. "Child", scroll)
    child:SetWidth(listW)
    scroll:SetScrollChild(child)
    page.summonScrollChild = child

    local sections = SuperMenu.SummonSections
    local hasSections = sections and sections[1]
    if not hasSections then
        local hint = child:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
        hint:SetPoint("TOPLEFT", child, "TOPLEFT", 12, -16)
        hint:SetWidth(listW - 24)
        hint:SetJustifyH("LEFT")
        hint:SetText("|cffff8888召唤列表未加载。|r\n请将整个 SuperMenu 文件夹复制到：\n|cffffff00Interface\\AddOns\\SuperMenu\\|r\n并确认含 SuperMenu_SummonData.lua，然后 /reload。")
        child:SetHeight(100)
        return
    end

    local height = T.LayoutSections(child, sections, {
        cols = 3,
        btnW = 168,
        onItem = function(item)
            local entry = item[2]
            return function()
                SendSMUSummon(entry)
            end
        end,
    })
    child:SetHeight(height)
end

function SuperMenu.BuildGMPanel(page)
    local body = page.body
    T.SectionLabel(body, "角色与系统", "TOPLEFT", T.PAD, -6)

    local items = {
        { "保存并返回角色选择", function() SendSMUAction("LOGOUT") end },
        { "不保存离开", function() SendSMUAction("LOGOUT_NO_SAVE") end },
    }
    local host = CreateFrame("Frame", nil, body)
    host:SetPoint("TOPLEFT", T.PAD, -28)
    host:SetPoint("TOPRIGHT", -T.PAD, -28)
    host:SetHeight(60)
    T.LayoutGrid(host, items, 2, 280, 24, 12, 6)
end

function SuperMenu.CreateMainFrame()
    if SuperMenu.mainFrame and SuperMenu.mainFrame.uiVersion == 26 and SuperMenu.mainFrame.panels[TAB_COUNT] then
        return SuperMenu.mainFrame
    end

    local f = T.CreateMainShell(TAB_NAMES)
    f.panels = {}
    f.tabButtons = {}

    for i, name in ipairs(TAB_NAMES) do
        f.tabButtons[i] = T.CreateTabButton(f.spine, name, i, f, TAB_COUNT)
        local hasFooter = (i == 1)
        f.panels[i] = T.CreatePage(f.pageHost, hasFooter)
        f.panels[i]:Hide()
    end

    SuperMenu.BuildQuickPanel(f.panels[1])
    SuperMenu.BuildTelePanel(f.panels[2])
    SuperMenu.BuildQuestPanel(f.panels[3])
    SuperMenu.BuildEnchantPanel(f.panels[4])
    SuperMenu.BuildSummonPanel(f.panels[5])
    SuperMenu.BuildGMPanel(f.panels[6])

    f.questScrollChild = f.panels[3].questScrollChild
    f.questFilterBox = f.panels[3].questFilterBox
    f.enchantPanel = f.panels[4]

    f.onTabClick = function(index)
        T.SetTabVisible(f, index, TAB_COUNT)
        if index == 3 and #SuperMenu.questRows == 0 then
            SendSMUQuestList()
        end
    end

    T.SetTabVisible(f, 1, TAB_COUNT)
    return f
end

function SuperMenu.Show()
    SuperMenu.CreateMainFrame()
    T.PlayOpen(SuperMenu.mainFrame)
end

function SuperMenu.Hide()
    if SuperMenu.mainFrame then
        SuperMenu.mainFrame:Hide()
        SuperMenu.mainFrame:SetAlpha(1)
    end
end

function SuperMenu.Toggle()
    if SuperMenu.mainFrame and SuperMenu.mainFrame:IsShown() then
        SuperMenu.Hide()
    else
        SuperMenu.Show()
    end
end
