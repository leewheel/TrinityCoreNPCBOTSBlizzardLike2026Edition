local addon, ns = ...

-- ============================================================
-- 斜杠命令 — /morph, /vm, /幻化
-- ============================================================

local mainFrame = ns.mainFrame

SLASH_Transmorpher1 = "/morph"
SLASH_Transmorpher2 = "/vm"
SLASH_Transmorpher3 = "/Transmorpher"

local function PrintHelp()
    local lines = {
        "|cffF5C842--- 幻化命令 ---|r",
        "|cffffff00/morph|r - 打开/关闭幻化窗口",
        "|cffffff00/morph reset|r - 重置所有幻化",
        "|cffffff00/morph status|r - 显示DLL和幻化状态",
        "|cffffff00/morph morph <模型ID>|r - 将种族幻化为指定模型ID",
        "|cffffff00/morph scale <数值>|r - 设置角色缩放比例 (0.1-10)",
        "|cffffff00/morph mount <模型ID>|r - 将坐骑幻化为指定模型ID",
        "|cffffff00/morph pet <模型ID>|r - 将宠物幻化为指定模型ID",
        "|cffffff00/morph hpet <模型ID>|r - 将战斗宠物幻化为指定模型ID",
        "|cffffff00/morph enchant <mh|oh> <附魔ID>|r - 应用附魔视觉效果",
        "|cffffff00/morph title <称号ID>|r - 应用称号",
        "|cffffff00/morph sync|r - 强制广播状态给其他玩家",
        "|cffffff00/morph export|r - 导出选中的配装字符串 (Ctrl+C)",
        "|cffffff00/morph import <字符串>|r - 导入TM1配装字符串",
        "|cffffff00/morph help|r - 显示此帮助",
    }
    for _, line in ipairs(lines) do
        SELECTED_CHAT_FRAME:AddMessage(line)
    end
end

local function PrintStatus()
    local dllStatus = TRANSMORPHER_DLL_LOADED and "|cff00ff00已加载|r" or "|cffff0000未加载|r"
    local hookStatus = "未知"
    if TRANSMORPHER_DLL_STATUS then
        hookStatus = TRANSMORPHER_DLL_STATUS.hooks and "|cff00ff00正常|r" or "|cffff6600未安装|r"
    end

    SELECTED_CHAT_FRAME:AddMessage("|cffF5C842--- 幻化状态 ---|r")
    SELECTED_CHAT_FRAME:AddMessage("  DLL：" .. dllStatus)
    SELECTED_CHAT_FRAME:AddMessage("  钩子：" .. hookStatus)

    -- 当前幻化状态
    if TransmorpherCharacterState then
        local s = TransmorpherCharacterState
        local morphCount = 0
        if s.Items then for _ in pairs(s.Items) do morphCount = morphCount + 1 end end

        SELECTED_CHAT_FRAME:AddMessage("  种族幻化：" .. (s.Morph and ("|cff00ff00" .. s.Morph .. "|r") or "|cff888888无|r"))
        SELECTED_CHAT_FRAME:AddMessage("  缩放：" .. (s.Scale and ("|cff00ff00" .. s.Scale .. "|r") or "|cff8888881.0|r"))
        SELECTED_CHAT_FRAME:AddMessage("  物品幻化：|cff00ff00" .. morphCount .. "|r 个槽位")
        SELECTED_CHAT_FRAME:AddMessage("  坐骑：" .. (s.MountDisplay and ("|cff00ff00" .. s.MountDisplay .. "|r") or "|cff888888无|r")
            .. (s.MountHidden and " |cffd676ff(已隐藏)|r" or ""))
        SELECTED_CHAT_FRAME:AddMessage("  宠物：" .. (s.PetDisplay and ("|cff00ff00" .. s.PetDisplay .. "|r") or "|cff888888无|r"))
        SELECTED_CHAT_FRAME:AddMessage("  战斗宠物：" .. (s.HunterPetDisplay and ("|cff00ff00" .. s.HunterPetDisplay .. "|r") or "|cff888888无|r")
            .. (s.HunterPetScale and s.HunterPetScale ~= 1.0 and (" 缩放:" .. s.HunterPetScale) or ""))
        SELECTED_CHAT_FRAME:AddMessage("  附魔：主手=" .. (s.EnchantMH or "无") .. " 副手=" .. (s.EnchantOH or "无"))
        SELECTED_CHAT_FRAME:AddMessage("  称号：" .. (s.TitleID and ("|cff00ff00" .. s.TitleID .. "|r") or "|cff888888无|r"))
    else
        SELECTED_CHAT_FRAME:AddMessage("  未保存幻化状态")
    end

    -- 同行玩家数
    local peerCount = ns.P2PGetPeerCount and ns.P2PGetPeerCount() or 0
    SELECTED_CHAT_FRAME:AddMessage("  同步的玩家：|cff00ff00" .. peerCount .. "|r")
