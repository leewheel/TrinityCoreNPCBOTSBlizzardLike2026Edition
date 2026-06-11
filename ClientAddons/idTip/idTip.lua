-- idTip for WotLK 3.3.5 (Interface 30300)
-- Ported from retail idTip with API compatibility shims.

local hooksecurefunc, select, tonumber, pairs, ipairs, type, pcall, string = hooksecurefunc, select, tonumber, pairs, ipairs, type, pcall, string
local addonName = ...

local function GetSpellIcon(spellId)
  if not spellId then return end
  return select(3, GetSpellInfo(spellId))
end

local function GetItemIcon(itemId)
  if not itemId then return end
  return select(10, GetItemInfo(itemId))
end

local kinds = {
  spell = "SpellID",
  item = "ItemID",
  unit = "NPC ID",
  quest = "QuestID",
  talent = "TalentID",
  achievement = "AchievementID",
  criteria = "CriteriaID",
  ability = "AbilityID",
  enchant = "EnchantID",
  gem = "GemID",
  macro = "MacroID",
  icon = "IconID",
  object = "ObjectID",
}

local function configKey(key)
  return key .. "Enabled"
end

-- 3.3.5 frames have no :HookScript(); wrap SetScript instead.
local function hookScript(frame, script, handler)
  if not frame or not frame.GetScript or not frame.SetScript then return end
  local orig = frame:GetScript(script)
  frame:SetScript(script, function(...)
    if orig then
      orig(...)
    end
    handler(...)
  end)
end

local function hook(table, fn, cb)
  if table and table[fn] then
    hooksecurefunc(table, fn, cb)
  end
end

local function getTooltipName(tooltip)
  return tooltip:GetName() or nil
end

local function addLine(tooltip, id, kind)
  if not id or id == "" or not tooltip or not tooltip.GetName then return end
  if idTipConfig and (not idTipConfig.enabled or not idTipConfig[configKey(kind)]) then return end
  if type(id) == "table" and #id == 1 then id = id[1] end

  local ok, name = pcall(getTooltipName, tooltip)
  if not ok or not name then return end

  local frame, text
  for i = tooltip:NumLines(), 1, -1 do
    frame = _G[name .. "TextLeft" .. i]
    if frame then text = frame:GetText() end
    if text and string.find(text, kinds[kind]) then return end
  end

  local left, right
  if type(id) == "table" then
    left = NORMAL_FONT_COLOR_CODE .. kinds[kind] .. "s" .. FONT_COLOR_CODE_CLOSE
    right = HIGHLIGHT_FONT_COLOR_CODE .. table.concat(id, ", ") .. FONT_COLOR_CODE_CLOSE
  else
    left = NORMAL_FONT_COLOR_CODE .. kinds[kind] .. FONT_COLOR_CODE_CLOSE
    right = HIGHLIGHT_FONT_COLOR_CODE .. id .. FONT_COLOR_CODE_CLOSE
  end

  tooltip:AddDoubleLine(left, right)
  tooltip:Show()
end

local function isStringOrNumber(val)
  local t = type(val)
  return t == "string" or t == "number"
end

local function add(tooltip, id, kind)
  addLine(tooltip, id, kind)

  if kind == "spell" and isStringOrNumber(id) then
    local iconId = GetSpellIcon(id)
    if iconId then add(tooltip, iconId, "icon") end
  end

  if kind == "item" and isStringOrNumber(id) then
    local iconId = GetItemIcon(id)
    if iconId then add(tooltip, iconId, "icon") end
  end
end

local function addByKind(tooltip, id, kind)
  if not kind or not id then return end
  if kind == "spell" or kind == "enchant" or kind == "trade" then
    add(tooltip, id, "spell")
  elseif kinds[kind] then
    add(tooltip, id, kind)
  end
end

