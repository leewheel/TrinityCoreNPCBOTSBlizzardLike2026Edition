local addon, ns = ...

-- ============================================================
-- SPECIAL SLOTS — Ground Mount, Flying Mount, Pet, Combat Pet, Morph Form
-- Vertical column under the Feet slot on the dressing room
-- ============================================================

local mainFrame = ns.mainFrame
mainFrame.specialSlots = {}

local SLOT_SIZE = 37

local function CreateSpecialSlot(slotName, tabIndex, emptyTexture)
    local safeName = slotName:gsub("%s+", "")
    local slot = CreateFrame("Button", "$parentSpecialSlot"..safeName, mainFrame, "ItemButtonTemplate")
    slot:SetSize(SLOT_SIZE, SLOT_SIZE)
    slot:SetFrameLevel(mainFrame.dressingRoom:GetFrameLevel() + 1)
    slot.slotName = slotName
    slot.tabIndex = tabIndex
    slot:RegisterForClicks("LeftButtonUp", "RightButtonUp")

    slot.textures = {}
    slot.textures.empty = slot:CreateTexture(nil, "BACKGROUND")
    slot.textures.empty:SetTexture(emptyTexture)
    slot.textures.empty:SetAllPoints()

    slot.icon = slot:CreateTexture(nil, "ARTWORK")
    slot.icon:SetAllPoints()
    slot.icon:SetTexCoord(0.08, 0.92, 0.08, 0.92)
    slot.icon:Hide()

    slot:SetScript("OnClick", function(self, button)
        if button == "LeftButton" then
            ns.tab_OnClick(mainFrame.buttons["tab"..self.tabIndex])
            PlaySound("gsTitleOptionOK")
        elseif button == "RightButton" then
            if self.slotName == "Mount" then
                if TransmorpherCharacterState then
                    TransmorpherCharacterState.MountDisplay = nil
                    TransmorpherCharacterState.MountName = nil
                    TransmorpherCharacterState.MountHidden = false
                    -- Clean up old variables
                    TransmorpherCharacterState.GroundMountDisplay = nil
                    TransmorpherCharacterState.GroundMountName = nil
                    TransmorpherCharacterState.FlyingMountDisplay = nil
                    TransmorpherCharacterState.FlyingMountName = nil
                end
                if ns.IsMorpherReady() then
                    ns.SendRawMorphCommand("MOUNT_RESET")
                end
                ns.UpdateSpecialSlots()
                if ns.BroadcastMorphState then ns.BroadcastMorphState() end
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：坐骑幻化已清除！")
                PlaySound("gsTitleOptionOK")
            elseif self.slotName == "Pet" then
                if ns.IsMorpherReady() then
                    ns.SendMorphCommand("PET_RESET")
                    ns.UpdateSpecialSlots()
                    SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：宠物幻化已重置！")
                    PlaySound("gsTitleOptionOK")
                end
            elseif self.slotName == "Combat Pet" then
                if ns.IsMorpherReady() then
                    ns.SendMorphCommand("HPET_RESET")
                    ns.UpdateSpecialSlots()
                    SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：战斗宠物幻化已重置！")
                    PlaySound("gsTitleOptionOK")
                end
            elseif self.slotName == "Morph Form" then
                if ns.IsMorpherReady() then
                    ns.SendMorphCommand("MORPH:0")
                    if TransmorpherCharacterState then TransmorpherCharacterState.Morph = nil end
                    ns.UpdatePreviewModel()
                    ns.UpdateSpecialSlots()
                    SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：角色幻化已重置！")
                    PlaySound("gsTitleOptionOK")
                end
            end
        end
    end)

    slot:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT"); GameTooltip:ClearLines()
        if self.name then
            GameTooltip:AddLine(self.name, 1, 1, 1)
            if self.displayID then GameTooltip:AddLine("模型ID: "..self.displayID, 0.7, 0.7, 0.7) end
            GameTooltip:AddLine(" ")
            GameTooltip:AddLine("左键点击：打开对应标签页", 0.5, 0.8, 0.5)
            GameTooltip:AddLine("右键点击：清除幻化", 1.0, 0.5, 0.5)
        else
            local slotNames = {["Mount"]="坐骑", ["Pet"]="宠物", ["Combat Pet"]="战斗宠物", ["Morph Form"]="幻化形态"}
            local dispName = slotNames[self.slotName] or self.slotName
            GameTooltip:AddLine("未设置"..dispName, 0.7, 0.7, 0.7)
            GameTooltip:AddLine(" "); GameTooltip:AddLine("点击打开对应标签页", 0.5, 0.8, 0.5)
        end
        GameTooltip:Show()
    end)
    slot:SetScript("OnLeave", function() GameTooltip:Hide() end)
    return slot
end

mainFrame.specialSlots.Mount = CreateSpecialSlot("Mount", 3, "Interface\\Icons\\Ability_Mount_RidingHorse")
mainFrame.specialSlots.Pet   = CreateSpecialSlot("Pet", 4, "Interface\\Icons\\INV_Box_PetCarrier_01")
mainFrame.specialSlots.CombatPet = CreateSpecialSlot("Combat Pet", 5, "Interface\\Icons\\Ability_Hunter_BeastCall")
mainFrame.specialSlots.MorphForm = CreateSpecialSlot("Morph Form", 6, "Interface\\Icons\\Spell_Shadow_Charm")

-- Position: Single Mount slot centered below Feet slot
mainFrame.specialSlots.Mount:SetPoint("TOP", mainFrame.slots["Feet"], "BOTTOM", 0, -20)
mainFrame.specialSlots.Pet:SetPoint("TOP", mainFrame.specialSlots.Mount, "BOTTOM", 0, -6)
mainFrame.specialSlots.CombatPet:SetPoint("TOP", mainFrame.specialSlots.Pet, "BOTTOM", 0, -4)
mainFrame.specialSlots.MorphForm:SetPoint("TOP", mainFrame.specialSlots.CombatPet, "BOTTOM", 0, -4)