end

SlashCmdList["Transmorpher"] = function(msg)
    msg = msg:trim()
    local cmd, rest = msg:match("^(%S+)%s*(.*)")
    if cmd then cmd = cmd:lower() else cmd = msg:lower() end

    if cmd == "reset" then
        if ns.IsMorpherReady() then
            ns.SendMorphCommand("RESET:ALL")
            -- 清除坐骑幻化
            if TransmorpherCharacterState then
                TransmorpherCharacterState.GroundMountDisplay = nil
                TransmorpherCharacterState.GroundMountName = nil
                TransmorpherCharacterState.FlyingMountDisplay = nil
                TransmorpherCharacterState.FlyingMountName = nil
                TransmorpherCharacterState.MountDisplay = nil
                TransmorpherCharacterState.MountHidden = false
                if TransmorpherCharacterState.Mounts then
                    wipe(TransmorpherCharacterState.Mounts)
                end
            end
            ns.SendRawMorphCommand("MOUNT_RESET")
            -- 清除所有已幻化的槽位状态
            if mainFrame.slots then
                for _, slotName in pairs(ns.slotOrder) do
                    local slot = mainFrame.slots[slotName]
                    if slot then
                        slot.isMorphed = false; slot.morphedItemId = nil; slot.isHiddenSlot = false
                        ns.HideMorphGlow(slot)
                        if slot.eyeButton then
                            slot.eyeButton.isHidden = false
                            if slot.eyeButton.UpdateVisuals then
                                slot.eyeButton:UpdateVisuals()
                            end
                        end
                        local equippedId = ns.GetEquippedItemForSlot(slotName)
                        if equippedId then slot:SetItem(equippedId)
                        else slot.itemId = nil; slot.textures.empty:Show(); slot.textures.item:Hide() end
                    end
                end
            end
            if mainFrame.enchantSlots then
                for _, es in pairs(mainFrame.enchantSlots) do
                    es.isMorphed = false; es:RemoveEnchant(); ns.HideMorphGlow(es)
                end
            end
            ns.SyncDressingRoom()

            if ns.BroadcastMorphState then
                ns.BroadcastMorphState(true)
            end

            SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：所有幻化已重置！")
        else
            SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：|cffff0000DLL未加载！|r 请将dinput8.dll放入您的WoW目录。")
        end

    elseif cmd == "status" then
        PrintStatus()

    elseif cmd == "help" then
        PrintHelp()

    elseif cmd == "morph" then
        local id = tonumber(rest)
        if id and id > 0 then
            if ns.IsMorpherReady() then
                ns.SendMorphCommand("MORPH:" .. id)
                if TransmorpherCharacterState then TransmorpherCharacterState.Morph = id end
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：种族已幻化为模型ID " .. id)
            else
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：|cffff0000DLL未加载！|r")
            end
        else
            SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：用法：/morph morph <模型ID>")
        end

    elseif cmd == "scale" then
        local val = tonumber(rest)
        if val and val >= 0.0 and val <= 10.0 then
            if ns.IsMorpherReady() then
                ns.SendMorphCommand("SCALE:" .. val)
                if val == 0 then
                    SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：缩放已重置为默认")
                else
                    SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：缩放已设为 " .. val)
                end
            else
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：|cffff0000DLL未加载！|r")
            end
        else
            SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：用法：/morph scale <数值> (输入0恢复默认)")
        end

    elseif cmd == "mount" then
        local id = tonumber(rest)
        if id and id > 0 then
            if ns.IsMorpherReady() then
                ns.SendMorphCommand("MOUNT_MORPH:" .. id)
                if ns.UpdateSpecialSlots then ns.UpdateSpecialSlots() end
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：坐骑已幻化为模型ID " .. id)
            else
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：|cffff0000DLL未加载！|r")
            end
        else
            SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：用法：/morph mount <模型ID>")
        end

    elseif cmd == "pet" then
        local id = tonumber(rest)
        if id and id > 0 then
            if ns.IsMorpherReady() then
                ns.SendMorphCommand("PET_MORPH:" .. id)
                if ns.UpdateSpecialSlots then ns.UpdateSpecialSlots() end
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：宠物已幻化为模型ID " .. id)
            else
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：|cffff0000DLL未加载！|r")
            end
        else
            SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：用法：/morph pet <模型ID>")
        end

    elseif cmd == "hpet" then
        local id = tonumber(rest)
        if id and id > 0 then
            if ns.IsMorpherReady() then
                ns.SendMorphCommand("HPET_MORPH:" .. id)
                ns.SendMorphCommand("HPET_SCALE:1.0")
                if TransmorpherCharacterState then TransmorpherCharacterState.HunterPetScale = 1.0 end
                if ns.UpdateSpecialSlots then ns.UpdateSpecialSlots() end
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：战斗宠物已幻化为模型ID " .. id)
            else
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：|cffff0000DLL未加载！|r")
            end
        else
            SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：用法：/morph hpet <模型ID>")
        end

    elseif cmd == "enchant" then
        local slot, id = rest:match("^(%S+)%s+(%d+)")
        id = tonumber(id)
        if slot and id and id > 0 then
            slot = slot:lower()
            if slot == "mh" then
                if ns.IsMorpherReady() then
                    ns.SendMorphCommand("ENCHANT_MH:" .. id)
                    if ns.ScheduleDressingRoomSync then ns.ScheduleDressingRoomSync(0.05) end
                    SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：主手附魔已设为 " .. id)
                end
            elseif slot == "oh" then
                if ns.IsMorpherReady() then
                    ns.SendMorphCommand("ENCHANT_OH:" .. id)
                    if ns.ScheduleDressingRoomSync then ns.ScheduleDressingRoomSync(0.05) end
                    SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：副手附魔已设为 " .. id)
                end
            else
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：用法：/morph enchant <mh|oh> <附魔ID>")
            end
        else
            SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：用法：/morph enchant <mh|oh> <附魔ID>")
        end

    elseif cmd == "title" then
        local id = tonumber(rest)
        if id and id > 0 then
            if ns.IsMorpherReady() then
                ns.SendMorphCommand("TITLE:" .. id)
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：称号已设为ID " .. id)
            else
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：|cffff0000DLL未加载！|r")
            end
        else
            SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：用法：/morph title <称号ID>")
        end

    elseif cmd == "sync" then
        if ns.BroadcastMorphState then
            ns.BroadcastMorphState()
            SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：状态已广播给其他玩家")
        else
            SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：P2P同步不可用")
        end

    elseif cmd == "export" then
        if ns.ShowExportLoadoutDialog then
            ns.ShowExportLoadoutDialog()
        else
            SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：请先打开配装标签页。")
        end

    elseif cmd == "import" then
        if rest and rest ~= "" then
            if ns.ImportLoadoutString then
                ns.ImportLoadoutString(rest, false)
            else
                SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：请先打开配装标签页。")
            end
        elseif ns.ShowImportLoadoutDialog then
            ns.ShowImportLoadoutDialog()
        else
            SELECTED_CHAT_FRAME:AddMessage("|cffF5C842<幻化>|r：用法：/morph import <TM1导出字符串>")
        end

    else
        if mainFrame:IsShown() then mainFrame:Hide() else mainFrame:Show() end
    end
end
