-- 奥术魔典 · 事件与通信（配合 .超级菜单 / .supermenu）

local frame = CreateFrame("Frame")
frame:RegisterEvent("PLAYER_LOGIN")
frame:RegisterEvent("CHAT_MSG_ADDON")
frame:RegisterEvent("CHAT_MSG_WHISPER")

local function TryDispatchAddon(arg1, arg2)
    local payload = ExtractSMUPayload(arg1, arg2)
    if payload then
        SMU_Log("收到 <- " .. payload:sub(1, 80))
        SuperMenu_HandlePayload(payload)
        return true
    end
    return false
end

frame:SetScript("OnEvent", function(self, event, ...)
    if event == "PLAYER_LOGIN" then
        SuperMenuDB = SuperMenuDB or {}
        if RegisterAddonMessagePrefix then
            RegisterAddonMessagePrefix(SMU_PREFIX)
        end
    elseif event == "CHAT_MSG_ADDON" then
        TryDispatchAddon(...)
    elseif event == "CHAT_MSG_WHISPER" then
        local msg, author = ...
        if author == UnitName("player") or author == "" then
            TryDispatchAddon(SMU_PREFIX, msg)
            if type(msg) == "string" then
                TryDispatchAddon(msg, nil)
            end
        end
    end
end)

SLASH_SUPERMENU1 = "/supermenu"
SLASH_GRIMOIRE1 = "/魔典"
SlashCmdList["SUPERMENU"] = function()
    print("|cffd4b86a奥术魔典:|r 请使用 |cff88ff88.超级菜单|r 或 |cff88ff88.supermenu|r（GM）。")
end
SlashCmdList["GRIMOIRE"] = SlashCmdList["SUPERMENU"]
