SuperMenu = SuperMenu or {}

SMU_PREFIX = "SMU"
SMU_DEBUG = false

function SMU_Log(msg)
    if SMU_DEBUG then
        print("|cff66ccff[超级菜单]|r " .. tostring(msg))
    end
end

function ExtractSMUPayload(arg1, arg2)
    if arg1 == SMU_PREFIX then
        return arg2 or ""
    end
    if type(arg2) == "string" and arg2 ~= "" then
        local body = arg2:match("^\t" .. SMU_PREFIX .. "\t(.+)$")
        if body then return body end
        body = arg2:match("^" .. SMU_PREFIX .. "\t(.+)$")
        if body then return body end
    end
    if type(arg1) == "string" and arg1:find("^" .. SMU_PREFIX) then
        return arg1:match("^" .. SMU_PREFIX .. "\t(.+)$") or arg1:match("^\t" .. SMU_PREFIX .. "\t(.+)$")
    end
    return nil
end

function SendSMU(msg)
    local name = UnitName("player")
    if not name then return end
    SMU_Log("发送 -> " .. msg)
    SendAddonMessage(SMU_PREFIX, msg, "WHISPER", name)
end

function SendSMUAction(act)
    SendSMU("ACT;" .. act)
end

function SendSMUTeleport(mapId, x, y, z, o)
    SendSMU(string.format("TP;%d;%.2f;%.2f;%.2f;%.2f", mapId, x, y, z, o or 0))
end

function SendSMUQuestList()
    SendSMU("Q_LIST")
end

function SendSMUQuestReset(ids)
    if not ids or #ids == 0 then
        print("|cffff0000超级菜单:|r 请先勾选要重置的任务。")
        return
    end
    SendSMU("Q_RESET;" .. table.concat(ids, ","))
end

function SendSMUQuestAdd(questId)
    questId = tonumber(questId)
    if not questId or questId <= 0 then
        print("|cffff0000超级菜单:|r 请输入有效任务 ID。")
        return
    end
    SendSMU("Q_ADD;" .. questId)
end

function SendSMUSummon(entry)
    SendSMU("SUMMON;" .. entry)
end

function SendSMUEnchant(enchantId, equipSlot)
    SendSMU("ENC;" .. tostring(enchantId) .. ";" .. tostring(equipSlot))
end

SuperMenu.questRows = SuperMenu.questRows or {}
SuperMenu.questSelected = SuperMenu.questSelected or {}
SuperMenu.bookmarks = SuperMenu.bookmarks or {}

function SuperMenu_GetBookmarks()
    local key = "SuperMenuMarks_" .. (UnitName("player") or "x")
    SuperMenuDB = SuperMenuDB or {}
    SuperMenuDB[key] = SuperMenuDB[key] or {}
    return SuperMenuDB[key]
end

function SuperMenu_SaveBookmark(slot)
    local b = SuperMenu_GetBookmarks()
    b[slot] = {
        mapId = GetCurrentMapAreaID and 0 or 0, -- 占位，服务端记录更准
    }
    SendSMU("MARK;" .. slot .. ";SET")
end

function SuperMenu_TeleportBookmark(slot)
    local b = SuperMenu_GetBookmarks()
    local mark = b[slot]
    if mark and mark.mapId then
        SendSMU(string.format("MARK;%d;GO;%d;%.2f;%.2f;%.2f;%.2f",
            slot, mark.mapId, mark.x, mark.y, mark.z, mark.o or 0))
    else
        print("|cffff0000超级菜单:|r " .. slot .. " 号坐标未记录，请先点击「记录」。")
    end
end

function SuperMenu_OnMarkStored(slot, mapId, x, y, z, o)
    local b = SuperMenu_GetBookmarks()
    b[slot] = { mapId = mapId, x = x, y = y, z = z, o = o }
    print("|cff00ff00超级菜单:|r 已保存 " .. slot .. " 号坐标。")
end

function SuperMenu_ClearQuestUI()
    SuperMenu.questRows = {}
    SuperMenu.questSelected = {}
    if SuperMenu.RefreshQuestScroll then
        SuperMenu.RefreshQuestScroll()
    end
end

function SuperMenu_AddQuestRow(questId, state, title)
    table.insert(SuperMenu.questRows, {
        id = questId,
        state = state,
        title = title,
        key = tostring(questId) .. " " .. (title or ""),
    })
    SuperMenu.questSelected[questId] = false
end

function SuperMenu_HandlePayload(payload)
    if not payload or payload == "" then return end

    if payload == "UI_OPEN" then
        if SuperMenu.Show then
            SuperMenu.Show()
            print("|cffd4b86a奥术魔典|r |cffc8a8ff已翻开。|r")
        end
        return
    end

    if payload == "Q_CLEAR" then
        SuperMenu_ClearQuestUI()
        return
    end

    if payload:match("^Q;") then
        local qid, state, title = payload:match("^Q;(%d+);([^;]*);(.*)$")
        if qid then
            SuperMenu_AddQuestRow(tonumber(qid), state, title)
        end
        return
    end

    if payload:match("^Q_DONE") then
        if SuperMenu.RefreshQuestScroll then SuperMenu.RefreshQuestScroll() end
        local n = payload:match("^Q_DONE;(%d+)") or "0"
        print("|cffd4b86a奥术魔典:|r 已载入 " .. n .. " 条任务残页。")
        return
    end

    if payload:match("^Q_RESET_OK") then
        SendSMUQuestList()
        print("|cff00ff00超级菜单:|r 已重置选中任务，列表已刷新。")
        return
    end

    if payload:match("^MARK;") then
        local slot, mapId, x, y, z, o = payload:match("^MARK;(%d+);(%d+);([^;]+);([^;]+);([^;]+);([^;]+)$")
        if slot then
            SuperMenu_OnMarkStored(tonumber(slot), tonumber(mapId), tonumber(x), tonumber(y), tonumber(z), tonumber(o))
        end
        return
    end
end
