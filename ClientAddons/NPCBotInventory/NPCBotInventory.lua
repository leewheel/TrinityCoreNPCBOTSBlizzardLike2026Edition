-- NPCBotInventory.lua - 事件、斜杠命令与钩子

-- [[ EVENTS ]] --
local Scanner = CreateFrame("GameTooltip", "BMUScannerTooltip", nil, "GameTooltipTemplate")

local isModelRefreshPending = false
local refreshFrame = CreateFrame("Frame")
refreshFrame:SetScript("OnUpdate", function(self)
    if isModelRefreshPending then
        isModelRefreshPending = false
        if MainFrame.selectedBot and MainFrame:IsShown() then
            SelectBot(MainFrame.selectedBot)
        end
    end
end)

MainFrame:RegisterEvent("CHAT_MSG_ADDON")
MainFrame:RegisterEvent("PLAYER_LOGIN")
MainFrame:RegisterEvent("GET_ITEM_INFO_RECEIVED")
MainFrame:RegisterEvent("PLAYER_TARGET_CHANGED")

MainFrame:SetScript("OnEvent", function(self, event, arg1, arg2)
    if event == "PLAYER_LOGIN" then
        playerName = UnitName("player")
        BotInventoryDB = BotInventoryDB or {}
        EnsureTemplateDatabase()
        templatesDB = nil

        BotInventoryDB[playerName] = BotInventoryDB[playerName] or {}
        db = BotInventoryDB[playerName]

        db.minimap = db.minimap or { angle = 45 }
        if db.hideHelmet == nil then db.hideHelmet = false end
        if db.showStats == nil then db.showStats = true end -- Ensure stats show default correctly

        db.templates = nil

        -- 3.3.5 无 RegisterAddonMessagePrefix，直接 SendAddonMessage 即可
        if RegisterAddonMessagePrefix then
            RegisterAddonMessagePrefix(BMU_PREFIX)
        end
        RegisterBotManagerHotkey()
        RegisterBotManagerOptions()
        UpdateMinimapPosition() 
        UpdateMinimapVisibility()
        UpdateHideHelmetVisual()
        RefreshBotList()

        
    elseif event == "PLAYER_TARGET_CHANGED" then
        if MainFrame.selectedBot and db[MainFrame.selectedBot] then
            local data = db[MainFrame.selectedBot]
            if UnitExists("target") and UnitName("target") == data.name then
                UpdateBotModel(MainFrame.selectedBot)
            end
        end
        
    elseif event == "GET_ITEM_INFO_RECEIVED" then
        isModelRefreshPending = true
        
    elseif event == "CHAT_MSG_ADDON" then
        local payload = ExtractBMUPayload(arg1, arg2)
        if not payload or payload == "" then
            if BMU_DEBUG and (arg1 or arg2) then
                local a2 = type(arg2) == "string" and arg2:sub(1, 72) or tostring(arg2)
                BMU_Log(string.format("忽略非BMU包 arg1='%s' arg2='%s'", tostring(arg1), a2))
            end
            return
        end

        if BMU_DEBUG then
            local preview = #payload > 64 and (payload:sub(1, 64) .. "…") or payload
            BMU_Log("收到 <- " .. preview)
        end

        local parts = SplitBMUPayload(payload)
        
        if parts[1] == "B" then
            bmuDebugStats.B = bmuDebugStats.B + 1
            local entry = tonumber(parts[2])
            local className = NormalizeClassName(parts[5])
            -- Always replace the entire record fresh to avoid stale race/gender from a previous load
            db[entry] = {
                entry   = entry,
                name    = parts[3],
                roles   = tonumber(parts[4]) or 0,
                className = className,
                displayId = tonumber(parts[6]) or 0,
                race    = tonumber(parts[7]) or 0,
                gender  = tonumber(parts[8]) or 0,
                level   = tonumber(parts[9]) or 0,
                spec    = tonumber(parts[10]) or 0,
                gear    = {},
            }
            
        elseif parts[1] == "G" then
            bmuDebugStats.G = bmuDebugStats.G + 1
            local entry = tonumber(parts[2])
            local slot = parts[3]
            local itemID = tonumber(parts[4])
            if db[entry] then
                if BMU_HasExtendedGearPacket(parts) then
                    db[entry].gear[slot] = MakeGearStateFromPacket(parts)
                else
                    db[entry].gear[slot] = { id = itemID or 0 }
                end
                local gearState = db[entry].gear[slot]
                local preloadLink = gearState and BuildGearItemLink(gearState)
                if preloadLink and not GetItemInfo(preloadLink) then
                    BMUScannerTooltip:SetOwner(WorldFrame, "ANCHOR_NONE")
                    BMUScannerTooltip:SetHyperlink(preloadLink)
                end
            elseif BMU_DEBUG then
                bmuDebugStats.dropG = bmuDebugStats.dropG + 1
                BMU_Log(string.format("G 包丢弃(尚无B): entry=%s slot=%s id=%s", tostring(entry), tostring(slot), tostring(itemID)))
            end
            
        elseif parts[1] == "S" then
            bmuDebugStats.S = bmuDebugStats.S + 1
            local entry = tonumber(parts[2])
            if db[entry] then
                if parts[28] then
                    db[entry].stats = {
                        hp = tonumber(parts[3]) or 0,
                        mp = tonumber(parts[4]) or 0,
                        str = tonumber(parts[5]) or 0,
                        agi = tonumber(parts[6]) or 0,
                        sta = tonumber(parts[7]) or 0,
                        int = tonumber(parts[8]) or 0,
                        spi = tonumber(parts[9]) or 0,
                        armor = tonumber(parts[10]) or 0,
                        def = tonumber(parts[11]) or 0,
                        resHoly = tonumber(parts[12]) or 0,
                        resFire = tonumber(parts[13]) or 0,
                        resNature = tonumber(parts[14]) or 0,
                        resFrost = tonumber(parts[15]) or 0,
                        resShadow = tonumber(parts[16]) or 0,
                        resArcane = tonumber(parts[17]) or 0,
                        block = tonumber(parts[18]) or 0,
                        dodge = tonumber(parts[19]) or 0,
                        parry = tonumber(parts[20]) or 0,
                        crit = tonumber(parts[21]) or 0,
                        ap = tonumber(parts[22]) or 0,
                        sp = tonumber(parts[23]) or 0,
                        spellPen = tonumber(parts[24]) or 0,
                        haste = tonumber(parts[25]) or 0,
                        hit = tonumber(parts[26]) or 0,
                        expertise = tonumber(parts[27]) or 0,
                        arpen = tonumber(parts[28]) or 0
                    }
                else
                    db[entry].stats = {
                        hp = tonumber(parts[3]) or 0,
                        mp = tonumber(parts[4]) or 0,
                        str = tonumber(parts[5]) or 0,
                        agi = tonumber(parts[6]) or 0,
                        sta = tonumber(parts[7]) or 0,
                        int = tonumber(parts[8]) or 0,
                        spi = tonumber(parts[9]) or 0,
                        armor = tonumber(parts[10]) or 0,
                        def = tonumber(parts[11]) or 0,
                        ap = tonumber(parts[12]) or 0,
                        sp = tonumber(parts[13]) or 0,
                        crit = tonumber(parts[14]) or 0,
                        haste = tonumber(parts[15]) or 0,
                        arpen = tonumber(parts[16]) or 0
                    }
                end
                if MainFrame.selectedBot == entry then
                    UpdateStatsPanel(entry)
                end
            end
            
        elseif parts[1] == "U" then
            -- By leewheel 20260528 - server gossip: open inspect UI after snapshot (U;OPEN;entry)
            if parts[2] == "OPEN" and parts[3] then
                local entry = tonumber(parts[3])
                if entry and NPCBotInventory_OpenInspect then
                    NPCBotInventory_OpenInspect(entry)
                end
            end

        elseif parts[1] == "E" then
            bmuDebugStats.E = bmuDebugStats.E + 1
            -- If it's a REFRESH, ask the server for the new data.
            if parts[2] == "REFRESH" then
                BMU_ResetDebugStats()
                BMU_ClearBotListDb()
                SendBMUMessage("REFRESH")
            end
            
            if parts[2] == "END" or parts[2] == "REFRESH" or not parts[2] then
                local botCount = BMU_CountBotsInDb()
                print(string.format("|cff66ccff机器人管理:|r 同步完成 — 列表 %d 个机器人 (收包 B=%d G=%d S=%d)",
                    botCount, bmuDebugStats.B, bmuDebugStats.G, bmuDebugStats.S))
                if BMU_DEBUG then
                    BMU_Log(string.format("E;%s 完成 db=%d dropG=%d selected=%s",
                        tostring(parts[2]), botCount, bmuDebugStats.dropG, tostring(MainFrame.selectedBot)))
                end
            end
            
            -- E|END/E|REFRESH means all B| and G| packets have arrived.
            if MainFrame.inspectMode then
                local inspectEntry = pendingInspectEntry or MainFrame.selectedBot
                if inspectEntry and db[inspectEntry] then
                    SelectBot(inspectEntry)
                end
                pendingInspectEntry = nil
            else
                RefreshBotList(GetBotListSearchFilter())
                -- Re-render the selected bot model with now-complete race/gear data
                if MainFrame.selectedBot and db[MainFrame.selectedBot] then
                    SelectBot(MainFrame.selectedBot)
                end
            end
            -- Print any pending confirmation message
            if pendingAction then
                print("|cff00ff00机器人管理:|r " .. pendingAction)
                pendingAction = nil
            end
        end
    end
end)

MainFrame:SetScript("OnShow", function()
    BMU_ResetDebugStats()
    if MainFrame.inspectMode and MainFrame.selectedBot then
        SendBMUMessage("QUERY;" .. MainFrame.selectedBot)
    else
        BMU_ClearBotListDb()
        SendBMUMessage("REFRESH")
    end
    if BMU_DEBUG then
        BMU_Log("窗口打开，已发 REFRESH/QUERY")
    end
end)

SLASH_BOTMGR1 = "/bm"
SLASH_BOTMGR2 = "/bmdebug"
SlashCmdList["BOTMGR"] = function(msg)
    if strtrim then
        msg = strtrim(msg or "")
    else
        msg = (msg or ""):match("^%s*(.-)%s*$")
    end
    if msg == "debug" or msg == "log" then
        BMU_DEBUG = not BMU_DEBUG
        print("|cff66ccff机器人管理:|r BMU 调试日志 " .. (BMU_DEBUG and "已开启" or "已关闭") .. "（/bm debug 切换）")
        if BMU_DEBUG then
            BMU_Log("playerName=" .. tostring(playerName) .. " dbBots=" .. BMU_CountBotsInDb())
        end
        return
    end
    ToggleBotManagerWindows()
end