local function splitItemString(itemString)
  local itemSplit = {}
  for v in string.gmatch(itemString, "(%d*:?)") do
    if v == ":" then
      itemSplit[#itemSplit + 1] = 0
    else
      itemSplit[#itemSplit + 1] = tonumber(string.gsub(v, ":", "")) or 0
    end
  end
  return itemSplit
end

local function attachItemTooltip(tooltip)
  if not tooltip or not tooltip.GetItem then return end

  local _, link = tooltip:GetItem()
  if not link then return end

  local itemString = string.match(link, "item:([%-?%d:]+)")
  if not itemString then return end

  local itemSplit = splitItemString(itemString)
  local itemId = itemSplit[1]
  if not itemId or itemId == 0 then return end

  add(tooltip, itemId, "item")

  -- WotLK link: item:id:enchant:gem1:gem2:gem3:gem4:...
  if itemSplit[2] and itemSplit[2] ~= 0 then
    add(tooltip, itemSplit[2], "enchant")
  end

  local gems = {}
  for i = 3, 6 do
    if itemSplit[i] and itemSplit[i] ~= 0 then
      gems[#gems + 1] = itemSplit[i]
    end
  end
  if #gems > 0 then
    add(tooltip, gems, "gem")
  end
end

-------------------------------------------------------------------------------
-- Tooltip hooks (3.3.5) — installed after PLAYER_LOGIN so FrameXML cannot overwrite them.
-------------------------------------------------------------------------------

local hooksInstalled = false

local function npcIdFromUnit(unit)
  if not unit or not UnitGUID then return end
  local guid = UnitGUID(unit)
  if not guid then return end
  local unitType = guid:match("^(%a+)-")
  if unitType == "Player" or unitType == "Pet" then return end
  return tonumber(guid:match("-(%d+)-%x+$"), 10)
end

local function installTooltipHooks()
  if hooksInstalled then return end
  hooksInstalled = true

  local function onSetHyperlink(tooltip, link)
    if not link then return end
    local kind, id = string.match(link, "^(%a+):(%d+)")
    addByKind(tooltip, id, kind)
  end

  hook(ItemRefTooltip, "SetHyperlink", onSetHyperlink)
  hook(GameTooltip, "SetHyperlink", onSetHyperlink)

  if GetActionInfo then
    hook(GameTooltip, "SetAction", function(tooltip, slot)
      local kind, id = GetActionInfo(slot)
      addByKind(tooltip, id, kind)
      if kind == "macro" and id and GetMacroSpell then
        local spellId = GetMacroSpell(id)
        if spellId then add(tooltip, spellId, "spell") end
      end
    end)
  end

  hook(_G, "SetItemRef", function(link)
    if not link then return end
    local spellId = tonumber(link:match("spell:(%d+)"))
    if spellId then add(ItemRefTooltip, spellId, "spell") end
  end)

  -- 3.3.5 GetSpell() returns name/rank only; hook SetSpell and hyperlink paths instead.
  if GameTooltip.SetSpell then
    hook(GameTooltip, "SetSpell", function(tooltip, index, bookType)
      if not GetSpellLink then return end
      local link = GetSpellLink(index, bookType)
      if link then
        local id = tonumber(link:match("spell:(%d+)"))
        add(tooltip, id, "spell")
      end
    end)
  end

  if SpellBook_GetSpellBookSlot then
    hook(_G, "SpellButton_OnEnter", function()
      local slot = SpellBook_GetSpellBookSlot(this)
      if slot and GetSpellBookItemInfo then
        local spellID = select(2, GetSpellBookItemInfo(slot, SpellBookFrame.bookType))
        add(GameTooltip, spellID, "spell")
      end
    end)
  end

  if GameTooltip.SetUnit then
    hook(GameTooltip, "SetUnit", function(tooltip, unit)
      local id = npcIdFromUnit(unit)
      if id then add(tooltip, id, "unit") end
    end)
  end

  hookScript(GameTooltip, "OnTooltipSetUnit", function(tooltip)
    local _, unit = tooltip:GetUnit()
    local id = npcIdFromUnit(unit)
    if id then add(tooltip, id, "unit") end
  end)

  local function onSetItem(tooltip)
    attachItemTooltip(tooltip)
  end

  hookScript(GameTooltip, "OnTooltipSetItem", onSetItem)
  hookScript(ItemRefTooltip, "OnTooltipSetItem", onSetItem)
  if ItemRefShoppingTooltip1 then hookScript(ItemRefShoppingTooltip1, "OnTooltipSetItem", onSetItem) end
  if ItemRefShoppingTooltip2 then hookScript(ItemRefShoppingTooltip2, "OnTooltipSetItem", onSetItem) end
  if ShoppingTooltip1 then hookScript(ShoppingTooltip1, "OnTooltipSetItem", onSetItem) end
  if ShoppingTooltip2 then hookScript(ShoppingTooltip2, "OnTooltipSetItem", onSetItem) end

  hook(GameTooltip, "SetTradeSkillItem", function(tooltip, skillIndex)
    if GetTradeSkillItemLink then
      local link = GetTradeSkillItemLink(skillIndex)
      if link then
        local id = tonumber(link:match("item:(%d+)"))
        add(tooltip, id, "item")
      end
    end
  end)

  hook(GameTooltip, "SetTradeSkillReagent", function(tooltip, skillIndex, reagentIndex)
    if GetTradeSkillReagentItemLink then
      local link = GetTradeSkillReagentItemLink(skillIndex, reagentIndex)
      if link then
        local id = tonumber(link:match("item:(%d+)"))
        add(tooltip, id, "item")
      end
    end
  end)

  if GameTooltip.SetGlyph then
    hook(GameTooltip, "SetGlyph", function(tooltip, index, talentGroup)
      if GetGlyphLink then
        local link = GetGlyphLink(index, talentGroup)
        if link then
          local id = tonumber(link:match("item:(%d+)"))
          add(tooltip, id, "item")
        end
      end
    end)
  end

  if GameTooltip.SetTalent then
    hook(GameTooltip, "SetTalent", function(tooltip, tabIndex, talentIndex)
      if GetTalentLink then
        local link = GetTalentLink(tabIndex, talentIndex)
        if link then
          local id = tonumber(link:match("spell:(%d+)"))
          add(tooltip, id, "spell")
        end
      end
    end)
  end
end

-------------------------------------------------------------------------------
-- Achievements (WotLK Achievement UI)
-------------------------------------------------------------------------------

local function achievementOnEnter(btn)
  if not btn or not btn.id then return end
  GameTooltip:SetOwner(btn, "ANCHOR_NONE")
  GameTooltip:SetPoint("TOPLEFT", btn, "TOPRIGHT", 0, 0)
  add(GameTooltip, btn.id, "achievement")
  GameTooltip:Show()
end

local function criteriaOnEnter(index)
  return function(frame)
    local btn = frame and frame.GetParent and frame:GetParent() and frame:GetParent():GetParent()
    if not btn or not btn.id or not GetAchievementCriteriaInfo then return end
    local criteriaId = select(9, GetAchievementCriteriaInfo(btn.id, frame.___index or index))
    if criteriaId then
      if not GameTooltip:IsVisible() then
        GameTooltip:SetOwner(btn:GetParent(), "ANCHOR_NONE")
      end
      GameTooltip:SetPoint("TOPLEFT", btn, "TOPRIGHT", 0, 0)
      add(GameTooltip, btn.id, "achievement")
      add(GameTooltip, criteriaId, "criteria")
      GameTooltip:Show()
    end
  end
end

local function hookAchievementUI()
  if not AchievementFrameAchievementsContainer or not AchievementFrameAchievementsContainer.buttons then return end

  for _, button in ipairs(AchievementFrameAchievementsContainer.buttons) do
    hookScript(button, "OnEnter", achievementOnEnter)
    hookScript(button, "OnLeave", GameTooltip_Hide)
  end

  local hooked = {}
  hook(_G, "AchievementButton_GetCriteria", function(index, renderOffScreen)
    local frame = _G["AchievementFrameCriteria" .. (renderOffScreen and "OffScreen" or "") .. index]
    if frame and not hooked[frame] then
      hookScript(frame, "OnEnter", criteriaOnEnter(index))
      hookScript(frame, "OnLeave", GameTooltip_Hide)
      hooked[frame] = true
    end
  end)
end

-------------------------------------------------------------------------------
-- Quest log
-------------------------------------------------------------------------------

local function addQuestLogIndex(tooltip, questIndex)
  if not questIndex or not GetQuestLogTitle then return end
  local isHeader = select(5, GetQuestLogTitle(questIndex))
  if isHeader then return end
  -- 3.3.5: title, level, questTag, suggestedGroup, isHeader, isCollapsed, isComplete, isDaily, questId
  local questId = select(9, GetQuestLogTitle(questIndex))
  if questId and questId > 0 then
    add(tooltip, questId, "quest")
  end
end

local function hookQuestLog()
  if QuestLogScrollFrame and QuestLogScrollFrame.buttons then
    for _, button in ipairs(QuestLogScrollFrame.buttons) do
      hookScript(button, "OnEnter", function(self)
        if not self or not self.GetID then return end
        local questIndex = self:GetID() + FauxScrollFrame_GetOffset(QuestLogScrollFrame)
        addQuestLogIndex(GameTooltip, questIndex)
      end)
    end
  end

  -- Fallback for default UI handler.
  if QuestLogTitleButton_OnEnter then
    hook(_G, "QuestLogTitleButton_OnEnter", function(self)
      if not self or not self.GetID or not QuestLogScrollFrame then return end
      local questIndex = self:GetID() + FauxScrollFrame_GetOffset(QuestLogScrollFrame)
      addQuestLogIndex(GameTooltip, questIndex)
    end)
  end
end

hook(GameTooltip, "SetQuestLogItem", function(tooltip, itemType, index)
  if GetQuestLogItemLink then
    local link = GetQuestLogItemLink(itemType, index)
    if link then
      local id = tonumber(link:match("item:(%d+)"))
      add(tooltip, id, "item")
    end
  end
  if GetQuestLogTitle then
    addQuestLogIndex(tooltip, GetQuestLogSelection())
  end
end)

hook(GameTooltip, "SetQuestItem", function(tooltip, itemType, index)
  if GetQuestItemLink then
    local link = GetQuestItemLink(itemType, index)
    if link then
      local id = tonumber(link:match("item:(%d+)"))
      add(tooltip, id, "item")
    end
  end
end)

-------------------------------------------------------------------------------
-- Events / config
-------------------------------------------------------------------------------

local f = CreateFrame("Frame")
f:RegisterEvent("ADDON_LOADED")
f:RegisterEvent("PLAYER_LOGIN")
f:SetScript("OnEvent", function(_, event, addon)
  if event == "ADDON_LOADED" and addon == addonName then
    local defaults = { enabled = true }

    if not idTipConfig then idTipConfig = {} end

    for key, value in pairs(defaults) do
      if type(idTipConfig[key]) ~= type(value) then
        idTipConfig[key] = value
      end
    end

    for key in pairs(kinds) do
      if type(idTipConfig[configKey(key)]) ~= "boolean" then
        idTipConfig[configKey(key)] = true
      end
    end
  elseif event == "PLAYER_LOGIN" then
    installTooltipHooks()
    hookQuestLog()
  elseif event == "ADDON_LOADED" then
    if addon == "Blizzard_AchievementUI" then
      hookAchievementUI()
    end

    if addon == "Blizzard_AchievementUI" or (QuestLogFrame and QuestLogFrame:IsShown()) then
      hookQuestLog()
    end
  end
end)

-- Quest log may load before our addon; hook when frame is shown.
if QuestLogFrame then
  hookScript(QuestLogFrame, "OnShow", hookQuestLog)
end

-------------------------------------------------------------------------------
-- Interface options (3.3.5)
-------------------------------------------------------------------------------

local panel = CreateFrame("Frame")
panel.name = addonName
panel:Hide()

panel:SetScript("OnShow", function()
  local function createCheckbox(label, key)
    local checkBox = CreateFrame("CheckButton", addonName .. "Check" .. label, panel, "InterfaceOptionsCheckButtonTemplate")
    checkBox:SetChecked(idTipConfig[key])
    checkBox:SetScript("OnClick", function(self)
      idTipConfig[key] = self:GetChecked() and true or false
    end)
    checkBox.Text:SetText(label)
    return checkBox
  end

  local title = panel:CreateFontString("ARTWORK", nil, "GameFontNormalLarge")
  title:SetPoint("TOPLEFT", 16, -16)
  title:SetText(addonName .. " (WotLK)")

  local enabledCheckBox = createCheckbox("Enabled", "enabled")
  enabledCheckBox:SetPoint("TOPLEFT", title, "BOTTOMLEFT", 0, -16)

  local kindsTitle = panel:CreateFontString("ARTWORK", nil, "GameFontNormal")
  kindsTitle:SetPoint("TOPLEFT", enabledCheckBox, "BOTTOMLEFT", 0, -16)
  kindsTitle:SetText("Types")

  local index = 0
  local rowHeight = 24
  local columnWidth = 150
  local rowNum = 7

  local keys = {}
  for key in pairs(kinds) do keys[#keys + 1] = key end
  table.sort(keys)

  for _, key in ipairs(keys) do
    local checkBox = createCheckbox(kinds[key], configKey(key))
    local columnIndex = math.floor(index / rowNum)
    local offsetRight = columnIndex * columnWidth
    local offsetUp = -(index * rowHeight) + (rowHeight * rowNum * columnIndex) - 16
    checkBox:SetPoint("TOPLEFT", kindsTitle, "BOTTOMLEFT", offsetRight, offsetUp)
    index = index + 1
  end

  panel:SetScript("OnShow", nil)
end)

if InterfaceOptions_AddCategory then
  InterfaceOptions_AddCategory(panel)
end

SLASH_IDTIP1 = "/idtip"
SlashCmdList.IDTIP = function()
  if InterfaceOptionsFrame_OpenToCategory then
    InterfaceOptionsFrame_OpenToCategory(panel)
    InterfaceOptionsFrame_OpenToCategory(panel)
  end
end
