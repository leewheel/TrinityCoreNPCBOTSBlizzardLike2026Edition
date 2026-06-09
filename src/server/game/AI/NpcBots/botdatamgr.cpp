#include "BattlegroundMgr.h"
#include "BattlegroundQueue.h"
#include "bot_ai.h"
#include "bot_chat_llm.h"
#ifdef TRINITY_NPCBOT_LLM_EMBED
#include "llama.h"
#endif
#include "botconfig.h"
#include "botdatamgr.h"
#include "botgearscore.h"
#include "botlog.h"
#include "botmgr.h"
#include "botspell.h"
#include "bottext.h"
#include "botwanderful.h"
#include "bot_wanderer_vendor.h"
#include "bpet_ai.h"
#include "CharacterCache.h"
#include "Containers.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "ChatPackets.h"
#include "GameTime.h"
#include "Group.h"
#include "GroupMgr.h"
#include "Item.h"
#include "LFGMgr.h"
#include "Log.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include "StringConvert.h"
#include "World.h"
#include "WorldDatabase.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <deque>
#include <filesystem>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

namespace
{
static uint32 GetProcessWorkingSetMB()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc = {};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return uint32(pmc.WorkingSetSize / (1024u * 1024u));
#endif
    return 0;
}

static std::string FormatUptimeDuration(uint32 uptimeSec)
{
    uint32 const hours = uptimeSec / 3600u;
    uint32 const mins = (uptimeSec % 3600u) / 60u;
    uint32 const secs = uptimeSec % 60u;
    return Bcore::StringFormat("{}h{}m{}s", hours, mins, secs);
}

struct WandererPeriodicStats
{
    uint32 gridRecycleTotal = 0;
    std::unordered_map<uint32, uint32> gridRecycleByMap;
    uint32 replenishQueued = 0;

    void RecordGridRecycle(uint32 mapId, uint32 count)
    {
        gridRecycleTotal += count;
        gridRecycleByMap[mapId] += count;
    }

    void RecordReplenishQueued(uint32 count) { replenishQueued += count; }

    void Clear() { gridRecycleTotal = 0; gridRecycleByMap.clear(); replenishQueued = 0; }
};

static WandererPeriodicStats s_wandererPeriodStats;

static std::string FormatWandererPeriodStats()
{
    if (!s_wandererPeriodStats.gridRecycleTotal && !s_wandererPeriodStats.replenishQueued)
        return " 周期统计: 格子回收 0 | 补员排队 0";

    std::string line = Bcore::StringFormat(" 周期统计: 格子回收 {} (", s_wandererPeriodStats.gridRecycleTotal);
    bool first = true;
    for (uint32 mapId : BotCfg::GetWanderContinentMapIds())
    {
        auto itr = s_wandererPeriodStats.gridRecycleByMap.find(mapId);
        if (itr == s_wandererPeriodStats.gridRecycleByMap.end() || !itr->second)
            continue;
        if (!first)
            line += ", ";
        line += Bcore::StringFormat("map{}:{}", mapId, itr->second);
        first = false;
    }
    if (first)
        line += "-";
    line += Bcore::StringFormat(") | 补员排队 {} | 空图阈值 {}s",
        s_wandererPeriodStats.replenishQueued, BotCfg::GetWandererGridEmptyMapIdleSec());
    return line;
}
}
/*
Npc Bot Data Manager by Trickerer (onlysuffering@gmail.com)
NpcBots DB Data management
%Complete: ???
*/

#ifdef _MSC_VER
# pragma warning(push, 4)
#endif

using namespace std::string_view_literals;

using NpcBotMgrDataMap = std::unordered_map<ObjectGuid /*player_guid*/, NpcBotMgrData>;
using NpcBotDataMap = std::unordered_map<uint32 /*entry*/, NpcBotData>;
using NpcBotAppearanceDataMap = std::unordered_map<uint32 /*entry*/, NpcBotAppearanceData>;
using NpcBotExtrasMap = std::unordered_map<uint32 /*entry*/, NpcBotExtras>;
using NpcBotTransmogDataMap = std::unordered_map<uint32 /*entry*/, NpcBotTransmogData>;
static NpcBotMgrDataMap _botMgrsData;
static NpcBotDataMap _botsData;
static NpcBotAppearanceDataMap _botsAppearanceData;
static NpcBotExtrasMap _botsExtras;
static NpcBotTransmogDataMap _botsTransmogData;
static NpcBotRegistry _existingBots;

static std::map<uint32, uint8> _wpMinSpawnLevelPerMapId;
static std::map<uint32, uint8> _wpMaxSpawnLevelPerMapId;
static std::map<uint8, std::set<uint32>> _spareBotIdsPerClassMap;
static std::unordered_map<uint32, uint32> _arenaBotOwnerLockMap;
static bool _arenaBotOwnerLockMapLoaded = false;
static std::unordered_map<uint32, std::string> _botChatPersonaByEntry;
static bool _botChatPersonaLoaded = false;
static std::unordered_map<uint32, uint32> _botChatCooldownUntilMs;
static std::deque<uint32> _botChatGlobalSayMs;
static std::unordered_map<uint32, std::deque<std::string>> _botChatRecentSceneEvents;
static std::unordered_map<uint32, std::string> _botChatRecentPvpKiller;
static std::unordered_map<uint32, std::string> _botChatRecentPvpVictim;
static std::unordered_map<uint32, uint8> _botChatPvpWinStreak;
static std::unordered_map<uint32, uint8> _botChatPvpLoseStreak;
static std::unordered_set<uint32> _botChatUrgentReplyEntries;
static std::unordered_set<uint32> _botChatDirectMentionEntries;
static std::unordered_map<uint32, size_t> _botChatDirectMentionFirstPosByEntry;
static std::unordered_map<uint32, std::string> _botChatUrgentChannelByEntry;
static std::unordered_map<uint32, uint32> _botChatUrgentMapByEntry;
static uint32 _botChatTickerMs = 0;
static std::unordered_map<uint32, uint32> _botAmbientChatTimerMs;
static std::unordered_map<uint32, std::deque<std::string>> _botChatRecentLinesByEntry;
static constexpr size_t BOT_CHAT_RECENT_LINES_MAX = 6;
// By leewheel 20260529 - per (bot, player) wanderer greet cooldown.
static std::unordered_map<uint64, uint32> _wandererGreetCooldownUntilMs;

struct MentionedBot
{
    Creature const* bot = nullptr;
    size_t firstPos = std::string::npos;
};

static std::vector<MentionedBot> FindMentionedBotsByMessage(std::string_view message)
{
    std::vector<MentionedBot> mentioned;
    mentioned.reserve(4);

    std::wstring wmsg;
    if (!Utf8toWStr(message, wmsg))
        return mentioned;
    wstrToLower(wmsg);

    for (Creature const* bot : _existingBots)
    {
        if (!bot || !bot->IsInWorld() || !bot->IsAlive())
            continue;

        std::wstring wname;
        if (!Utf8toWStr(bot->GetName(), wname))
            continue;
        wstrToLower(wname);
        if (wname.size() < 2)
            continue;

        size_t const mentionPos = wmsg.find(wname);
        if (mentionPos == std::wstring::npos)
            continue;

        mentioned.push_back({ bot, mentionPos });
        if (mentioned.size() >= 4)
            break;
    }

    std::ranges::sort(mentioned, [](MentionedBot const& a, MentionedBot const& b)
    {
        if (a.firstPos != b.firstPos)
            return a.firstPos < b.firstPos;
        return a.bot->GetEntry() < b.bot->GetEntry();
    });

    return mentioned;
}

static void ReloadArenaBotOwnerLocks()
{
    // By leewheel 20260528 - arena fixed-roster bot ownership lock map (bot_entry -> owner_guid).
    _arenaBotOwnerLockMap.clear();
    QueryResult res = CharacterDatabase.Query("SELECT bot_entry, owner_guid FROM characters_npcbot_arena_roster WHERE bot_entry > 0");
    if (!res)
    {
        _arenaBotOwnerLockMapLoaded = true;
        return;
    }

    do
    {
        Field* fields = res->Fetch();
        _arenaBotOwnerLockMap[fields[0].GetUInt32()] = fields[1].GetUInt32();
    } while (res->NextRow());

    _arenaBotOwnerLockMapLoaded = true;
}

static uint32 GetArenaBotLockOwner(uint32 botEntry)
{
    if (!_arenaBotOwnerLockMapLoaded)
        ReloadArenaBotOwnerLocks();

    auto itr = _arenaBotOwnerLockMap.find(botEntry);
    return itr != _arenaBotOwnerLockMap.end() ? itr->second : 0;
}

static void ReloadBotChatPersonas()
{
    _botChatPersonaByEntry.clear();
    QueryResult res = CharacterDatabase.Query("SELECT bot_entry, style_prompt FROM characters_npcbot_chat_persona WHERE enabled = 1");
    if (!res)
    {
        _botChatPersonaLoaded = true;
        return;
    }

    do
    {
        Field* fields = res->Fetch();
        _botChatPersonaByEntry[fields[0].GetUInt32()] = fields[1].GetString();
    } while (res->NextRow());
    _botChatPersonaLoaded = true;
}

static bool ContainsBlockedPoliticalTerms(std::string_view text)
{
    static std::array<std::string_view, 7> const blocked = { "政治", "政党", "政府", "国家主席", "选举", "外交", "意识形态" };
    return std::ranges::any_of(blocked, [text](std::string_view term) { return text.find(term) != std::string_view::npos; });
}

static bool LooksMostlyChinese(std::string_view text)
{
    if (text.empty())
        return false;
    uint32 ascii = 0;
    for (unsigned char c : text)
        if (c < 0x80)
            ++ascii;
    return ascii < (text.size() * 3 / 4);
}

static std::string LimitChineseReplyLength(std::string_view text, size_t maxHanChars)
{
    // By leewheel 20260528 - hard cap bot chat length by CJK codepoints, keep punctuation if attached.
    std::string out;
    out.reserve(text.size());

    size_t hanCount = 0;
    for (size_t i = 0; i < text.size();)
    {
        unsigned char c = static_cast<unsigned char>(text[i]);
        size_t n = 1;
        if ((c & 0x80) == 0x00)
            n = 1;
        else if ((c & 0xE0) == 0xC0)
            n = 2;
        else if ((c & 0xF0) == 0xE0)
            n = 3;
        else if ((c & 0xF8) == 0xF0)
            n = 4;
        if (i + n > text.size())
            break;

        bool isLikelyHan = (n == 3); // Chinese in UTF-8 is mostly 3-byte, enough for this lightweight cap.
        if (isLikelyHan && hanCount >= maxHanChars)
            break;

        out.append(text.substr(i, n));
        if (isLikelyHan)
            ++hanCount;
        i += n;
    }

    return out;
}

static void PushSceneEventByEntry(uint32 botEntry, std::string_view eventText)
{
    if (!botEntry || eventText.empty())
        return;

    // By leewheel 20260528 - keep small rolling scene-event window per bot for chat grounding.
    auto& q = _botChatRecentSceneEvents[botEntry];
    q.emplace_back(eventText);
    while (q.size() > 6)
        q.pop_front();
}

static void RememberPvpKillerByEntry(uint32 botEntry, std::string_view killerName)
{
    if (!botEntry || killerName.empty())
        return;
    _botChatRecentPvpKiller[botEntry] = std::string(killerName);
}

static void RememberPvpVictimByEntry(uint32 botEntry, std::string_view victimName)
{
    if (!botEntry || victimName.empty())
        return;
    _botChatRecentPvpVictim[botEntry] = std::string(victimName);
}

static std::string PopPvpKillerByEntry(uint32 botEntry)
{
    auto itr = _botChatRecentPvpKiller.find(botEntry);
    if (itr == _botChatRecentPvpKiller.end())
        return {};
    std::string out = std::move(itr->second);
    _botChatRecentPvpKiller.erase(itr);
    return out;
}

static std::string PopPvpVictimByEntry(uint32 botEntry)
{
    auto itr = _botChatRecentPvpVictim.find(botEntry);
    if (itr == _botChatRecentPvpVictim.end())
        return {};
    std::string out = std::move(itr->second);
    _botChatRecentPvpVictim.erase(itr);
    return out;
}

static std::string PopLatestSceneEvent(uint32 botEntry)
{
    auto itr = _botChatRecentSceneEvents.find(botEntry);
    if (itr == _botChatRecentSceneEvents.end() || itr->second.empty())
        return {};

    std::string ev = std::move(itr->second.back());
    itr->second.pop_back();
    if (itr->second.empty())
        _botChatRecentSceneEvents.erase(itr);
    return ev;
}

static bool HasPendingSceneEvent(uint32 botEntry)
{
    auto itr = _botChatRecentSceneEvents.find(botEntry);
    return itr != _botChatRecentSceneEvents.end() && !itr->second.empty();
}

static Player* ResolveConnectedBotOwner(Creature const* bot)
{
    if (!bot || !bot->GetBotAI() || bot->GetBotAI()->IAmFree())
        return nullptr;

    ObjectGuid::LowType const ownerLow = bot->GetBotAI()->GetBotOwnerGuid();
    if (!ownerLow)
        return nullptr;

    Player* owner = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(ownerLow));
    if (!owner || !owner->IsInWorld())
        return nullptr;

    return owner;
}

static Group const* LookupPlayerGroupSafe(Player* player)
{
    if (!player || !player->GetGroupRef().isValid())
        return nullptr;

    Group* gr = player->GetGroup();
    if (!gr)
        return nullptr;

    if (sGroupMgr->GetGroupByGUID(gr->GetGUID()) != gr)
        return nullptr;

    return gr;
}

static Group const* ResolveBotChatGroup(Creature const* bot)
{
    if (!bot)
        return nullptr;

    Player* owner = ResolveConnectedBotOwner(bot);
    if (!owner)
        return nullptr;

    Group const* gr = LookupPlayerGroupSafe(owner);
    if (!gr)
        return nullptr;

    Group* mutGr = const_cast<Group*>(gr);
    ObjectGuid const botGuid = bot->GetGUID();
    if (!mutGr->IsMember(botGuid) && !mutGr->IsMember(owner->GetGUID()))
        return nullptr;

    return gr;
}

static Player const* ResolveBotChatOwner(Creature const* bot)
{
    return ResolveConnectedBotOwner(bot);
}

static bool BotHasHiredSquad(Creature const* bot, Player const** outOwner = nullptr)
{
    Player const* owner = ResolveBotChatOwner(bot);
    if (!owner)
        return false;
    if (outOwner)
        *outOwner = owner;
    return true;
}

static bool LooksLikeRepetitiveBotLine(std::string_view text)
{
    if (text.empty())
        return true;
    static std::string_view const bannedSubstrings[] =
    {
        "节奏太稳", "稳住节奏", "保持节奏", "咱这节奏", "咱们节奏", "节奏太快了", "节奏太快",
        "歇口气", "探探路", "别掉队", "回头我去", "聚点前"
    };
    for (std::string_view key : bannedSubstrings)
    {
        if (text.find(key) != std::string_view::npos)
            return true;
    }
    return false;
}

static bool IsRepeatedBotChatLine(uint32 entry, std::string const& msg)
{
    auto itr = _botChatRecentLinesByEntry.find(entry);
    if (itr == _botChatRecentLinesByEntry.end())
        return false;
    for (std::string const& prev : itr->second)
    {
        if (prev == msg)
            return true;
        if (prev.size() >= 8 && msg.size() >= 8 && prev.substr(0, 8) == msg.substr(0, 8))
            return true;
    }
    return false;
}

static void RememberBotChatLine(uint32 entry, std::string const& msg)
{
    auto& recent = _botChatRecentLinesByEntry[entry];
    recent.push_back(msg);
    while (recent.size() > BOT_CHAT_RECENT_LINES_MAX)
        recent.pop_front();
}

static std::string PickNonRepeatingPartyLine(Creature const* bot, std::string_view channelHint)
{
    static std::string_view const travelLines[] =
    {
        "前面路口左拐，跟紧。",
        "补给还够吗？不够说一声。",
        "这波怪我来开，你们准备。",
        "马上到任务点了。",
        "先清这波，再往前推。",
        "注意背后，可能有巡逻。",
        "我去看下前面有没有怪。",
        "歇会儿也行，我不急。"
    };
    static std::string_view const banterLines[] =
    {
        "行，听你的。",
        "收到，跟上。",
        "可以，走。",
        "没问题。",
        "好嘞。",
        "嗯，继续。"
    };
    std::string_view const* pool = (channelHint == "小队频道" || channelHint == "团队频道") ? travelLines : banterLines;
    size_t const poolSize = (channelHint == "小队频道" || channelHint == "团队频道") ? std::size(travelLines) : std::size(banterLines);
    for (uint8 attempt = 0; attempt < 8; ++attempt)
    {
        std::string candidate = std::string(pool[(bot->GetEntry() + attempt + urand(0, uint32(poolSize - 1))) % poolSize]);
        if (!IsRepeatedBotChatLine(bot->GetEntry(), candidate) && !LooksLikeRepetitiveBotLine(candidate))
            return candidate;
    }
    return std::string(banterLines[bot->GetEntry() % std::size(banterLines)]);
}

static bool BotChatMapHasPlayers(Creature const* bot)
{
    Map const* map = bot ? bot->GetMap() : nullptr;
    return map && map->HavePlayers();
}

static bool IsInstanceMapForChat(Map const* map)
{
    return map && map->IsDungeon();
}

static int32 GetBotChatCandidatePriority(Creature const* bot, uint32 nowMs)
{
    if (!bot)
        return 0;

    uint32 const entry = bot->GetEntry();
    if (_botChatDirectMentionEntries.contains(entry))
        return 1000;
    if (_botChatUrgentReplyEntries.contains(entry))
        return 900;
    if (HasPendingSceneEvent(entry))
        return 850;

    if (_botChatCooldownUntilMs[entry] > nowMs)
        return 0;

    if (ResolveBotChatGroup(bot))
        return 750;
    if (BotHasHiredSquad(bot))
        return 720;
    if (bot->IsWandererBot() && BotChatMapHasPlayers(bot))
        return 500;
    return 0;
}

static void SeedProactivePartySceneEvent(Creature const* bot)
{
    if (!bot || HasPendingSceneEvent(bot->GetEntry()))
        return;

    Map const* map = bot->GetMap();
    Player const* owner = ResolveBotChatOwner(bot);

    if (bot->IsInCombat())
    {
        if (Unit const* victim = bot->GetVictim())
        {
            bool const boss = victim->IsCreature() && victim->ToCreature()->IsDungeonBoss();
            PushSceneEventByEntry(bot->GetEntry(), Bcore::StringFormat(
                "event={}; target={}; map={}",
                boss ? "BOSS_FIGHT" : "COMBAT_ACTIVE",
                victim->GetName(), bot->GetMapId()));
        }
        else
        {
            PushSceneEventByEntry(bot->GetEntry(), Bcore::StringFormat(
                "event=COMBAT_ACTIVE; map={}", bot->GetMapId()));
        }
    }
    else if (owner && owner->IsInCombat())
    {
        PushSceneEventByEntry(bot->GetEntry(), Bcore::StringFormat(
            "event=COMBAT_ACTIVE; map={}; owner={}", bot->GetMapId(), owner->GetName()));
    }
    else if (map && map->IsRaid())
    {
        PushSceneEventByEntry(bot->GetEntry(), Bcore::StringFormat(
            "event=RAID_PROGRESS; map={}", bot->GetMapId()));
    }
    else if (map && IsInstanceMapForChat(map))
    {
        PushSceneEventByEntry(bot->GetEntry(), Bcore::StringFormat(
            "event=DUNGEON_PROGRESS; map={}; instance=1", bot->GetMapId()));
    }
    else if (owner && owner->isMoving())
    {
        PushSceneEventByEntry(bot->GetEntry(), Bcore::StringFormat(
            "event=TRAVEL; map={}; owner={}", bot->GetMapId(), owner->GetName()));
    }
    else if (owner)
    {
        bool onQuest = false;
        for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
        {
            if (owner->GetQuestSlotQuestId(slot))
            {
                onQuest = true;
                break;
            }
        }
        if (onQuest)
        {
            PushSceneEventByEntry(bot->GetEntry(), Bcore::StringFormat(
                "event=QUESTING; map={}; owner={}", bot->GetMapId(), owner->GetName()));
        }
        else
        {
            PushSceneEventByEntry(bot->GetEntry(), Bcore::StringFormat(
                "event=PARTY_BANTER; context=open_world; map={}", bot->GetMapId()));
        }
    }
    else
    {
        PushSceneEventByEntry(bot->GetEntry(), "event=PARTY_BANTER; context=idle");
    }
}

static std::string BuildBotChatTemplate(Creature const* bot, std::string_view channelHint);

static std::string ExtractSceneEventField(std::string_view event, std::string_view key)
{
    std::string pattern = std::string(key) + "=";
    size_t pos = event.find(pattern);
    if (pos == std::string_view::npos)
        return {};
    pos += pattern.size();
    size_t end = event.find(';', pos);
    if (end == std::string_view::npos)
        return std::string(event.substr(pos));
    return std::string(event.substr(pos, end - pos));
}

static bool IsPlayerDirectedChatEvent(std::string_view event)
{
    return event.starts_with("event=PLAYER_CHAT") || event.starts_with("event=PLAYER_MENTION") ||
        event.starts_with("event=PLAYER_WHISPER");
}

static bool LooksLikeSocialGreeting(std::string_view text)
{
    if (text.empty())
        return false;
    static std::string_view const keys[] =
    {
        "晚上好", "早上好", "下午好", "中午好", "你好", "大家好", "嗨", "哈喽", "hello", "hi",
        "在吗", "睡了", "早安", "晚安", "好呀", "好啊"
    };
    for (std::string_view key : keys)
    {
        if (text.find(key) != std::string_view::npos)
            return true;
    }
    return false;
}

static bool LooksLikeInternalPersonaPrompt(std::string_view text)
{
    return text.find("身份=魔兽老玩家") != std::string_view::npos ||
        text.find("规则=仅中文") != std::string_view::npos ||
        text.find("人格=") != std::string_view::npos;
}

static std::string PickPartySocialFallback(Creature const* bot, std::string_view playerText)
{
    if (LooksLikeSocialGreeting(playerText))
    {
        static std::string_view const greetLines[] =
        {
            "晚上好呀！",
            "嗨，在呢。",
            "嗯嗯，咋啦？",
            "收到，队长~",
            "好嘞，一起加油。"
        };
        return std::string(greetLines[bot->GetEntry() % std::size(greetLines)]);
    }
    static std::string_view const casualLines[] =
    {
        "哈哈行。",
        "懂了懂了。",
        "可以可以。",
        "没问题，听你的。",
        "行，我跟上。"
    };
    return std::string(casualLines[bot->GetEntry() % std::size(casualLines)]);
}

static bool IsBotChatGroupChannel(std::string_view channelName)
{
    return channelName == "小队频道" || channelName == "团队频道";
}

// By leewheel 20260529 - wanderer meets player on the road: dedicated greet prompt + /say.
static void TryWandererGreetPlayer(Creature* bot, Player* player)
{
    if (!bot || !player || player->IsGameMaster())
        return;

    // By leewheel 20260609 - skip greeting for hostile faction players
    if (bot->IsValidAttackTarget(player))
        return;

    uint64 const key = (uint64(bot->GetEntry()) << 32) | player->GetGUID().GetCounter();
    uint32 const nowMs = GameTime::GetGameTimeMS();
    auto itr = _wandererGreetCooldownUntilMs.find(key);
    if (itr != _wandererGreetCooldownUntilMs.end() && itr->second > nowMs)
        return;

    _wandererGreetCooldownUntilMs[key] = nowMs + BotCfg::GetWandererGreetCooldownMs();

    PushSceneEventByEntry(bot->GetEntry(), Bcore::StringFormat(
        "event=PLAYER_NEARBY_MEET; player={}; map={}; level={}",
        player->GetName(), bot->GetMapId(), uint32(player->GetLevel())));

    std::string msg = BuildBotChatTemplate(bot, "打招呼");
    if (!msg.empty())
        bot->Say(msg, LANG_UNIVERSAL, player);
}

static std::string BuildBotChatTemplate(Creature const* bot, std::string_view channelHint)
{
    auto raceNameZh = [](uint8 race) -> std::string_view
    {
        switch (race)
        {
            case RACE_HUMAN:            return "人类";
            case RACE_ORC:              return "兽人";
            case RACE_DWARF:            return "矮人";
            case RACE_NIGHTELF:         return "暗夜精灵";
            case RACE_UNDEAD_PLAYER:    return "亡灵";
            case RACE_TAUREN:           return "牛头人";
            case RACE_GNOME:            return "侏儒";
            case RACE_TROLL:            return "巨魔";
            case RACE_BLOODELF:         return "血精灵";
            case RACE_DRAENEI:          return "德莱尼";
            default:                    return "未知种族";
        }
    };

    std::string action = "休息中";
    if (bot->IsInCombat())
        action = "正在战斗";
    else if (bot->isMoving())
        action = "正在移动";
    std::string recentEvent = PopLatestSceneEvent(bot->GetEntry());
    std::string pvpKiller = PopPvpKillerByEntry(bot->GetEntry());
    std::string pvpVictim = PopPvpVictimByEntry(bot->GetEntry());
    uint8 const winStreak = _botChatPvpWinStreak[bot->GetEntry()];
    uint8 const loseStreak = _botChatPvpLoseStreak[bot->GetEntry()];

    std::string tacticalHint = "无";
    if (bot->IsInCombat() && channelHint == "小队频道")
        tacticalHint = "优先集火当前目标";
    else if (bot->IsInCombat() && channelHint == "团队频道")
        tacticalHint = "分散站位，注意Boss技能";
    else if (bot->GetMap()->IsBattleground())
        tacticalHint = "战场优先抱团推进点位，残血先撤并报点";

    std::string worldCall = "无";
    if (channelHint == "世界频道" && !pvpKiller.empty())
    {
        Position const& p = bot->GetPosition();
        worldCall = Bcore::StringFormat("我在地图{}坐标({:.1f},{:.1f})被{}打了，来人支援报仇",
            bot->GetMapId(), p.GetPositionX(), p.GetPositionY(), pvpKiller);
    }
    std::string channelStyle = "正常语气";
    if (channelHint == "打招呼")
    {
        channelStyle = "路边偶遇真人玩家，热情简短打招呼，可聊赶路或地图，禁止编造刚给过buff/技能/护盾";
    }
    else if (channelHint == "世界频道贸易")
    {
        channelStyle = "在世界频道吆喝出售装绑装备，必须含物品名与起拍价，并提示密语你谈价，一句话，像真人跳蚤市场";
    }
    else if (channelHint == "世界频道")
    {
        if (recentEvent.find("event=PVP_ASSAULT") != std::string::npos)
            channelStyle = "被敌对阵营玩家偷袭正在战斗中，立即在世界频道喊救命和坐标，语气紧急";
        else
            channelStyle = "可轻度挑衅和阵营喊话，但不要低俗刷屏";
    }
    else if (channelHint == "小队频道" || channelHint == "团队频道")
    {
        if (bot->IsInCombat())
            channelStyle = "战斗中简短报点，一句话";
        else if (recentEvent.find("event=BOSS_PULL") != std::string::npos || recentEvent.find("event=BOSS_FIGHT") != std::string::npos)
            channelStyle = "Boss战小队频道：可喊开战/注意技能/分散或打断，基于已知事件，禁止编造未发生的机制";
        else if (recentEvent.find("event=COMBAT_PULL") != std::string::npos || recentEvent.find("event=COMBAT_ACTIVE") != std::string::npos)
            channelStyle = "副本战斗中：简短报拉怪、集火或注意走位，口语自然";
        else if (recentEvent.find("event=DUNGEON_PROGRESS") != std::string::npos)
            channelStyle = "副本推进中：可聊路线、补给或下一波，像正常队伍";
        else if (recentEvent.find("event=RAID_PROGRESS") != std::string::npos)
            channelStyle = "团本推进：可聊Boss进度、分配或站位，简短自然";
        else if (recentEvent.find("event=TRAVEL") != std::string::npos)
            channelStyle = "赶路中闲聊：聊路线、坐骑或下一目标，语气轻松";
        else if (recentEvent.find("event=QUESTING") != std::string::npos)
            channelStyle = "做任务途中：可聊任务进度或附近怪点，像队友搭话";
        else if (recentEvent.find("event=BOSS_CAST") != std::string::npos)
            channelStyle = "Boss读条/施法预警：喊注意技能、躲圈或打断，基于事件不要编造";
        else if (recentEvent.find("event=PARTY_BANTER") != std::string::npos)
            channelStyle = "队友闲聊搭话，语气轻松自然，不要复读战术套话";
        else if (IsPlayerDirectedChatEvent(recentEvent))
        {
            std::string const playerText = ExtractSceneEventField(recentEvent, "text");
            std::string const speaker = ExtractSceneEventField(recentEvent, "speaker");
            if (LooksLikeSocialGreeting(playerText))
            {
                channelStyle = Bcore::StringFormat(
                    "队友{}在问候：「{}」，像朋友一样自然回招呼，可幽默，禁止复读「保持节奏」「稳住节奏」等打本套话",
                    speaker.empty() ? "有人" : speaker, playerText.empty() ? "你好" : playerText);
            }
            else
            {
                channelStyle = Bcore::StringFormat(
                    "队友{}说：「{}」，直接接这句话闲聊或回应，口语自然，禁止复读战术套话",
                    speaker.empty() ? "有人" : speaker, playerText.empty() ? "..." : playerText);
            }
        }
        else
            channelStyle = "队友闲聊时可搭话，语气自然，不要复读战术套话";
    }
    else if (channelHint == "密语")
    {
        if (IsPlayerDirectedChatEvent(recentEvent))
        {
            std::string const playerText = ExtractSceneEventField(recentEvent, "text");
            std::string const speaker = ExtractSceneEventField(recentEvent, "speaker");
            channelStyle = Bcore::StringFormat(
                "玩家{}密语：「{}」，像真人一样简短回应，不要复读打本节奏套话",
                speaker.empty() ? "有人" : speaker, playerText.empty() ? "..." : playerText);
        }
        else
            channelStyle = "密语闲聊，简短自然";
    }

    std::string persona;
    if (!_botChatPersonaLoaded)
        ReloadBotChatPersonas();
    if (auto itr = _botChatPersonaByEntry.find(bot->GetEntry()); itr != _botChatPersonaByEntry.end())
        persona = itr->second;

    std::string objectiveHint = tacticalHint;
    if (channelHint == "世界频道" && !pvpKiller.empty())
        objectiveHint = worldCall;

    if (persona.empty())
    {
        NpcBotData const* data = BotDataMgr::SelectNpcBotData(bot->GetEntry());
        if (NpcBotExtras const* ex = BotDataMgr::SelectNpcBotExtras(bot->GetEntry()))
        {
            std::string className = bot_ai::LocalizedNpcText(nullptr, BotDataMgr::TextForClass(ex->bclass));
            std::string specName = bot_ai::LocalizedNpcText(nullptr, BotDataMgr::TextForSpec(data ? data->spec : BOT_SPEC_DEFAULT));
            // By leewheel 20260528 - compact prompt for small model: structured, short, single-output target.
            persona = Bcore::StringFormat(
                "身份=魔兽老玩家;种族={};职业={};专精={};状态={};频道={};"
                "事件={};PVP赢={};PVP输={};被谁打={};我打了谁={};战术={};"
                "语气={};规则=仅中文口语,真实玩家口吻,不可承认AI/机器人,必须完整一句话并收束,尽量50-80字,绝不超100字,禁止编造未发生的技能或buff,只输出一句自然话。",
                raceNameZh(ex->race), className, specName, action, channelHint,
                recentEvent.empty() ? "无" : recentEvent,
                uint32(winStreak), uint32(loseStreak),
                pvpKiller.empty() ? "无" : pvpKiller,
                pvpVictim.empty() ? "无" : pvpVictim,
                objectiveHint, channelStyle);
        }
        else
        {
            persona = Bcore::StringFormat(
                "身份=魔兽老玩家;状态={};频道={};事件={};PVP赢={};PVP输={};被谁打={};我打了谁={};战术={};"
                "语气={};规则=仅中文口语,真实玩家口吻,不可承认AI/机器人,必须完整一句话并收束,尽量50-80字,绝不超100字,禁止编造未发生的技能或buff,只输出一句自然话。",
                action, channelHint,
                recentEvent.empty() ? "无" : recentEvent,
                uint32(winStreak), uint32(loseStreak),
                pvpKiller.empty() ? "无" : pvpKiller,
                pvpVictim.empty() ? "无" : pvpVictim,
                objectiveHint, channelStyle);
        }
    }
    else
    {
        persona = Bcore::StringFormat(
            "人格={};状态={};频道={};事件={};PVP赢={};PVP输={};被谁打={};我打了谁={};战术={};语气={};"
            "规则=仅中文口语,真实玩家口吻,不可承认AI/机器人,必须完整一句话并收束,尽量50-80字,绝不超100字,禁止编造未发生的技能或buff,只输出一句自然话。",
            persona, action, channelHint,
            recentEvent.empty() ? "无" : recentEvent,
            uint32(winStreak), uint32(loseStreak),
            pvpKiller.empty() ? "无" : pvpKiller,
            pvpVictim.empty() ? "无" : pvpVictim,
            objectiveHint, channelStyle);
    }

    std::string llmReply;
    if (NpcBotChatLLM::Engine::Instance().IsEnabled())
        llmReply = NpcBotChatLLM::Engine::Instance().GenerateReply(bot, persona);
    if (!llmReply.empty())
        persona = std::move(llmReply);
    else if (channelHint == "打招呼")
    {
        static std::string_view const greetLines[] =
        {
            "嘿，路过打个招呼，祝你好运！",
            "嗨，冒险者，路上小心。",
            "哟，忙着呢？需要帮忙喊一声。",
            "刚路过，见你在附近，打个招呼。"
        };
        persona = std::string(greetLines[urand(0, std::size(greetLines) - 1)]);
    }
    else if (channelHint == "世界频道贸易")
    {
        std::string tradeLine = WandererVendor::BuildTradeShoutFallback(bot);
        if (!tradeLine.empty())
            persona = std::move(tradeLine);
    }
    else if (LooksLikeInternalPersonaPrompt(persona))
    {
        if (channelHint == "小队频道" || channelHint == "团队频道" || channelHint == "密语")
            persona = PickPartySocialFallback(bot, ExtractSceneEventField(recentEvent, "text"));
        else if (channelHint == "世界频道")
        {
            if (!pvpKiller.empty())
                persona = worldCall;
            else
                persona = "有人在吗，一起刷刷？";
        }
        else
            persona = "收到。";
    }

    if (LooksLikeRepetitiveBotLine(persona) || IsRepeatedBotChatLine(bot->GetEntry(), persona))
        persona = PickNonRepeatingPartyLine(bot, channelHint);

    // By leewheel 20260528 - lightweight safe gate (Chinese + no politics).
    if (!LooksMostlyChinese(persona) || ContainsBlockedPoliticalTerms(persona))
        return "今天天气不错，我们继续冒险吧。";
    return LimitChineseReplyLength(persona, 100);
}

static void SendBotMessageToGroup(Creature const* bot, Group const* group, bool isRaid, std::string const& text)
{
    if (!group || !bot)
        return;
    // By leewheel 20260528 - enforce hard cap for proactive party/raid messages at send layer.
    std::string const cappedText = LimitChineseReplyLength(text, 100);
    for (GroupReference const* itr = group->GetFirstMember(); itr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (!member || !member->GetSession())
            continue;
        LocaleConstant const locale = member->GetSession()->GetSessionDbLocaleIndex();
        std::string const displayName = BotDataMgr::GetNpcBotDisplayName(bot->GetEntry(), locale);

        WorldPackets::Chat::Chat packet;
        // CHAT_MSG_PARTY/RAID omit creature SenderName on 3.3.5 clients; MONSTER_PARTY includes it.
        if (isRaid)
        {
            std::string const raidText = Bcore::StringFormat("{}：{}", displayName, cappedText);
            packet.Initialize(CHAT_MSG_RAID, LANG_UNIVERSAL, bot, member, raidText, 0, "", locale);
        }
        else
        {
            packet.Initialize(CHAT_MSG_MONSTER_PARTY, LANG_UNIVERSAL, bot, member, cappedText, 0, "", locale);
            if (!displayName.empty())
                packet.SenderName = displayName;
        }
        member->SendDirectMessage(packet.Write());
    }
}

static void SendBotMessageToOwnerPlayer(Creature const* bot, Player const* owner, bool isRaid, std::string const& text)
{
    if (!bot || !owner)
        return;

    Player* recipient = ObjectAccessor::FindConnectedPlayer(owner->GetGUID());
    if (!recipient || !recipient->GetSession())
        return;

    std::string const cappedText = LimitChineseReplyLength(text, 100);
    LocaleConstant const locale = recipient->GetSession()->GetSessionDbLocaleIndex();
    std::string const displayName = BotDataMgr::GetNpcBotDisplayName(bot->GetEntry(), locale);

    WorldPackets::Chat::Chat packet;
    if (isRaid)
    {
        std::string const raidText = Bcore::StringFormat("{}：{}", displayName, cappedText);
        packet.Initialize(CHAT_MSG_RAID, LANG_UNIVERSAL, bot, recipient, raidText, 0, "", locale);
    }
    else
    {
        packet.Initialize(CHAT_MSG_MONSTER_PARTY, LANG_UNIVERSAL, bot, recipient, cappedText, 0, "", locale);
        if (!displayName.empty())
            packet.SenderName = displayName;
    }
    recipient->SendDirectMessage(packet.Write());
}

static void TryBotChannelChat(uint32 diff)
{
    if (!BotCfg::IsBotChatEnabled())
        return;

    _botChatTickerMs += diff;
    if (_botChatTickerMs < 500)
        return;
    _botChatTickerMs = 0;

    uint32 const nowMs = GameTime::GetGameTimeMS();
    while (!_botChatGlobalSayMs.empty() && (nowMs - _botChatGlobalSayMs.front()) > 60000)
        _botChatGlobalSayMs.pop_front();
    if (_botChatGlobalSayMs.size() >= BotCfg::GetBotChatGlobalMaxPerMinute())
        return;

    int32 bestPriority = 0;
    std::vector<Creature const*> candidates;
    candidates.reserve(48);
    for (Creature const* bot : _existingBots)
    {
        if (!bot || !bot->IsInWorld() || !bot->GetBotAI() || !bot->IsAlive())
            continue;

        int32 const priority = GetBotChatCandidatePriority(bot, nowMs);
        if (priority <= 0)
            continue;

        if (priority > bestPriority)
        {
            bestPriority = priority;
            candidates.clear();
        }
        if (priority == bestPriority)
            candidates.push_back(bot);
    }

    if (candidates.empty())
        return;

    Creature const* bot = candidates[urand(0, uint32(candidates.size() - 1))];
    bool const directMention = _botChatDirectMentionEntries.contains(bot->GetEntry());
    bool const urgentReply = _botChatUrgentReplyEntries.contains(bot->GetEntry());
    std::string urgentChannel = urgentReply && _botChatUrgentChannelByEntry.contains(bot->GetEntry())
        ? _botChatUrgentChannelByEntry[bot->GetEntry()] : std::string();
    uint32 const urgentMapId = urgentReply && _botChatUrgentMapByEntry.contains(bot->GetEntry())
        ? _botChatUrgentMapByEntry[bot->GetEntry()] : bot->GetMapId();
    bool const urgentPartyChannel = urgentChannel == "小队频道";
    bool const urgentRaidChannel = urgentChannel == "团队频道";

    Group const* chatGroup = ResolveBotChatGroup(bot);
    Player const* squadOwner = nullptr;
    bool const hasSquad = BotHasHiredSquad(bot, &squadOwner);
    std::string_view channelHint = "世界频道";
    if (urgentPartyChannel)
        channelHint = "小队频道";
    else if (urgentRaidChannel)
        channelHint = "团队频道";
    else if (urgentReply && !urgentChannel.empty())
        channelHint = "频道回复";
    else if (chatGroup)
        channelHint = chatGroup->isRaidGroup() ? "团队频道" : "小队频道";
    else if (hasSquad && squadOwner)
    {
        Group const* squadGroup = LookupPlayerGroupSafe(const_cast<Player*>(squadOwner));
        channelHint = (squadGroup && squadGroup->isRaidGroup()) ? "团队频道" : "小队频道";
    }
    else if (bot->IsWandererBot() && BotChatMapHasPlayers(bot))
        channelHint = "世界频道";

    if ((channelHint == "小队频道" || channelHint == "团队频道") && !urgentReply && !HasPendingSceneEvent(bot->GetEntry()))
        SeedProactivePartySceneEvent(bot);

    if (bot->IsWandererBot() && !urgentReply && channelHint == "世界频道" && WandererVendor::WantsTradeWorldShout(bot))
    {
        std::string tradeEv = WandererVendor::BuildTradeSceneEvent(bot);
        if (!tradeEv.empty())
            PushSceneEventByEntry(bot->GetEntry(), tradeEv);
        channelHint = "世界频道贸易";
    }

    std::string msg = LimitChineseReplyLength(BuildBotChatTemplate(bot, channelHint), 100);
    if (msg.empty() || IsRepeatedBotChatLine(bot->GetEntry(), msg) || LooksLikeRepetitiveBotLine(msg))
    {
        msg = PickNonRepeatingPartyLine(bot, channelHint);
        if (msg.empty())
            return;
    }

    bool sent = false;
    bool const partyChannel = (channelHint == "小队频道" || channelHint == "团队频道");
    bool const isRaidChannel = channelHint == "团队频道";

    if (partyChannel && chatGroup)
    {
        if (chatGroup->isRaidGroup() && BotCfg::IsBotChatRaidEnabled())
        {
            SendBotMessageToGroup(bot, chatGroup, true, msg);
            sent = true;
        }
        else if (BotCfg::IsBotChatPartyEnabled())
        {
            SendBotMessageToGroup(bot, chatGroup, false, msg);
            sent = true;
        }
    }
    else if (partyChannel && hasSquad && squadOwner)
    {
        if (isRaidChannel && BotCfg::IsBotChatRaidEnabled())
        {
            SendBotMessageToOwnerPlayer(bot, squadOwner, true, msg);
            sent = true;
        }
        else if (BotCfg::IsBotChatPartyEnabled())
        {
            SendBotMessageToOwnerPlayer(bot, squadOwner, false, msg);
            sent = true;
        }
    }

    bool const allowWorldAutonomous = BotCfg::IsBotChatWorldEnabled() && BotChatMapHasPlayers(bot) &&
        (bot->IsWandererBot() || urgentReply || !partyChannel);
    if (!sent && allowWorldAutonomous && !partyChannel)
    {
        // Build colored name prefix for world channel messages
        std::string namePrefix;
        if (bot->IsNPCBot())
        {
            std::string_view classColor;
            std::string_view className;
            switch (bot->GetBotClass())
            {
                case BOT_CLASS_WARRIOR:      classColor = "|cffc79c6e"; className = "战士"; break;
                case BOT_CLASS_PALADIN:      classColor = "|cfff58cba"; className = "圣骑士"; break;
                case BOT_CLASS_HUNTER:       classColor = "|cffabd473"; className = "猎人"; break;
                case BOT_CLASS_ROGUE:        classColor = "|cfffff569"; className = "潜行者"; break;
                case BOT_CLASS_PRIEST:       classColor = "|cffffffff"; className = "牧师"; break;
                case BOT_CLASS_DEATH_KNIGHT: classColor = "|cffc41f3b"; className = "死亡骑士"; break;
                case BOT_CLASS_SHAMAN:       classColor = "|cff0070de"; className = "萨满"; break;
                case BOT_CLASS_MAGE:         classColor = "|cff69ccf0"; className = "法师"; break;
                case BOT_CLASS_WARLOCK:      classColor = "|cff9482c9"; className = "术士"; break;
                case BOT_CLASS_DRUID:        classColor = "|cffff7d0a"; className = "德鲁伊"; break;
                case BOT_CLASS_BM:           classColor = "|cffa10015"; className = "剑圣"; break;
                case BOT_CLASS_ARCHMAGE:     classColor = "|cff028a99"; className = "大法师"; break;
                case BOT_CLASS_DREADLORD:    classColor = "|cff534161"; className = "恐惧魔王"; break;
                case BOT_CLASS_SPELLBREAKER: classColor = "|cffcf3c1f"; className = "破法者"; break;
                case BOT_CLASS_DARK_RANGER:  classColor = "|cff3e255e"; className = "黑暗游侠"; break;
                case BOT_CLASS_NECROMANCER:  classColor = "|cff9900cc"; className = "亡灵法师"; break;
                case BOT_CLASS_SEA_WITCH:    classColor = "|cff40d7a9"; className = "海巫"; break;
                case BOT_CLASS_CRYPT_LORD:   classColor = "|cff19782b"; className = "地穴领主"; break;
                default: break;
            }
            if (!classColor.empty())
                namePrefix = Bcore::StringFormat("{}[{}]|r {}: ", classColor, className, bot->GetName());
        }
        if (namePrefix.empty())
            namePrefix = bot->GetName() + std::string(": ");
        std::string worldMsg = namePrefix + msg;
        std::string const outChannel = (!urgentChannel.empty() ? urgentChannel : BotCfg::GetBotChatWorldChannelName());
        Map* outMap = sMapMgr->CreateBaseMap(urgentMapId);
        if (!outMap)
            outMap = bot->GetMap();
        for (MapReference const& ref : outMap->GetPlayers())
        {
            Player* player = ref.GetSource();
            if (!player || !player->GetSession())
                continue;
            WorldPackets::Chat::Chat packet;
            packet.Initialize(CHAT_MSG_CHANNEL, LANG_UNIVERSAL, bot, nullptr, worldMsg, 0, outChannel);
            player->SendDirectMessage(packet.Write());
        }
        sent = outMap && outMap->HavePlayers();
    }

    if (sent)
    {
        RememberBotChatLine(bot->GetEntry(), msg);
        _botChatUrgentReplyEntries.erase(bot->GetEntry());
        _botChatDirectMentionEntries.erase(bot->GetEntry());
        _botChatDirectMentionFirstPosByEntry.erase(bot->GetEntry());
        _botChatUrgentChannelByEntry.erase(bot->GetEntry());
        _botChatUrgentMapByEntry.erase(bot->GetEntry());
        _botChatGlobalSayMs.push_back(nowMs);

        uint32 nextCooldown = urand(BotCfg::GetBotChatIntervalMinMs(), BotCfg::GetBotChatIntervalMaxMs());
        if (partyChannel || bot->IsInCombat())
            nextCooldown = std::min(nextCooldown, 12000u);
        if (directMention)
            nextCooldown = 4000u;

        _botChatCooldownUntilMs[bot->GetEntry()] = nowMs + std::max(nextCooldown, partyChannel ? 5000u : BotCfg::GetBotChatBotCooldownMs());
    }
}

static bool IsArenaBotLockedForOwnerImpl(uint32 botEntry, uint32 ownerGuidLow)
{
    // By leewheel 20260528 - arena locked bot can only be reused by the same owner.
    uint32 const lockOwner = GetArenaBotLockOwner(botEntry);
    return lockOwner != 0 && lockOwner != ownerGuidLow;
}
static CreatureTemplateContainer _botsExtraCreatureTemplates;
static std::unordered_map<uint32, EquipmentInfo const*> _botsExtraCreatureEquipmentTemplates;
static std::set<uint32> _botsExtraCreaturesToDespawn;
static std::list<std::pair<uint32, WanderNode const*>> _botsWanderCreaturesToSpawn;
// By leewheel 20260528 - after a player enters a world map, wanderers stay passive briefly (no rush to clear mobs).
static std::unordered_map<uint32, time_t> _wandererMapWakeGraceUntil;

static ItemPerBotClassPerBotCategoryMap _botsExtraCreatureSortedGear;

using BotGearStorageMap = std::unordered_map<ObjectGuid /*playerGuid*/, BotBankItemContainer>;
static BotGearStorageMap _botStoredGearMap;
using BotGearSetStorageMap = std::unordered_map<ObjectGuid /*playerGuid*/, BotItemSetsArray>;
static BotGearSetStorageMap _botStoredGearSetMap;

static bool allBotsLoaded = false;

static uint32 next_wandering_bot_spawn_delay = 0;

static EventProcessor botSpawnEvents;
static std::unordered_map<ObjectGuid, EventProcessor> botBGJoinEvents;

bool BotBankItemCompare::operator()(Item const* item1, Item const* item2) const
{
    ItemTemplate const* proto1 = item1->GetTemplate();
    ItemTemplate const* proto2 = item2->GetTemplate();

    if (proto1->Class == proto2->Class)
    {
        if (proto1->SubClass == proto2->SubClass)
        {
            if (proto1->InventoryType == proto2->InventoryType)
            {
                if (proto1->Quality == proto2->Quality)
                {
                    float gs1 = CalculateItemGearScoreRaw(proto1);
                    float gs2 = CalculateItemGearScoreRaw(proto2);
                    if (gs1 == gs2)
                    {
                        if (proto1->Name1 == proto2->Name1)
                            return item1->GetGUID().GetCounter() < item2->GetGUID().GetCounter();
                        return proto1->Name1 < proto2->Name1;
                    }
                    return gs1 < gs2;
                }
                return proto1->Quality > proto2->Quality;
            }
            return proto1->InventoryType < proto2->InventoryType;
        }
        return proto1->SubClass < proto2->SubClass;
    }
    return proto1->Class < proto2->Class;
}

class BotBattlegroundEnterEvent : public BasicEvent
{
    const ObjectGuid _playerGUID;
    const ObjectGuid _botGUID;
    const BattlegroundQueueTypeId _bgQueueTypeId;
    const uint64 _removeTime;

public:
    BotBattlegroundEnterEvent(ObjectGuid playerGUID, ObjectGuid botGUID, BattlegroundQueueTypeId bgQueueTypeId, uint64 removeTime)
        : _playerGUID(playerGUID), _botGUID(botGUID), _bgQueueTypeId(bgQueueTypeId), _removeTime(removeTime) {}

    void AbortMe()
    {
        BOT_LOG_ERROR("npcbots", "BotBattlegroundEnterEvent: Aborting bot {} bg {}!", _botGUID.GetEntry(), uint32(_bgQueueTypeId.BattlemasterListId));
        sBattlegroundMgr->GetBattlegroundQueue(_bgQueueTypeId).RemovePlayer(_botGUID, true);
        BotDataMgr::DespawnWandererBot(_botGUID.GetEntry());
    }

    void AbortAll()
    {
        BOT_LOG_ERROR("npcbots", "BotBattlegroundEnterEvent: Aborting ALL bots by {} bg {}!", _playerGUID.GetCounter(), uint32(_bgQueueTypeId.BattlemasterListId));
        AbortMe();
        botBGJoinEvents.at(_playerGUID).KillAllEvents(false);
    }

    bool Execute(uint64 e_time, uint32 /*p_time*/) override
    {
        if (e_time >= _removeTime)
        {
            AbortMe();
            return true;
        }
        else if (Creature const* bot = BotDataMgr::FindBot(_botGUID.GetEntry()))
        {
            // Battleground is created at this point, try to find it
            BattlegroundQueue& queue = sBattlegroundMgr->GetBattlegroundQueue(_bgQueueTypeId);
            BattlegroundQueue::QueuedPlayersMap::const_iterator qpm_citr = queue.m_QueuedPlayers.find(_botGUID);
            GroupQueueInfo const* my_gqi = qpm_citr != queue.m_QueuedPlayers.cend() ? qpm_citr->second.GroupInfo : nullptr;
            Battleground* bg = my_gqi ? sBattlegroundMgr->GetBattleground(my_gqi->IsInvitedToBGInstanceGUID, BattlegroundTypeId(_bgQueueTypeId.BattlemasterListId)) : nullptr;

            if (!bg || bg->GetPlayersCountByTeam(ALLIANCE) + bg->GetPlayersCountByTeam(HORDE) >= bg->GetMaxPlayersPerTeam() * 2)
            {
                AbortAll();
                return true;
            }

            if (!queue.IsBotInvited(_botGUID, bg->GetInstanceID()))
            {
                AbortMe();
                return true;
            }

            if (bg->GetPlayersCountByTeam(ALLIANCE) + bg->GetPlayersCountByTeam(HORDE) > 0)
            {
                Map* bgMap = ASSERT_NOTNULL(sMapMgr->FindMap(bg->GetMapId(), bg->GetInstanceID()));

                queue.RemovePlayer(bot->GetGUID(), false);

                //BG is set second time in Battleground::AddBot() but it's the same value so this is alright
                bot->GetBotAI()->SetBG(bg);

                TeamId teamId = BotDataMgr::GetTeamIdForFaction(bot->GetFaction());
                BotMgr::TeleportBot(const_cast<Creature*>(bot), bgMap, bg->GetTeamStartPosition(teamId), true, false);
            }
            else if (std::ranges::any_of(queue.m_QueuedPlayers, [=](BattlegroundQueue::QueuedPlayersMap::value_type const& qpm_pair) {
                return qpm_pair.first.IsPlayer() && qpm_pair.second.GroupInfo->IsInvitedToBGInstanceGUID == my_gqi->IsInvitedToBGInstanceGUID;
            }))
                botBGJoinEvents.at(_playerGUID).AddEventAtOffset(new BotBattlegroundEnterEvent(_playerGUID, _botGUID, _bgQueueTypeId, _removeTime), 2s);
            else
                AbortAll();
        }

        return true;
    }

    void Abort(uint64 /*e_time*/) override { AbortMe(); }
};

static void SpawnWandererBot(uint32 bot_id, WanderNode const* spawnLoc, NpcBotRegistry* registry)
{
    NpcBotData const* bot_data = BotDataMgr::SelectNpcBotData(bot_id);
    NpcBotExtras const* bot_extras = BotDataMgr::SelectNpcBotExtras(bot_id);
    Position spawnPos = spawnLoc->GetPosition();

    ASSERT(bot_data);
    ASSERT(bot_extras);

    Map* map = sMapMgr->CreateBaseMap(spawnLoc->GetMapId());
    // Do not call LoadGrid here: it pins grids without unload. AddToWorld loads the cell as needed.

    // Fix bad/old waypoint Z to avoid "spawn in air/underground -> only shadow or pop-in".
    float const groundZ = map->GetHeight(spawnPos.GetPositionX(), spawnPos.GetPositionY(), spawnPos.GetPositionZ() + 5.0f, true, 50.0f);
    if (groundZ > INVALID_HEIGHT)
    {
        if (std::fabs(groundZ - spawnPos.GetPositionZ()) > 2.0f)
            BOT_LOG_DEBUG("npcbots", "Wanderer spawn Z corrected: bot {} map {} ({:.2f},{:.2f}) z {:.2f} -> {:.2f}",
                bot_id, map->GetId(), spawnPos.GetPositionX(), spawnPos.GetPositionY(), spawnPos.GetPositionZ(), groundZ);
        spawnPos.m_positionZ = groundZ + 0.15f;
    }

    Creature* bot = new Creature();
    if (!bot->LoadBotCreatureFromDB(0, map, true, true, bot_id, &spawnPos))
    {
        delete bot;
        BOT_LOG_FATAL("server.loading", "Cannot load npcbot from DB!");
        ASSERT(false);
    }

    if (registry)
        registry->insert(bot);
}

void BotDataMgr::DespawnWandererBot(uint32 entry)
{
    Creature const* bot = FindBot(entry);
    if (bot && bot->IsWandererBot())
    {
        if (bot->GetBotAI())
            bot->GetBotAI()->canUpdate = false;
        _botsExtraCreaturesToDespawn.insert(entry);
    }
    else
        BOT_LOG_ERROR("npcbots", "DespawnWandererBot(): trying to despawn non-existing wanderer bot {} '{}'!", entry, bot ? bot->GetName() : "unknown");
}

void BotDataMgr::DisableAndDespawnWanderer(Creature* bot, char const* reason)
{
    if (!bot || !bot->IsWandererBot())
        return;

    BOT_LOG_WARN("npcbots", "Wanderer {} ({}) removed: {}", bot->GetName(), bot->GetEntry(), reason ? reason : "unknown");
    DespawnWandererBot(bot->GetEntry());
}

static void SpawnDungeonBot(uint32 bot_id, Player const* owner)
{
    CreatureTemplate const& bot_template = _botsExtraCreatureTemplates.at(bot_id);
    NpcBotData const* bot_data = BotDataMgr::SelectNpcBotData(bot_id);
    NpcBotExtras const* bot_extras = BotDataMgr::SelectNpcBotExtras(bot_id);

    ASSERT(bot_data);
    ASSERT(bot_extras);

    Map* map = owner->GetMap();

    BOT_LOG_DEBUG("npcbots", "Spawning dungeon bot for player {}: {} ({}) class {} spec {} race {} fac {}, location: mapId {} {}",
        owner->GetName(), bot_template.Name, bot_id, uint32(bot_extras->bclass), uint32(bot_data->spec), uint32(bot_extras->race), bot_data->faction,
        owner->GetMapId(), owner->GetPosition().ToString());

    TempSummon* bot = map->SummonCreature(bot_id, *owner);
    if (!bot)
    {
        BOT_LOG_FATAL("npcbots", "Cannot spawn bot {} ({}) for player {}!", bot_id, bot_template.Name, owner->GetGUID().ToString());
        ABORT();
    }

    bot->setActive(true);

    ASSERT(owner->GetBotMgr()->AddDungeonBot(bot) == BOT_ADD_SUCCESS);
}

void BotDataMgr::DespawnDungeonBot(uint32 entry)
{
    Creature const* bot = FindBot(entry);
    if (bot && bot->IsSummon())
    {
        if (bot->GetBotAI())
            bot->GetBotAI()->canUpdate = false;
        _botsExtraCreaturesToDespawn.insert(entry);
    }
    else
        BOT_LOG_ERROR("npcbots", "DespawnDungeonBot(): trying to despawn non-existing dungeon bot {} '{}'!", entry, bot ? bot->GetName() : "unknown");
}

namespace
{
uint32 ResolveExistingItemEntry(std::initializer_list<uint32> candidates)
{
    for (uint32 entry : candidates)
        if (entry && sObjectMgr->GetItemTemplate(entry))
            return entry;
    return 0;
}
}

struct WanderingBotsGenerator;
static WanderingBotsGenerator* GetWanderingBotsGenerator();

struct WanderingBotsGenerator
{
private:
    using NodeVec = std::vector<WanderNode const*>;
    static bool IsSpawnNodeTooCloseToPlayers(WanderNode const* node, float minDist)
    {
        if (!node)
            return true;

        Map* map = sMapMgr->CreateBaseMap(node->GetMapId());
        if (!map || !map->HavePlayers())
            return false;

        float const minDistSq = minDist * minDist;
        for (MapReference const& ref : map->GetPlayers())
        {
            Player* player = ref.GetSource();
            if (!player || !player->IsInWorld() || player->IsGameMaster())
                continue;

            if (player->GetExactDist2dSq(node->m_positionX, node->m_positionY) < minDistSq)
                return true;
        }

        return false;
    }

    const std::map<uint8, uint32> wbot_faction_for_ex_class = {
        {BOT_CLASS_BM, FACTION_TEMPLATE_NEUTRAL_HOSTILE/*2u*/},
        {BOT_CLASS_SPHYNX, FACTION_TEMPLATE_NEUTRAL_HOSTILE},
        {BOT_CLASS_ARCHMAGE, FACTION_TEMPLATE_NEUTRAL_HOSTILE/*1u*/},
        {BOT_CLASS_DREADLORD, FACTION_TEMPLATE_NEUTRAL_HOSTILE},
        {BOT_CLASS_SPELLBREAKER, FACTION_TEMPLATE_NEUTRAL_HOSTILE/*1610u*/},
        {BOT_CLASS_DARK_RANGER, FACTION_TEMPLATE_NEUTRAL_HOSTILE},
        {BOT_CLASS_NECROMANCER, FACTION_TEMPLATE_NEUTRAL_HOSTILE},
        {BOT_CLASS_SEA_WITCH, FACTION_TEMPLATE_NEUTRAL_HOSTILE},
        {BOT_CLASS_CRYPT_LORD, FACTION_TEMPLATE_NEUTRAL_HOSTILE}
    };

    uint32 next_bot_id;
    uint32 enabledBotsCount;
    friend WanderingBotsGenerator* GetWanderingBotsGenerator();

    WanderingBotsGenerator()
    {
        next_bot_id = BOT_ENTRY_CREATE_BEGIN - 1;
        QueryResult result = CharacterDatabase.PQuery("SELECT value FROM worldstates WHERE entry = {}", uint32(BOT_GIVER_ENTRY));
        if (!result)
        {
            BOT_LOG_WARN("server.loading", "Next bot id for autogeneration is not found! Resetting! (client cache may interfere with names)");
            for (uint32 bot_cid : BotDataMgr::GetExistingNPCBotIds())
                if (bot_cid > next_bot_id)
                    next_bot_id = bot_cid;
            CharacterDatabase.DirectPExecute("INSERT INTO worldstates (entry, value, comment) VALUES ({}, {}, '{}')",
                uint32(BOT_GIVER_ENTRY), next_bot_id, "NPCBOTS MOD - last autogenerated bot entry");
        }
        else
            next_bot_id = result->Fetch()[0].GetUInt32();

        ASSERT(next_bot_id > BOT_ENTRY_BEGIN);

        for (uint8 c = BOT_CLASS_WARRIOR; c < BOT_CLASS_END; ++c)
            if (BotCfg::IsWanderingClassEnabled(c) && !_spareBotIdsPerClassMap.contains(c))
                _spareBotIdsPerClassMap.insert({ c, {} });

        for (auto const& [id, extras] : _botsExtras)
        {
            uint8 c = extras.bclass;
            if (c != BOT_CLASS_NONE && BotCfg::IsWanderingClassEnabled(c))
            {
                ++enabledBotsCount;
                if (!_botsData.contains(id))
                {
                    ASSERT(_spareBotIdsPerClassMap.contains(c));
                    _spareBotIdsPerClassMap.at(c).insert(id);
                }
            }
        }

        for (uint8 c = BOT_CLASS_WARRIOR; c < BOT_CLASS_END; ++c)
            if (_spareBotIdsPerClassMap.contains(c) && _spareBotIdsPerClassMap.at(c).empty())
                _spareBotIdsPerClassMap.erase(c);
    }

    void GenerateDungeonBotToSpawn(std::tuple<uint32, uint8, uint8, uint32>&& entry_class_spec_roles, Player const* owner)
    {
        CreatureTemplateContainer const& all_templates = sObjectMgr->GetCreatureTemplates();

        while (all_templates.contains(++next_bot_id));

        const auto& [orig_entry, bot_class, bot_spec, bot_roles] = entry_class_spec_roles;
        CreatureTemplate const* orig_template = ASSERT_NOTNULL(sObjectMgr->GetCreatureTemplate(orig_entry));
        NpcBotExtras const* orig_extras = ASSERT_NOTNULL(BotDataMgr::SelectNpcBotExtras(orig_entry));
        CreatureTemplate& bot_template = _botsExtraCreatureTemplates[next_bot_id];

        //copy all fields
        bot_template = *orig_template;
        bot_template.Entry = next_bot_id;
        bot_template.Title = "";
        bot_template.KillCredit[0] = orig_entry;

        bot_template.minlevel = owner->GetLevel();
        bot_template.maxlevel = owner->GetLevel();

        bot_template.InitializeQueryData();

        ObjectGuid::LowType owner_lowguid = owner->GetGUID().GetCounter();
        _botsData.emplace(std::piecewise_construct, std::forward_as_tuple(next_bot_id), std::forward_as_tuple(owner_lowguid, 0ull, bot_roles, FACTION_FRIENDLY, bot_spec));
        _botsExtras.emplace(next_bot_id, NpcBotExtras{.race = orig_extras->race, .bclass = bot_class});
        if (NpcBotAppearanceData const* orig_apdata = BotDataMgr::SelectNpcBotAppearance(orig_entry))
            _botsAppearanceData.emplace(std::piecewise_construct, std::forward_as_tuple(next_bot_id),
                std::forward_as_tuple(orig_apdata->name, orig_apdata->gender, orig_apdata->skin, orig_apdata->face, orig_apdata->hair, orig_apdata->haircolor, orig_apdata->features));

        int8 beqId = 1;
        _botsExtraCreatureEquipmentTemplates[next_bot_id] = sObjectMgr->GetEquipmentInfo(orig_entry, beqId);

        SpawnDungeonBot(next_bot_id, owner);

        _spareBotIdsPerClassMap.at(bot_class).erase(orig_entry);
        if (_spareBotIdsPerClassMap.at(bot_class).empty())
            _spareBotIdsPerClassMap.erase(bot_class);
    }

    bool GenerateWanderingBotToSpawn(std::pair<uint8, uint32> const& spareBotPair, uint8 desired_bracket,
        NodeVec const& spawns_a, NodeVec const& spawns_h, NodeVec const& spawns_n,
        bool immediate, PvPDifficultyEntry const* bracketEntry, NpcBotRegistry* registry)
    {
        CreatureTemplateContainer const& all_templates = sObjectMgr->GetCreatureTemplates();

        while (all_templates.contains(++next_bot_id));

        const auto [bot_class, orig_entry] = spareBotPair;
        CreatureTemplate const* orig_template = ASSERT_NOTNULL(sObjectMgr->GetCreatureTemplate(orig_entry));
        NpcBotExtras const* orig_extras = ASSERT_NOTNULL(BotDataMgr::SelectNpcBotExtras(orig_entry));
        uint32 bot_faction = BotDataMgr::GetDefaultFactionForBotRaceClass(bot_class, orig_extras->race);

        NodeVec const* bot_spawn_nodes;
        TeamId bot_team = BotDataMgr::GetTeamIdForFaction(bot_faction);
        switch (bot_team)
        {
            case TEAM_ALLIANCE:
                bot_spawn_nodes = &spawns_a;
                break;
            case TEAM_HORDE:
                bot_spawn_nodes = &spawns_h;
                break;
            default:
                bot_spawn_nodes = &spawns_n;
                break;
        }
        NodeVec level_nodes;
        level_nodes.reserve(bot_spawn_nodes->size());
        desired_bracket = std::max<uint8>(desired_bracket, BotDataMgr::GetMinLevelForBotClass(bot_class) / 10);
        for (WanderNode const* node : *bot_spawn_nodes)
        {
            if (desired_bracket * 10 + 9 >= node->GetLevels().first && node->GetLevels().second >= desired_bracket * 10)
                level_nodes.push_back(node);
        }

        ASSERT(!level_nodes.empty());

        float const minSpawnDist = BotCfg::GetWandererMinSpawnDistToPlayer();

        // By leewheel 20260528 - replenish/spawn on maps with no players first (world feels pre-populated).
        NodeVec spawn_pool;
        if (BotCfg::EnableWanderingReplenishPreferEmptyMaps())
        {
            spawn_pool.reserve(level_nodes.size());
            for (WanderNode const* node : level_nodes)
            {
                if (!BotDataMgr::IsWandererMapActive(node->GetMapId()))
                    spawn_pool.push_back(node);
            }
        }

        if (spawn_pool.empty())
        {
            spawn_pool.reserve(level_nodes.size());
            for (WanderNode const* node : level_nodes)
            {
                if (!IsSpawnNodeTooCloseToPlayers(node, minSpawnDist))
                    spawn_pool.push_back(node);
            }
        }

        if (spawn_pool.empty())
            spawn_pool = level_nodes;

        WanderNode const* spawnLoc = Bcore::Containers::SelectRandomContainerElement(spawn_pool);

        CreatureTemplate& bot_template = _botsExtraCreatureTemplates[next_bot_id];
        //copy all fields
        bot_template = *orig_template;
        bot_template.Entry = next_bot_id;
        bot_template.Title = "";
        bot_template.speed_run = BotCfg::GetBotWandererSpeedMod();
        bot_template.KillCredit[0] = orig_entry;
        //bot_template.type_flags |= CREATURE_TYPE_FLAG_FORCE_GOSSIP;

        uint32 max_level = DEFAULT_MAX_LEVEL;
        if (bracketEntry && BotCfg::IsBotLevelCappedByConfigBG())
        {
            max_level = std::min<uint32>(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL), max_level);
            max_level = std::min<uint32>(GetMaxLevelForExpansion(sWorld->getIntConfig(CONFIG_EXPANSION)), max_level);
        }

        if (bracketEntry)
        {
            //force level range for bgs
            bot_template.minlevel = std::min<uint32>(bracketEntry->MinLevel, max_level);
            bot_template.maxlevel = std::min<uint32>(bracketEntry->MaxLevel, max_level);
            if (sWorld->getBoolConfig(CONFIG_BG_XP_FOR_KILL))
                bot_template.flags_extra &= ~(CREATURE_FLAG_EXTRA_NO_XP);
        }
        else
        {
            bot_template.minlevel = std::min<uint32>(std::max<uint32>(desired_bracket * 10, spawnLoc->GetLevels().first), max_level);
            bot_template.maxlevel = std::min<uint32>(std::min<uint32>(desired_bracket * 10 + 9, spawnLoc->GetLevels().second), max_level);
            bot_template.flags_extra &= ~(CREATURE_FLAG_EXTRA_NO_XP);

            if (sWorld->IsFFAPvPRealm())
            {
                bot_template.faction = FACTION_TEMPLATE_NEUTRAL_HOSTILE;
                bot_faction = FACTION_TEMPLATE_NEUTRAL_HOSTILE;
            }
        }

        if (std::string_view wanderName = BotDataMgr::GetNpcBotAppearanceName(orig_entry); !wanderName.empty())
            bot_template.Name = std::string(wanderName);

        bot_template.InitializeQueryData();

        uint8 bot_spec = BotDataMgr::SelectSpecForClass(bot_class);
        _botsData.emplace(std::piecewise_construct, std::forward_as_tuple(next_bot_id), std::forward_as_tuple(BotDataMgr::DefaultRolesForClass(bot_class, bot_spec), bot_faction, bot_spec));
        _botsExtras.emplace(next_bot_id, NpcBotExtras{.race = orig_extras->race, .bclass = bot_class});
        if (NpcBotAppearanceData const* orig_apdata = BotDataMgr::SelectNpcBotAppearance(orig_entry))
            _botsAppearanceData.emplace(std::piecewise_construct, std::forward_as_tuple(next_bot_id),
                std::forward_as_tuple(orig_apdata->name, orig_apdata->gender, orig_apdata->skin, orig_apdata->face, orig_apdata->hair, orig_apdata->haircolor, orig_apdata->features));

        int8 beqId = 1;
        _botsExtraCreatureEquipmentTemplates[next_bot_id] = sObjectMgr->GetEquipmentInfo(orig_entry, beqId);

        //We do not create CreatureData for generated bots

        CellCoord c = Bcore::ComputeCellCoord(spawnLoc->m_positionX, spawnLoc->m_positionY);
        GridCoord g = Bcore::ComputeGridCoord(spawnLoc->m_positionX, spawnLoc->m_positionY);
        ASSERT(c.IsCoordValid(), "Invalid Cell coord!");
        ASSERT(g.IsCoordValid(), "Invalid Grid coord!");
        Map* map = sMapMgr->CreateBaseMap(spawnLoc->GetMapId());
        ASSERT(map->GetEntry()->IsContinent() || map->GetEntry()->IsBattlegroundOrArena(), "%s", map->GetDebugInfo().c_str());

        if (immediate)
            SpawnWandererBot(next_bot_id, spawnLoc, registry);
        else
            _botsWanderCreaturesToSpawn.emplace_back(next_bot_id, spawnLoc);

        _spareBotIdsPerClassMap.at(bot_class).erase(orig_entry);
        if (_spareBotIdsPerClassMap.at(bot_class).empty())
            _spareBotIdsPerClassMap.erase(bot_class);

        return true;
    }

public:
    uint32 GetNextBotId() const { return next_bot_id; }

    uint32 GetEnabledBotsCount() const { return enabledBotsCount; }

    uint32 GetSpareBotsCount(TeamId teamId = TEAM_NEUTRAL) const
    {
        uint32 count = 0;
        for (auto const& [bot_class, bots] : _spareBotIdsPerClassMap)
        {
            if (teamId == TEAM_NEUTRAL)
                count += bots.size();
            else
            {
                if (bot_class >= BOT_CLASS_EX_START)
                {
                    auto cit = wbot_faction_for_ex_class.find(bot_class);
                    if (cit != wbot_faction_for_ex_class.cend() && cit->second == FACTION_TEMPLATE_NEUTRAL_HOSTILE)
                        continue;
                }

                for (uint32 entry : bots)
                {
                    NpcBotExtras const* extras = ASSERT_NOTNULL(BotDataMgr::SelectNpcBotExtras(entry));
                    uint32 bot_faction = BotDataMgr::GetDefaultFactionForBotRaceClass(extras->bclass, extras->race);
                    TeamId bot_team = BotDataMgr::GetTeamIdForFaction(bot_faction);
                    if (teamId == bot_team)
                        ++count;
                }
            }
        }
        return count;
    }

    bool GenerateWanderingBotsToSpawn(uint32 count, int32 map_id, int32 team, bool immediate, PvPDifficultyEntry const* bracketEntry, NpcBotRegistry* registry, uint32& spawned)
    {
        using NodeVec = std::vector<WanderNode const*>;

        if (_spareBotIdsPerClassMap.empty())
            return false;

        std::array<NodeVec, 3> spawns_all{};
        for (NodeVec& vec : spawns_all)
            vec.reserve(WanderNode::GetWPMapsCount() * 20u);

        auto& [spawns_a, spawns_h, spawns_n] = spawns_all;
        WanderNode::DoForAllWPs([map_id = map_id, &spawns_a = spawns_a, &spawns_h = spawns_h, &spawns_n = spawns_n](WanderNode const* wp) {
            MapEntry const* mapEntry = sMapStore.LookupEntry(wp->GetMapId());
            if ((map_id == -1) ? mapEntry->IsWorldMap() : (int32(mapEntry->ID) == map_id))
            {
                if (wp->HasFlag(BotWPFlags::BOTWP_FLAG_SPAWN))
                {
                    if (bot_ai::IsWanderNodeAvailableForBotFaction(wp, FACTION_TEMPLATE_ALLIANCE_DEFAULT, false, true))
                        spawns_a.emplace_back(wp);
                    else if (bot_ai::IsWanderNodeAvailableForBotFaction(wp, FACTION_TEMPLATE_HORDE_DEFAULT, false, true))
                        spawns_h.emplace_back(wp);
                    if (bot_ai::IsWanderNodeAvailableForBotFaction(wp, FACTION_TEMPLATE_NEUTRAL_HOSTILE, false, true))
                        spawns_n.emplace_back(wp);
                }
            }
        });

        bool found_maxlevel_node_a = false;
        bool found_maxlevel_node_h = false;
        bool found_maxlevel_node_n = false;
        const uint8 maxof_minclasslvl_nor = BotDataMgr::GetMinLevelForBotClass(BOT_CLASS_DEATH_KNIGHT); // 55
        const uint8 maxof_minclasslvl_ex = BotDataMgr::GetMinLevelForBotClass(BOT_CLASS_DREADLORD); // 60
        for (WanderNode const* wp : spawns_a)
        {
            if (wp->GetLevels().second >= maxof_minclasslvl_nor)
            {
                found_maxlevel_node_a = true;
                break;
            }
        }
        for (WanderNode const* wp : spawns_h)
        {
            if (wp->GetLevels().second >= maxof_minclasslvl_nor)
            {
                found_maxlevel_node_h = true;
                break;
            }
        }
        for (WanderNode const* wp : spawns_n)
        {
            if (wp->GetLevels().second >= maxof_minclasslvl_ex)
            {
                found_maxlevel_node_n = true;
                break;
            }
        }

        PctBrackets bracketPcts{};
        PctBrackets bots_per_bracket{};

        std::vector<std::pair<uint8, uint32>> teamSpareBotIdsPerClass;
        teamSpareBotIdsPerClass.reserve(count);

        if (team == -1)
        {
            if (!found_maxlevel_node_a || !found_maxlevel_node_h || !found_maxlevel_node_n)
                return false;

            //make a full copy
            for (auto const& [bot_class, spare_bots] : _spareBotIdsPerClassMap)
                for (uint32 spareBotId : spare_bots)
                    teamSpareBotIdsPerClass.emplace_back(bot_class, spareBotId);
            bracketPcts = BotCfg::GetBotWandererLevelBrackets();
        }
        else
        {
            ASSERT(bracketEntry);

            bracketPcts[bracketEntry->MinLevel / 10] = 100u;
            switch (team)
            {
                case ALLIANCE:
                    if (!found_maxlevel_node_a)
                        return false;
                    break;
                case HORDE:
                    if (!found_maxlevel_node_h)
                        return false;
                    break;
                case TEAM_OTHER:
                default:
                    if (!found_maxlevel_node_n)
                        return false;
                    break;
            }

            for (auto const& [bot_class, spare_bots] : _spareBotIdsPerClassMap)
            {
                for (uint32 spareBotId : spare_bots)
                {
                    NpcBotExtras const* orig_extras = ASSERT_NOTNULL(BotDataMgr::SelectNpcBotExtras(spareBotId));
                    uint32 bot_faction = BotDataMgr::GetDefaultFactionForBotRaceClass(orig_extras->bclass, orig_extras->race);
                    uint32 botTeam = BotDataMgr::GetTeamForFaction(bot_faction);

                    if (int32(botTeam) != team)
                        continue;

                    if (BotDataMgr::GetMinLevelForBotClass(bot_class) > bracketEntry->MaxLevel)
                        continue;

                    teamSpareBotIdsPerClass.emplace_back(bot_class, spareBotId);
                }
            }
        }

        if (teamSpareBotIdsPerClass.empty())
            return false;

        uint32 total_bots_in_brackets = 0;
        for (size_t k{}; k < BRACKETS_COUNT; ++k)
        {
            if (!bracketPcts[k])
                continue;
            bots_per_bracket[k] = CalculatePct(count, bracketPcts[k]);
            total_bots_in_brackets += bots_per_bracket[k];
        }
        for (std::make_signed_t<std::size_t> j = BRACKETS_COUNT - 1; j >= 0; --j)
        {
            if (bots_per_bracket[j])
            {
                bots_per_bracket[j] += count - total_bots_in_brackets;
                break;
            }
        }

        std::vector<uint8> brackets_shuffled;
        brackets_shuffled.reserve(count);
        for (uint8 bracket{}; bracket < BRACKETS_COUNT; ++bracket)
        {
            while (bots_per_bracket[bracket])
            {
                brackets_shuffled.push_back(bracket);
                --bots_per_bracket[bracket];
            }
        }

        Bcore::Containers::RandomShuffle(teamSpareBotIdsPerClass);
        Bcore::Containers::RandomShuffle(brackets_shuffled);

        for (std::size_t i{}; i < brackets_shuffled.size() && !teamSpareBotIdsPerClass.empty();) // i is a counter, NOT used as index or value
        {
            uint8 bracket = brackets_shuffled[i];

            int8 tries = 100;
            do {
                --tries;
                if (GenerateWanderingBotToSpawn(teamSpareBotIdsPerClass.back(), bracket, spawns_a, spawns_h, spawns_n, immediate, bracketEntry, registry))
                {
                    ++i;
                    ++spawned;
                    teamSpareBotIdsPerClass.pop_back();
                    break;
                }
            } while (tries >= 0);

            if (tries < 0)
                return false;
        }

        CharacterDatabase.PExecute("UPDATE worldstates SET value = {} WHERE entry = {}", next_bot_id, uint32(BOT_GIVER_ENTRY));

        return true;
    }

    bool GenerateDungeonBotsToSpawn(Player const* owner, uint32 bots_count, std::array<uint8, 3>&& desired_roles)
    {
        // PLAYER_ROLE_TANK = 0, PLAYER_ROLE_HEALER = 1, PLAYER_ROLE_DAMAGE = 2
        using enum lfg::LfgRoles;
        using tuple_type = std::tuple<uint32, uint8, uint32>;
        using roles_arr = std::decay_t<decltype(desired_roles)>;

        if (_spareBotIdsPerClassMap.empty())
            return false;

        auto roles_array_to_bot_role_mask = [](roles_arr const& roles) {
            return static_cast<uint32>(
                (!!roles[0] ? static_cast<uint32>(BOT_ROLE_TANK) : 0) |
                (!!roles[1] ? static_cast<uint32>(BOT_ROLE_HEAL) : 0) |
                (!!roles[2] ? static_cast<uint32>(BOT_ROLE_DPS) : 0));
        };

        std::vector<tuple_type> spareBots;
        spareBots.reserve(bots_count);

        for (auto const& [bot_class, spare_bots] : _spareBotIdsPerClassMap)
        {
            const uint32 bot_roles = BotDataMgr::GetViableRolesForClass(bot_class) & roles_array_to_bot_role_mask(desired_roles);
            if (!bot_roles)
                continue;

            for (const uint32 spareBotId : spare_bots)
            {
                if (IsArenaBotLockedForOwnerImpl(spareBotId, owner->GetGUID().GetCounter()))
                    continue;

                if (BotCfg::FilterRaces())
                {
                    NpcBotExtras const* orig_extras = ASSERT_NOTNULL(BotDataMgr::SelectNpcBotExtras(spareBotId));
                    const uint32 bot_faction = BotDataMgr::GetDefaultFactionForBotRaceClass(orig_extras->bclass, orig_extras->race);
                    const uint32 botTeam = BotDataMgr::GetTeamForFaction(bot_faction);

                    if (botTeam != owner->GetTeam())
                        continue;
                }

                if (BotDataMgr::GetMinLevelForBotClass(bot_class) > owner->GetLevel())
                    continue;

                spareBots.emplace_back(spareBotId, bot_class, bot_roles);
            }
        }

        Bcore::Containers::RandomShuffle(spareBots);

        roles_arr remaining_roles = desired_roles; // copy

        for (auto const& ecr : spareBots)
        {
            const uint32 remaining_bot_roles_mask = roles_array_to_bot_role_mask(remaining_roles);
            if (!remaining_bot_roles_mask)
                break;

            if (uint32 broles_mask = std::get<2>(ecr) & remaining_bot_roles_mask)
            {
                const uint32 first_role = BotDataMgr::GetFirstRoleInMask(broles_mask);
                broles_mask = first_role | BOT_ROLE_PARTY | BOT_ROLE_DPS;
                //if (broles_mask & BOT_ROLE_TANK)
                //    broles_mask |= BOT_ROLE_DPS;
                if (first_role == BOT_ROLE_TANK)
                    broles_mask &= ~BOT_ROLE_RANGED;
                if ((!BotDataMgr::IsMeleeClass(std::get<1>(ecr)) || first_role == BOT_ROLE_HEAL) && first_role != BOT_ROLE_TANK)
                    broles_mask |= BOT_ROLE_RANGED;

                const uint8 bot_spec = BotDataMgr::SelectBotSpecForRoles(std::get<1>(ecr), first_role);

                if ((broles_mask & BOT_ROLE_RANGED) && BotDataMgr::IsMeleeSpec(bot_spec))
                    broles_mask &= ~BOT_ROLE_RANGED;

                GenerateDungeonBotToSpawn({ std::get<0>(ecr), std::get<1>(ecr), bot_spec, broles_mask }, owner);
                for (const uint32 brmask : { BOT_ROLE_DPS, BOT_ROLE_HEAL, BOT_ROLE_TANK })
                {
                    if (first_role & brmask)
                    {
                        const uint8 brindex = (brmask == BOT_ROLE_TANK) ? 0 : (brmask == BOT_ROLE_HEAL) ? 1 : 2;
                        ASSERT(remaining_roles[brindex] > 0);
                        remaining_roles[brindex]--;
                        break;
                    }
                }
            }
        }

        CharacterDatabase.PExecute("UPDATE worldstates SET value = {} WHERE entry = {}", next_bot_id, uint32(BOT_GIVER_ENTRY));

        if (std::ranges::any_of(remaining_roles, [](uint8 val) { return !!val; }))
        {
            BOT_LOG_WARN("npcbot", "Failed to summon enough bots for player {} ({})! Remaining: {}, {}, {}", owner->GetName(), owner->GetGUID().GetCounter(),
                uint32(remaining_roles[0]), uint32(remaining_roles[1]), uint32(remaining_roles[2]));
            return false;
        }

        return true;
    }

    uint32 GenerateArenaBotsToSpawn(Player const* owner, uint8 arenaType, uint8 desiredBots, std::vector<ArenaBotRosterSlot>& roster, bool& rosterChanged)
    {
        if (!owner || arenaType < 2 || arenaType > 5 || _spareBotIdsPerClassMap.empty())
            return 0;
        auto isArenaAllowedClass = [](uint8 botClass) -> bool
        {
            // By leewheel 20260528 - arena fairness policy: disallow hero/extra classes in arena teams.
            return botClass >= BOT_CLASS_WARRIOR && botClass < BOT_CLASS_EX_START;
        };

        // By leewheel 20260528 - strict arena DPS-only spec resolver (exclude tank/heal specs).
        auto selectArenaDpsSpecForClass = [](uint8 botClass) -> uint8
        {
            switch (botClass)
            {
                case BOT_CLASS_WARRIOR:      return urand(0, 1) ? BOT_SPEC_WARRIOR_ARMS : BOT_SPEC_WARRIOR_FURY;
                case BOT_CLASS_PALADIN:      return BOT_SPEC_PALADIN_RETRIBUTION;
                case BOT_CLASS_HUNTER:       return Bcore::Containers::SelectRandomContainerElement(std::array<uint8, 3>{ BOT_SPEC_HUNTER_BEASTMASTERY, BOT_SPEC_HUNTER_MARKSMANSHIP, BOT_SPEC_HUNTER_SURVIVAL });
                case BOT_CLASS_ROGUE:        return Bcore::Containers::SelectRandomContainerElement(std::array<uint8, 3>{ BOT_SPEC_ROGUE_ASSASINATION, BOT_SPEC_ROGUE_COMBAT, BOT_SPEC_ROGUE_SUBTLETY });
                case BOT_CLASS_PRIEST:       return BOT_SPEC_PRIEST_SHADOW;
                case BOT_CLASS_DEATH_KNIGHT: return BOT_SPEC_DK_UNHOLY;
                case BOT_CLASS_SHAMAN:       return urand(0, 1) ? BOT_SPEC_SHAMAN_ELEMENTAL : BOT_SPEC_SHAMAN_ENHANCEMENT;
                case BOT_CLASS_MAGE:         return Bcore::Containers::SelectRandomContainerElement(std::array<uint8, 3>{ BOT_SPEC_MAGE_ARCANE, BOT_SPEC_MAGE_FIRE, BOT_SPEC_MAGE_FROST });
                case BOT_CLASS_WARLOCK:      return Bcore::Containers::SelectRandomContainerElement(std::array<uint8, 3>{ BOT_SPEC_WARLOCK_AFFLICTION, BOT_SPEC_WARLOCK_DEMONOLOGY, BOT_SPEC_WARLOCK_DESTRUCTION });
                case BOT_CLASS_DRUID:        return BOT_SPEC_DRUID_BALANCE;
                default:                     return BOT_SPEC_DEFAULT;
            }
        };

        auto isArenaDisallowedSpec = [](uint8 spec) -> bool
        {
            switch (spec)
            {
                case BOT_SPEC_WARRIOR_PROTECTION:
                case BOT_SPEC_PALADIN_HOLY:
                case BOT_SPEC_PALADIN_PROTECTION:
                case BOT_SPEC_PRIEST_DISCIPLINE:
                case BOT_SPEC_PRIEST_HOLY:
                case BOT_SPEC_DK_BLOOD:
                case BOT_SPEC_SHAMAN_RESTORATION:
                case BOT_SPEC_DRUID_RESTORATION:
                case BOT_SPEC_DRUID_FERAL:
                    return true;
                default:
                    return false;
            }
        };

        uint32 const targetBots = desiredBots > 0 ? desiredBots : uint8(arenaType - 1);
        uint32 spawned = 0;

        for (uint32 i = 0; i < targetBots && !_spareBotIdsPerClassMap.empty(); ++i)
        {
            uint32 desiredEntry = 0;
            uint8 desiredClass = BOT_CLASS_NONE;
            uint8 desiredSpec = BOT_SPEC_DEFAULT;
            if (i < roster.size())
            {
                desiredEntry = roster[i].botEntry;
                desiredClass = roster[i].botClass;
                desiredSpec = roster[i].botSpec;
            }

            uint8 botClass = BOT_CLASS_NONE;
            uint32 spareBotId = 0;

            auto classItr = _spareBotIdsPerClassMap.find(desiredClass);
            if (isArenaAllowedClass(desiredClass) && desiredEntry != 0 && classItr != _spareBotIdsPerClassMap.end() && classItr->second.contains(desiredEntry))
            {
                botClass = desiredClass;
                spareBotId = desiredEntry;
            }
            else if (isArenaAllowedClass(desiredClass) && classItr != _spareBotIdsPerClassMap.end() && !classItr->second.empty())
            {
                botClass = desiredClass;
                spareBotId = *classItr->second.begin();
            }
            else
            {
                auto anyItr = std::find_if(_spareBotIdsPerClassMap.begin(), _spareBotIdsPerClassMap.end(),
                    [&isArenaAllowedClass](auto const& kv) { return isArenaAllowedClass(kv.first) && !kv.second.empty(); });
                if (anyItr == _spareBotIdsPerClassMap.end())
                    break;
                botClass = anyItr->first;
                spareBotId = *anyItr->second.begin();
            }

            uint8 arenaSpec = desiredSpec;
            if (!BotDataMgr::IsValidSpecForClass(botClass, arenaSpec) || arenaSpec == BOT_SPEC_DEFAULT || isArenaDisallowedSpec(arenaSpec))
                arenaSpec = selectArenaDpsSpecForClass(botClass);

            if (i < roster.size() && (roster[i].botEntry != spareBotId || roster[i].botClass != botClass))
            {
                roster[i].botEntry = spareBotId;
                roster[i].botClass = botClass;
                rosterChanged = true;
            }

            // By leewheel 20260528 - arena policy: fixed roster + DPS-only + grouped bot.
            uint32 arenaRoles = BOT_ROLE_DPS | BOT_ROLE_PARTY;
            if (!BotDataMgr::IsMeleeSpec(arenaSpec))
                arenaRoles |= BOT_ROLE_RANGED;

            GenerateDungeonBotToSpawn({ spareBotId, botClass, arenaSpec, arenaRoles }, owner);
            _arenaBotOwnerLockMap[spareBotId] = owner->GetGUID().GetCounter();
            ++spawned;
        }

        CharacterDatabase.PExecute("UPDATE worldstates SET value = {} WHERE entry = {}", next_bot_id, uint32(BOT_GIVER_ENTRY));
        return spawned;
    }

    static void CleanExtraBotData(Creature const* bot)
    {
        const uint8 bot_class = bot->GetBotClass();
        const uint32 bot_despawn_id = bot->GetEntry();
        const uint32 original_id = _botsExtraCreatureTemplates.at(bot_despawn_id).KillCredit[0];

        auto bditr = _botsData.find(bot_despawn_id);
        auto beitr = _botsExtras.find(bot_despawn_id);
        auto baditr = _botsAppearanceData.find(bot_despawn_id);
        auto bwcetitr = _botsExtraCreatureEquipmentTemplates.find(bot_despawn_id);
        auto bwctitr = _botsExtraCreatureTemplates.find(bot_despawn_id);

        ASSERT(bditr != _botsData.end());
        ASSERT(beitr != _botsExtras.end());
        //ASSERT(baditr != _botsAppearanceData.end()); may not exist
        ASSERT(bwcetitr != _botsExtraCreatureEquipmentTemplates.end());
        ASSERT(bwctitr != _botsExtraCreatureTemplates.end());

        _botsData.erase(bditr);
        _botsExtras.erase(beitr);
        if (baditr != _botsAppearanceData.end())
            _botsAppearanceData.erase(baditr);
        _botsExtraCreatureEquipmentTemplates.erase(bwcetitr);
        _botsExtraCreatureTemplates.erase(bwctitr);

        _spareBotIdsPerClassMap[bot_class].insert(original_id);
    }

};

// By leewheel 20260528 - keep singleton accessor as free function to avoid IDE E0135 false-positives.
static WanderingBotsGenerator* GetWanderingBotsGenerator()
{
    static WanderingBotsGenerator instance;
    return &instance;
}

#define sBotGen GetWanderingBotsGenerator()

void BotDataMgr::UpdateWandererLogSampler(uint32 diff)
{
    const uint32 desiredCount = BotCfg::GetDesiredWanderingBotsCount();
    if (desiredCount == 0)
        return;

    static uint32 wandererLogTimer = 0;
    wandererLogTimer += diff;

    const uint32 intervalMs = BotCfg::GetWanderingBotsLogSampleIntervalMs();
    if (wandererLogTimer < intervalMs)
        return;
    wandererLogTimer -= intervalMs;

    std::vector<Creature const*> wanderers;
    wanderers.reserve(desiredCount);
    for (Creature const* bot : _existingBots)
    {
        if (bot && bot->IsInWorld() && bot->IsWandererBot() && bot->GetBotAI())
            wanderers.push_back(bot);
    }

    const uint32 activeCount = uint32(wanderers.size());
    uint32 sampleCount = BotCfg::GetWanderingBotsLogSampleCount();
    sampleCount = std::min(sampleCount, desiredCount);
    sampleCount = std::min(sampleCount, activeCount);

    if (sampleCount)
        Bcore::Containers::RandomResize(wanderers, sampleCount);

    time_t const now = GameTime::GetGameTime();
    time_t const startTime = GameTime::GetStartTime();
    uint32 const uptimeSec = uint32(std::max<time_t>(now - startTime, 0));

    BOT_LOG_INFO("npcbots",
        "\n 随机漫游机器人行为样本:\n"
        "========================================================================================\n"
        " 当前时间: {} | 服务启动: {} | 总运行: {} | 内存: {} MB\n"
        " 抽取 {} 个样本 / 当前激活 {} 个 / 配置上限 {} 个 / 取样间隔 {}s\n"
        "{}\n"
        "========================================================================================",
        TimeToTimestampStr(now), TimeToTimestampStr(startTime), FormatUptimeDuration(uptimeSec), GetProcessWorkingSetMB(),
        sampleCount, activeCount, desiredCount, intervalMs / IN_MILLISECONDS, FormatWandererPeriodStats());

    if (sampleCount)
    {
        bot_ai::LogWandererBehaviorSampleHeader();
        for (Creature const* bot : wanderers)
            bot->GetBotAI()->LogWandererBehaviorSample();
    }

    BOT_LOG_INFO("npcbots", "========================================================================================");
    s_wandererPeriodStats.Clear();
}

// Counts wanderers already allocated (in world, off-grid, or queued to spawn).
static uint32 CountAllocatedWanderers()
{
    uint32 count = uint32(_botsWanderCreaturesToSpawn.size());
    for (Creature const* bot : _existingBots)
        if (bot && bot->IsWandererBot())
            ++count;
    return count;
}

void BotDataMgr::TryReplenishWanderingBots()
{
    uint32 const desiredConfig = BotCfg::GetDesiredWanderingBotsCount();
    if (!desiredConfig)
        return;

    uint32 const allocated = CountAllocatedWanderers();
    if (allocated >= desiredConfig)
        return;

    uint32 const spare = sBotGen->GetSpareBotsCount();
    uint32 need = desiredConfig - allocated;
    need = std::min(need, spare);
    if (!need)
    {
        // Config may exceed available templates (e.g. players hired bots since startup).
        static bool loggedCapacityCeiling = false;
        if (!loggedCapacityCeiling)
        {
            loggedCapacityCeiling = true;
            BOT_LOG_INFO("npcbots",
                "TryReplenishWanderingBots: wanderer template pool exhausted (allocated {}, config desired {}, spare {}). "
                "Reduce NpcBot.WanderingBots.Continents.Count or expect fewer roaming bots while bots are hired.",
                allocated, desiredConfig, spare);
        }
        return;
    }

    uint32 spawned = 0;
    if (!sBotGen->GenerateWanderingBotsToSpawn(need, -1, -1, false, nullptr, nullptr, spawned))
    {
        static time_t lastWarnTime = 0;
        time_t const now = GameTime::GetGameTime();
        if (now - lastWarnTime >= 300)
        {
            lastWarnTime = now;
            BOT_LOG_WARN("npcbots",
                "TryReplenishWanderingBots: failed to queue {} wanderers (allocated {}, config desired {}, spare {})",
                need, allocated, desiredConfig, spare);
        }
    }
    else if (spawned)
        s_wandererPeriodStats.RecordReplenishQueued(spawned);
}

void BotDataMgr::PushBotChatSceneEvent(Creature const* bot, std::string_view eventText)
{
    if (!bot || eventText.empty())
        return;
    PushSceneEventByEntry(bot->GetEntry(), eventText);
}

void BotDataMgr::NotifyBotCombatScene(Creature const* bot, Unit const* target)
{
    if (!BotCfg::IsBotChatEnabled() || !bot || !target || !bot->GetBotAI() || bot->GetBotAI()->IAmFree())
        return;
    if (!ResolveBotChatGroup(bot) && !BotHasHiredSquad(bot))
        return;

    bool const boss = target->IsCreature() && target->ToCreature()->IsDungeonBoss();
    PushSceneEventByEntry(bot->GetEntry(), Bcore::StringFormat(
        "event={}; target={}; map={}; hp_pct={}",
        boss ? "BOSS_PULL" : "COMBAT_PULL",
        target->GetName(), bot->GetMapId(), uint32(target->GetHealthPct())));

    _botChatUrgentReplyEntries.insert(bot->GetEntry());
    _botChatUrgentChannelByEntry[bot->GetEntry()] = "小队频道";
    _botChatUrgentMapByEntry[bot->GetEntry()] = bot->GetMapId();

    uint32 const nowMs = GameTime::GetGameTimeMS();
    uint32& cd = _botChatCooldownUntilMs[bot->GetEntry()];
    if (cd > nowMs + 4000)
        cd = nowMs + urand(1500, 4000);
}

void BotDataMgr::NotifyBotCastScene(Creature const* bot, Unit const* caster, uint32 spellId)
{
    if (!BotCfg::IsBotChatEnabled() || !bot || !caster || !spellId || !bot->GetBotAI() || bot->GetBotAI()->IAmFree())
        return;
    if (!ResolveBotChatGroup(bot) && !BotHasHiredSquad(bot))
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return;

    bool const boss = caster->IsCreature() && caster->ToCreature()->IsDungeonBoss();
    if (!boss && !caster->IsInCombatWith(bot))
        return;

    PushSceneEventByEntry(bot->GetEntry(), Bcore::StringFormat(
        "event=BOSS_CAST; caster={}; spell={}; map={}",
        caster->GetName(), spellInfo->SpellName[0], bot->GetMapId()));

    _botChatUrgentReplyEntries.insert(bot->GetEntry());
    _botChatUrgentChannelByEntry[bot->GetEntry()] = bot->GetMap() && bot->GetMap()->IsRaid() ? "团队频道" : "小队频道";
    _botChatUrgentMapByEntry[bot->GetEntry()] = bot->GetMapId();

    uint32 const nowMs = GameTime::GetGameTimeMS();
    uint32& cd = _botChatCooldownUntilMs[bot->GetEntry()];
    if (cd > nowMs + 3000)
        cd = nowMs + urand(800, 2500);
}

void BotDataMgr::UpdateBotAmbientChat(Creature* bot, uint32 diff)
{
    if (!BotCfg::IsBotChatEnabled() || !bot || !bot->IsInWorld() || !bot->GetBotAI() || bot->GetBotAI()->IAmFree())
        return;
    if (!ResolveBotChatGroup(bot) && !BotHasHiredSquad(bot))
        return;

    uint32 const entry = bot->GetEntry();
    uint32& timer = _botAmbientChatTimerMs[entry];
    if (timer > diff)
    {
        timer -= diff;
        return;
    }

    timer = urand(12000, 28000);
    if (bot->IsInCombat())
        timer = urand(6000, 14000);

    if (HasPendingSceneEvent(entry))
        return;

    SeedProactivePartySceneEvent(bot);
}

void BotDataMgr::PushBotChatPvpDefeatEvent(Creature const* bot, std::string_view killerName)
{
    if (!bot || killerName.empty())
        return;
    RememberPvpKillerByEntry(bot->GetEntry(), killerName);
    _botChatPvpWinStreak[bot->GetEntry()] = 0;
    _botChatPvpLoseStreak[bot->GetEntry()] = std::min<uint8>(10, uint8(_botChatPvpLoseStreak[bot->GetEntry()] + 1));
}

void BotDataMgr::PushBotChatPvpKillEvent(Creature const* bot, std::string_view victimName)
{
    if (!bot || victimName.empty())
        return;
    RememberPvpVictimByEntry(bot->GetEntry(), victimName);
    _botChatPvpLoseStreak[bot->GetEntry()] = 0;
    _botChatPvpWinStreak[bot->GetEntry()] = std::min<uint8>(10, uint8(_botChatPvpWinStreak[bot->GetEntry()] + 1));
}

void BotDataMgr::PushBotChatPvpAssaultEvent(Creature const* bot, std::string_view attackerName)
{
    if (!bot || attackerName.empty() || bot->GetEntry() == 0)
        return;
    if (HasPendingSceneEvent(bot->GetEntry()))
        return;
    // Set PvP killer so worldCall distress triggers
    RememberPvpKillerByEntry(bot->GetEntry(), attackerName);
    // Push a PvP assault scene event so the bot can cry for help while still alive
    PushSceneEventByEntry(bot->GetEntry(), Bcore::StringFormat(
        "event=PVP_ASSAULT; attacker={}; map={}; x={:.1f}; y={:.1f}",
        attackerName, bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY()));
}

void BotDataMgr::UpdateWandererSocial(Creature* bot)
{
    if (!bot || !bot->IsInWorld() || !bot->IsAlive() || !bot->IsWandererBot())
        return;

    WandererVendor::OnWandererTick(bot);

    if (!BotCfg::IsWandererGreetEnabled() || !BotCfg::IsBotChatEnabled())
        return;

    if (bot->IsInCombat())
        return;

    Map* map = bot->GetMap();
    if (!map)
        return;

    float const greetDist = BotCfg::GetWandererGreetDist();
    bool greeted = false;
    for (MapReference const& ref : map->GetPlayers())
    {
        if (greeted)
            break;

        Player* player = ref.GetSource();
        if (!player || !player->IsAlive() || player->IsGameMaster())
            continue;

        if (!bot->IsWithinDistInMap(player, greetDist))
            continue;

        TryWandererGreetPlayer(bot, player);
        greeted = true;
    }
}

bool BotDataMgr::TryHandleWhisperToWandererBot(Player* player, std::string_view targetName, std::string_view message)
{
    if (!player || targetName.empty() || message.empty())
        return false;

    Creature const* bot = FindBot(targetName, player->GetSession()->GetSessionDbcLocale());
    if (!bot || !bot->IsInWorld())
        return false;

    Creature* mutableBot = const_cast<Creature*>(bot);
    if (bot->IsWandererBot() && WandererVendor::TryHandlePlayerWhisper(player, mutableBot, message))
        return true;

    if (!BotCfg::IsBotChatEnabled() || !bot->IsAlive())
        return false;

    PushSceneEventByEntry(bot->GetEntry(), Bcore::StringFormat(
        "event=PLAYER_WHISPER; speaker={}; text={}", player->GetName(), message));
    std::string reply = BuildBotChatTemplate(bot, "密语");
    if (reply.empty())
        reply = "收到，我在。";
    mutableBot->Whisper(reply, LANG_UNIVERSAL, player);
    _botChatCooldownUntilMs[bot->GetEntry()] = GameTime::GetGameTimeMS() + 5000u;
    return true;
}

void BotDataMgr::OnPlayerChannelChat(Player const* player, std::string_view channelName, std::string_view message, Group const* group)
{
    if (!player || message.empty() || !BotCfg::IsBotChatEnabled())
        return;
    if (channelName.empty())
        return;

    bool const groupChannel = IsBotChatGroupChannel(channelName);
    Group const* playerGroup = groupChannel ? (group ? group : player->GetGroup()) : nullptr;
    if (groupChannel && !playerGroup)
        return;

    // By leewheel 20260528 - direct mention always forces that bot to answer, except party/raid is scoped to the player's group.
    std::vector<MentionedBot> const mentionedBots = FindMentionedBotsByMessage(message);
    for (MentionedBot const& mentioned : mentionedBots)
    {
        Creature const* bot = mentioned.bot;
        if (groupChannel)
        {
            bot_ai const* ai = bot->GetBotAI();
            if (!ai || ai->GetGroup() != playerGroup)
                continue;
        }

        std::string evt = Bcore::StringFormat("event=PLAYER_MENTION; channel={}; speaker={}; text={}",
            channelName, player->GetName(), message);
        PushSceneEventByEntry(bot->GetEntry(), evt);
        _botChatUrgentReplyEntries.insert(bot->GetEntry());
        _botChatDirectMentionEntries.insert(bot->GetEntry());
        _botChatDirectMentionFirstPosByEntry[bot->GetEntry()] = mentioned.firstPos;
        _botChatUrgentChannelByEntry[bot->GetEntry()] = std::string(channelName);
        _botChatUrgentMapByEntry[bot->GetEntry()] = player->GetMapId();
        _botChatCooldownUntilMs[bot->GetEntry()] = 0;
    }

    if (groupChannel && !mentionedBots.empty())
        return;

    // By leewheel 20260528 - player channel messages seed nearby bot reply context (city/world immersion).
    uint32 seeded = 0;
    for (Creature const* bot : _existingBots)
    {
        if (!bot || !bot->IsInWorld() || !bot->IsAlive())
            continue;
        if (bot->GetMapId() != player->GetMapId())
            continue;
        if (groupChannel)
        {
            bot_ai const* ai = bot->GetBotAI();
            if (!ai || ai->GetGroup() != playerGroup)
                continue;
        }
        // By leewheel 20260528 - non-party channel interaction is map-wide, not short-range.

        std::string evt = Bcore::StringFormat("event=PLAYER_CHAT; channel={}; speaker={}; text={}",
            channelName, player->GetName(), message);
        PushSceneEventByEntry(bot->GetEntry(), evt);
        _botChatUrgentReplyEntries.insert(bot->GetEntry());
        _botChatUrgentChannelByEntry[bot->GetEntry()] = std::string(channelName);
        _botChatUrgentMapByEntry[bot->GetEntry()] = player->GetMapId();
        _botChatCooldownUntilMs[bot->GetEntry()] = 0; // allow immediate response on next tick
        ++seeded;
        if (seeded >= 8)
            break;
    }
}

bool BotDataMgr::IsWandererMapActive(uint32 mapId)
{
    // By leewheel 20260528 - centralized "map has players" probe for wanderer dormancy/activation.
    Map* map = sMapMgr->CreateBaseMap(mapId);
    return map && map->HavePlayers();
}

void BotDataMgr::NotifyPlayerEnteredWorldMap(Player const* player)
{
    if (!player)
        return;

    Map const* map = player->GetMap();
    if (!map || !map->GetEntry()->IsWorldMap())
        return;

    uint32 const graceSec = BotCfg::GetWandererMapWakeGraceSec();
    if (!graceSec)
        return;

    _wandererMapWakeGraceUntil[player->GetMapId()] = GameTime::GetGameTime() + graceSec;
}

bool BotDataMgr::IsWandererMapInWakeGracePeriod(uint32 mapId)
{
    auto itr = _wandererMapWakeGraceUntil.find(mapId);
    if (itr == _wandererMapWakeGraceUntil.end())
        return false;

    if (GameTime::GetGameTime() >= itr->second)
    {
        _wandererMapWakeGraceUntil.erase(itr);
        return false;
    }

    return true;
}

void BotDataMgr::UpdateWandererGridRecycle(uint32 diff)
{
    // By leewheel 20260528 - keep wanderers persistent across world maps (do not despawn on empty maps).
    // Dormant/active behavior is handled in bot_ai::Evade() using IsWandererMapActive().
    // Keep wanderers globally present. Empty maps are now "dormant" in bot_ai:
    // bots stop roaming and idle/sit until a player appears on that map.
    (void)diff;
}

namespace
{
    bool s_llmEnableCache = false;
    bool s_llmUseGpuCache = false;
    std::string s_llmPathCache;

    void RefreshNpcBotLLMConfig()
    {
        bool const llmEnableNow = BotCfg::IsBotChatLLMEnabled();
        bool const llmUseGpuNow = BotCfg::IsBotChatLLMUseGpu();
        std::string llmPathNow = BotCfg::GetBotChatLLMModelName().empty() ? BotCfg::GetBotChatLLMModelPath() : BotCfg::GetBotChatLLMModelName();
        if (llmEnableNow == s_llmEnableCache && llmPathNow == s_llmPathCache && llmUseGpuNow == s_llmUseGpuCache)
            return;

        NpcBotChatLLM::Engine::Instance().Configure(llmEnableNow, llmPathNow, llmUseGpuNow);
        s_llmEnableCache = llmEnableNow;
        s_llmUseGpuCache = llmUseGpuNow;
        s_llmPathCache = std::move(llmPathNow);
    }
}

void BotDataMgr::InitNpcBotLLM()
{
#ifdef TRINITY_NPCBOT_LLM_EMBED
    // --- 1. 检查 CUDA 可用性 ---
    bool const gpuAvailable = llama_supports_gpu_offload();
    if (!gpuAvailable)
    {
        TC_LOG_INFO("server.loading", ">> NpcBot LLM: disabled — no CUDA/GPU backend detected on this machine");
        return;
    }

    // --- 2. 扫描模型目录下的 .gguf 文件 ---
    std::filesystem::path modelDir = std::filesystem::path(".") / "ClientData" / "Ai.Mod";
    std::vector<std::filesystem::path> ggufFiles;
    std::error_code ec;
    if (std::filesystem::exists(modelDir, ec))
    {
        for (auto const& entry : std::filesystem::directory_iterator(modelDir, ec))
            if (entry.path().extension() == ".gguf")
                ggufFiles.push_back(entry.path());
    }

    // --- 3. 根据 gguf 数量决定加载哪个 ---
    std::string selectedModel;
    if (ggufFiles.empty())
    {
        TC_LOG_INFO("server.loading", ">> NpcBot LLM: no .gguf model found under '{}'", modelDir.string());
        return;
    }
    else if (ggufFiles.size() == 1)
    {
        // A: 只有一个，直接用
        selectedModel = ggufFiles[0].string();
        TC_LOG_INFO("server.loading", ">> NpcBot LLM: auto-detected single model '{}'", ggufFiles[0].filename().string());
    }
    else
    {
        // B: 有多个，匹配 NpcBot.Chat.LLM.ModelName
        std::string const desired = BotCfg::GetBotChatLLMModelName();
        if (desired.empty())
        {
            TC_LOG_ERROR("server.loading", ">> NpcBot LLM: multiple .gguf models found but NpcBot.Chat.LLM.ModelName is not set");
            return;
        }
        bool found = false;
        for (auto const& gf : ggufFiles)
        {
            if (gf.filename().string() == desired || gf.stem().string() == desired)
            {
                selectedModel = gf.string();
                found = true;
                break;
            }
        }
        if (!found)
        {
            TC_LOG_ERROR("server.loading", ">> NpcBot LLM: model '{}' not found among available .gguf files", desired);
            return;
        }
    }

    // --- 4. 加载选定模型 ---
    TC_LOG_INFO("server.loading", ">> NpcBot LLM: loading '{}' (Device=gpu, cwd={})...",
        std::filesystem::path(selectedModel).filename().string(), std::filesystem::current_path().string());

    NpcBotChatLLM::Engine::Instance().Configure(true, selectedModel, true);

    if (NpcBotChatLLM::Engine::Instance().IsEnabled())
    {
        // Cache values to prevent RefreshNpcBotLLMConfig() from re-loading on first Update() tick
        s_llmEnableCache = true;
        s_llmUseGpuCache = true;
        s_llmPathCache = selectedModel;
        TC_LOG_INFO("server.loading", ">> NpcBot LLM: ready (see npcbots log for GPU/VRAM details)");
    }
    else
        TC_LOG_ERROR("server.loading", ">> NpcBot LLM: failed to load '{}' — check Server.log for details",
            std::filesystem::path(selectedModel).filename().string());
#else
    TC_LOG_INFO("server.loading", ">> NpcBot LLM: disabled — worldserver built WITHOUT embedded llama");
#endif
}

void BotDataMgr::Update(uint32 diff)
{
    RefreshNpcBotLLMConfig();

    UpdateWandererLogSampler(diff);
    UpdateWandererGridRecycle(diff);
    TryBotChannelChat(diff); // By leewheel 20260528 - world/party/raid chat ticker.

    botSpawnEvents.Update(diff);
    for (auto& [_, events] : botBGJoinEvents)
        events.Update(diff);

    //lock is not needed here
    for (Creature const* bot : _existingBots)
    {
        if (!bot->IsInWorld() && bot->FindMap() && !bot->IsWandererBot() && !!SelectNpcBotData(bot->GetEntry()) && bot->IsFreeBot())
        {
            bot->GetBotAI()->CommonTimers(diff);
            bot->GetBotAI()->UpdateAI(diff);
        }
    }

    if (!_botsExtraCreaturesToDespawn.empty())
    {
        BOT_LOG_DEBUG("npcbots", "Bots to despawn: {}", uint32(_botsExtraCreaturesToDespawn.size()));

        while (!_botsExtraCreaturesToDespawn.empty())
        {
            auto bot_despawn_iter = _botsExtraCreaturesToDespawn.begin();

            const uint32 bot_despawn_id = *bot_despawn_iter;
            Creature* bot = const_cast<Creature*>(ASSERT_NOTNULL(FindBot(bot_despawn_id)));

            if (!bot->IsInWorld())
                break;

            _botsExtraCreaturesToDespawn.erase(bot_despawn_iter);

            const uint32 bot_orig_id = _botsExtraCreatureTemplates.at(bot_despawn_id).KillCredit[0];
            const std::string_view bot_name = bot->GetName();

            BotMgr::CleanupsBeforeBotDelete(bot);
            bot->GetBotAI()->canUpdate = false;
            if (bot->IsSummon())
                bot->ToTempSummon()->UnSummon();
            else
                bot->GetMap()->AddObjectToRemoveList(bot);

            const auto bditr = _botsData.find(bot_despawn_id);
            ASSERT(bditr != _botsData.cend());
            const bool is_owned = bditr->second.owner != 0;

            sBotGen->CleanExtraBotData(bot);

            BOT_LOG_DEBUG("npcbots", "Despawned {} bot {} '{}' (orig {})", is_owned ? "dungeon" : "wanderer", bot_despawn_id, bot_name, bot_orig_id);
        }
    }

    if (!_botsWanderCreaturesToSpawn.empty())
    {
        static const uint32 WANDERING_BOT_SPAWN_DELAY = 500;

        next_wandering_bot_spawn_delay += diff;

        while (next_wandering_bot_spawn_delay >= WANDERING_BOT_SPAWN_DELAY && !_botsWanderCreaturesToSpawn.empty())
        {
            next_wandering_bot_spawn_delay -= WANDERING_BOT_SPAWN_DELAY;

            auto const& p = _botsWanderCreaturesToSpawn.front();

            uint32 bot_id = p.first;
            WanderNode const* spawnLoc = p.second;

            _botsWanderCreaturesToSpawn.pop_front();

            SpawnWandererBot(bot_id, spawnLoc, nullptr);
        }

        return;
    }

    static uint32 replenishTimer = 0;
    replenishTimer += diff;
    if (replenishTimer >= 5 * IN_MILLISECONDS)
    {
        replenishTimer = 0;
        TryReplenishWanderingBots();
    }
}

std::shared_mutex* BotDataMgr::GetLock()
{
    static std::shared_mutex _lock;
    return &_lock;
}

bool BotDataMgr::AllBotsLoaded()
{
    return allBotsLoaded;
}

void BotDataMgr::LoadNpcBots(bool spawn)
{
    if (allBotsLoaded)
        return;

    BOT_LOG_INFO("server.loading", "Starting NpcBot system...");

    GenerateBotCustomSpells();

    uint32 botoldMSTime = getMSTime();

    Field* field;
    uint8 index;

    //                                                      1       2       3     4     5     6          7
    QueryResult result = WorldDatabase.Query("SELECT entry, `name*`, gender, skin, face, hair, haircolor, features FROM creature_template_npcbot_appearance");
    if (result)
    {
        do
        {
            field = result->Fetch();
            index = 0;
            uint32 entry = field[  index].GetUInt32();

            if (!sObjectMgr->GetCreatureTemplate(entry))
            {
                BOT_LOG_ERROR("server.loading", "Bot entry {} has appearance data but doesn't exist in `creature_template` table! Skipped.", entry);
                continue;
            }

            std::string bot_name = field[++index].GetString();
            uint8 bot_gender =    field[++index].GetUInt8();
            uint8 bot_skin =      field[++index].GetUInt8();
            uint8 bot_face =      field[++index].GetUInt8();
            uint8 bot_hair =      field[++index].GetUInt8();
            uint8 bot_haircolor = field[++index].GetUInt8();
            uint8 bot_features =  field[++index].GetUInt8();

            _botsAppearanceData.emplace(std::piecewise_construct, std::forward_as_tuple(entry),
                std::forward_as_tuple(std::move(bot_name), bot_gender, bot_skin, bot_face, bot_hair, bot_haircolor, bot_features));

        } while (result->NextRow());

        BOT_LOG_INFO("server.loading", ">> Bot appearance data loaded");
    }
    else
        BOT_LOG_INFO("server.loading", ">> Bots appearance data is not loaded. Table `creature_template_npcbot_appearance` is empty!");

    //                                          1      2
    result = WorldDatabase.Query("SELECT entry, class, race FROM creature_template_npcbot_extras");
    if (result)
    {
        do
        {
            field = result->Fetch();
            index = 0;
            uint32 entry =      field[  index].GetUInt32();

            if (!sObjectMgr->GetCreatureTemplate(entry))
            {
                BOT_LOG_ERROR("server.loading", "Bot entry {} has extras data but doesn't exist in `creature_template` table! Skipped.", entry);
                continue;
            }

            uint8 bot_class = field[++index].GetUInt8();
            uint8 bot_race =  field[++index].GetUInt8();

            _botsExtras.emplace(entry, NpcBotExtras{ .race = bot_race, .bclass = bot_class });

        } while (result->NextRow());

        BOT_LOG_INFO("server.loading", ">> Bot race data loaded");
    }
    else
        BOT_LOG_INFO("server.loading", ">> Bots race data is not loaded. Table `creature_template_npcbot_extras` is empty!");

    //                                              1     2        3
    result = CharacterDatabase.Query("SELECT entry, slot, item_id, fake_id FROM characters_npcbot_transmog");
    if (result)
    {
        do
        {
            field = result->Fetch();
            index = 0;
            uint32 entry =          field[  index].GetUInt32();

            if (!sObjectMgr->GetCreatureTemplate(entry))
            {
                BOT_LOG_ERROR("server.loading", "Bot entry {} has transmog data but doesn't exist in `creature_template` table! Skipped.", entry);
                continue;
            }

            _botsTransmogData.try_emplace(entry, NpcBotTransmogData{});

            //load data
            uint8 slot =            field[++index].GetUInt8();
            uint32 item_id =        field[++index].GetUInt32();
            int32 fake_id =         field[++index].GetInt32();

            _botsTransmogData[entry].transmogs.at(slot) = {item_id, fake_id};

        } while (result->NextRow());

        BOT_LOG_INFO("server.loading", ">> Bot transmog data loaded");
    }
    else
        BOT_LOG_INFO("server.loading", ">> Bots transmog data is not loaded. Table `characters_npcbot_transmog` is empty!");

    //                                       0      1      2      3     4        5                          6
    result = CharacterDatabase.Query("SELECT entry, owner, roles, spec, faction, UNIX_TIMESTAMP(hire_time), shared_owners, "
    //   7          8          9          10              11          12          13         14         15
        "equipMhEx, equipOhEx, equipRhEx, equipHead, equipShoulders, equipChest, equipWaist, equipLegs, equipFeet, "
    //   16          17          18         19         20            21            22             23             24
        "equipWrist, equipHands, equipBack, equipBody, equipFinger1, equipFinger2, equipTrinket1, equipTrinket2, equipNeck, "
    //   25               26
        "spells_disabled, miscvalues, hire_source FROM characters_npcbot");

    std::vector<uint32> entryList;
    if (result)
    {
        uint32 botcounter = 0;
        uint32 datacounter = 0;
        std::set<uint32> botgrids;
        QueryResult infores;
        CreatureTemplate const* proto;
        entryList.reserve(result->GetRowCount());

        do
        {
            field = result->Fetch();
            index = 0;
            uint32 entry =          field[  index].GetUInt32();

            if (!sObjectMgr->GetCreatureTemplate(entry))
            {
                BOT_LOG_ERROR("server.loading", "Bot entry {} doesn't exist in `creature_template` table! Skipped.", entry);
                continue;
            }

            //load data
            uint32 bot_owner =          field[++index].GetUInt32();
            uint32 bot_roles =          field[++index].GetUInt32();
            uint8  bot_spec =           field[++index].GetUInt8();
            uint32 bot_faction =        field[++index].GetUInt32();
            uint64 bot_hire_time =      field[++index].GetUInt64();

            entryList.push_back(entry);
            _botsData.emplace(std::piecewise_construct, std::forward_as_tuple(entry), std::forward_as_tuple(bot_owner, bot_hire_time, bot_roles, bot_faction, bot_spec));
            auto& bot_data = _botsData.at(entry);

            for (std::string_view shared_owner_sv : Bcore::Tokenize(field[++index].GetStringView(), ' ', false))
            {
                if (Optional<uint32> showner_guid = Bcore::StringTo<uint32>(shared_owner_sv))
                {
                    const ObjectGuid showner_pguid = ObjectGuid::Create<HighGuid::Player>(*showner_guid);
                    if (!sCharacterCache->HasCharacterCacheEntry(showner_pguid))
                    {
                        BOT_LOG_WARN("server.loading", "Bot entry {} has shared owner {} which doesn't exist! Skipped.", entry, *showner_guid);
                        continue;
                    }
                    bot_data.shared_owners.insert(*showner_guid);
                }
            }

            for (uint8 i = BOT_SLOT_MAINHAND; i != BOT_INVENTORY_SIZE; ++i)
                bot_data.equips[i] = field[++index].GetUInt32();

            if (char const* disabled_spells_str = field[++index].GetCString())
            {
                std::vector<std::string_view> tok = Bcore::Tokenize(disabled_spells_str, ' ', false);
                for (std::size_t i{}; i != tok.size(); ++i)
                    bot_data.disabled_spells.insert(*(Bcore::StringTo<uint32>(tok[i])));
            }

            if (char const* miscvalues_str = field[++index].GetCString())
            {
                std::vector<std::string_view> tok = Bcore::Tokenize(miscvalues_str, ' ', false);
                for (std::size_t i{}; i != tok.size(); ++i)
                {
                    std::vector<std::string_view> tok2 = Bcore::Tokenize(tok[i], ':', false);
                    bot_data.miscvalues.emplace(*(Bcore::StringTo<uint32>(tok2[0])), *(Bcore::StringTo<uint32>(tok2[1])));
                }
            }

            bot_data.hire_source = field[++index].GetUInt8();

            ++datacounter;

        } while (result->NextRow());

        BOT_LOG_INFO("server.loading", ">> Loaded {} bot data entries", datacounter);

        if (spawn)
        {
            for (uint32 entry : entryList)
            {
                proto = sObjectMgr->GetCreatureTemplate(entry);
                //                                     1     2    3           4           5           6
                infores = WorldDatabase.PQuery("SELECT guid, map, position_x, position_y, position_z, orientation FROM creature WHERE id = {}", entry);
                if (!infores)
                {
                    BOT_LOG_ERROR("server.loading", "Cannot spawn npcbot {} (id: {}), not found in `creature` table!", proto->Name, entry);
                    continue;
                }

                field = infores->Fetch();
                uint32 tableGuid = field[0].GetUInt32();
                uint32 mapId = uint32(field[1].GetUInt16());
                float pos_x = field[2].GetFloat();
                float pos_y = field[3].GetFloat();
                float pos_z = field[4].GetFloat();
                float ori = field[5].GetFloat();

                CellCoord c = Bcore::ComputeCellCoord(pos_x, pos_y);
                GridCoord g = Bcore::ComputeGridCoord(pos_x, pos_y);
                ASSERT(c.IsCoordValid(), "Invalid Cell coord!");
                ASSERT(g.IsCoordValid(), "Invalid Grid coord!");
                Map* map = sMapMgr->CreateBaseMap(mapId);
                Position spawnPos(pos_x, pos_y, pos_z, ori);
                Creature* bot = new Creature();
                if (!bot->LoadBotCreatureFromDB(tableGuid, map, false, false, entry, &spawnPos))
                {
                    delete bot;
                    BOT_LOG_FATAL("server.loading", "Cannot load npcbot {} from DB!", entry);
                    ABORT();
                }

                if (!bot->AIM_Initialize())
                {
                    delete bot;
                    BOT_LOG_FATAL("server.loading", "Cannot initialize npcbot {} AI!", entry);
                    ABORT();
                }

                if (!bot->IsAlive())
                {
                    BOT_LOG_WARN("server.loading", "bot {} is dead, respawning!", entry);
                    bot->setDeathState(JUST_RESPAWNED);
                }

                BOT_LOG_DEBUG("server.loading", ">> Spawned npcbot {} (id: {}, map: {}, grid: {}, cell: {})", proto->Name, entry, mapId, g.GetId(), c.GetId());
                botgrids.insert(g.GetId());
                ++botcounter;
            }

            BOT_LOG_INFO("server.loading", ">> Spawned {} npcbot(s) within {} grid(s) in {} ms", botcounter, uint32(botgrids.size()), GetMSTimeDiffToNow(botoldMSTime));
        }
    }
    else
        BOT_LOG_INFO("server.loading", ">> Loaded 0 npcbots. Table `characters_npcbot` is empty!");

    std::vector<uint32> invalid_ids;

    auto report_inavlid_ids = [&invalid_ids](std::string_view error_msg) {
        std::ostringstream ss;
        ss << error_msg << " IDs: ";
        for (uint32 bot_id : invalid_ids)
            ss << Bcore::ToString(bot_id) << ", ";
        ss << "\nFix your DB contents and retry";
        ABORT_MSG(ss.str().c_str());
    };

    for (auto const& [_, cdata] : sObjectMgr->GetAllCreatureData())
        if (cdata.id >= BOT_ENTRY_BEGIN && sObjectMgr->GetCreatureTemplate(cdata.id)->IsNPCBot() && std::ranges::find(entryList, cdata.id) == entryList.cend())
            invalid_ids.push_back(cdata.id);

    if (!invalid_ids.empty())
    {
        report_inavlid_ids("Invalid NPCBot spawns found in `creature` table having no data in `characters_npcbot` table!");
        invalid_ids.clear();
    }

    for (uint32 bot_id : entryList)
        if (!_botsExtras.contains(bot_id))
            invalid_ids.push_back(bot_id);

    if (!invalid_ids.empty())
    {
        report_inavlid_ids("Invalid NPCBots found in `characters_npcbot` table having no data in `creature_template_npcbot_extras` table!");
        invalid_ids.clear();
    }

    allBotsLoaded = true;
}

void BotDataMgr::LoadNpcBotGroupData()
{
    BOT_LOG_INFO("server.loading", "Loading NPCBot group members...");

    uint32 oldMSTime = getMSTime();

    CharacterDatabase.DirectExecute("DELETE FROM characters_npcbot_group_member WHERE guid NOT IN (SELECT guid FROM `groups`)");
    CharacterDatabase.DirectExecute("DELETE FROM characters_npcbot_group_member WHERE entry NOT IN (SELECT entry FROM characters_npcbot)");

    //                                                   0     1      2            3         4
    QueryResult result = CharacterDatabase.Query("SELECT guid, entry, memberFlags, subgroup, roles FROM characters_npcbot_group_member ORDER BY guid");
    if (!result)
    {
        BOT_LOG_INFO("server.loading", ">> Loaded 0 NPCBot group members. DB table `characters_npcbot_group_member` is empty!");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 creature_id = fields[1].GetUInt32();
        uint8 subgroup = fields[3].GetUInt8();
        if (!SelectNpcBotExtras(creature_id))
        {
            BOT_LOG_WARN("server.loading", "Table `characters_npcbot_group_member` contains non-NPCBot creature {} which will not be loaded!", creature_id);
            continue;
        }

        if (Group* group = sGroupMgr->GetGroupByDbStoreId(fields[0].GetUInt32()))
        {
            group->LoadCreatureMemberFromDB(creature_id, fields[2].GetUInt8(), subgroup, fields[4].GetUInt8());
            const_cast<Creature*>(ASSERT_NOTNULL(BotDataMgr::FindBot(creature_id)))->SetBotGroup(group, subgroup);
        }
        else
            BOT_LOG_ERROR("misc", "BotDataMgr::LoadNpcBotGroupData: Consistency failed, can't find group (storage id: {})", fields[0].GetUInt32());

        ++count;

    } while (result->NextRow());

    BOT_LOG_INFO("server.loading", ">> Loaded {} NPCBot group members in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void BotDataMgr::LoadNpcBotGearStorage()
{
    BOT_LOG_INFO("server.loading", "Loading NPCBot items storage...");

    uint32 oldMSTime = getMSTime();

    QueryResult result = CharacterDatabase.Query(
    //          0               1                   2         3            4           5         6                7                    8              9              10       11       12            13             14       15
        "SELECT ii.creatorGuid, ii.giftCreatorGuid, ii.count, ii.duration, ii.charges, ii.flags, ii.enchantments, ii.randomPropertyId, ii.durability, ii.playedTime, ii.text, ii.guid, ii.itemEntry, ii.owner_guid, gs.guid, gs.item_guid"
        " FROM  characters_npcbot_gear_storage gs JOIN item_instance ii ON gs.item_guid = ii.guid ORDER BY gs.guid, gs.item_guid");
    if (!result)
    {
        BOT_LOG_INFO("server.loading", ">> Loaded 0 NPCBot stored gear items. DB table `characters_npcbot_gear_storage` is empty!");
        return;
    }

    uint32 count = 0;
    std::set<uint32> player_guids;
    do
    {
        Field* fields = result->Fetch();

        uint32 item_id = fields[12].GetUInt32();
        uint32 player_guidlow = fields[14].GetUInt32();
        uint32 item_guidlow = fields[15].GetUInt32();

        Item* item = new Item();
        ObjectGuid player_guid = ObjectGuid::Create<HighGuid::Player>(player_guidlow);
        ASSERT(item->LoadFromDB(item_guidlow, player_guid, fields, item_id), "LoadNpcBotGearStorage(): unable to load item %u id %u! Owner: %s", item_guidlow, item_id, player_guid.ToString().c_str());

        _botStoredGearMap[player_guid].insert(item);
        player_guids.insert(player_guidlow);
        ++count;

    } while (result->NextRow());

    BOT_LOG_INFO("server.loading", ">> Loaded {} NPCBot stored items for {} bot owners in {} ms", count, uint32(player_guids.size()), GetMSTimeDiffToNow(oldMSTime));
}

void BotDataMgr::LoadNpcBotGearSets()
{
    BOT_LOG_INFO("server.loading", "Loading NPCBot item sets...");

    uint32 oldMSTime = getMSTime();

    auto make_set_guid = [](uint32 plow, uint8 set_id) { return MAKE_PAIR64(plow, set_id); };
    auto unpack_set_guid = [](uint64 set_guid) { return std::tuple(PAIR64_LOPART(set_guid), (uint8)PAIR64_HIPART(set_guid)); };

    //                                                   0      1       2
    QueryResult result = CharacterDatabase.Query("SELECT owner, set_id, set_name FROM characters_npcbot_gear_set");
    if (!result)
    {
        BOT_LOG_INFO("server.loading", ">> Loaded 0 NPCBot item sets. DB table `characters_npcbot_gear_set` is empty!");
        return;
    }

    std::set<uint32> player_guids;
    std::set<uint64> set_guids;
    do
    {
        Field* fields = result->Fetch();

        uint32 player_guidlow = fields[0].GetUInt32();
        uint8 set_id          = fields[1].GetUInt8();
        std::string set_name  = fields[2].GetString();

        ObjectGuid player_guid = ObjectGuid::Create<HighGuid::Player>(player_guidlow);

        UpdateBotItemSet(player_guid, set_id, std::move(set_name));

        player_guids.insert(player_guidlow);
        set_guids.insert(make_set_guid(player_guidlow, set_id));

    } while (result->NextRow());

    //                                       0      1       2     3
    result = CharacterDatabase.Query("SELECT owner, set_id, slot, item_id FROM characters_npcbot_gear_set_item ORDER BY owner,set_id,slot");

    std::set<uint64> invalid_sets;
    if (!result)
        invalid_sets = set_guids; //full copy
    else
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 player_guidlow = fields[0].GetUInt32();
            uint8 set_id          = fields[1].GetUInt8();
            uint8 slot            = fields[2].GetUInt8();
            uint32 item_id        = fields[3].GetUInt32();

            uint64 set_guid = make_set_guid(player_guidlow, set_id);

            if (!player_guids.contains(player_guidlow))
            {
                BOT_LOG_ERROR("server.loading", "Table `characters_npcbot_gear_set_item` contains values '{} {} {}' for non-existent player {}. Removing!",
                    uint32(set_id), uint32(slot), item_id, player_guidlow);
                invalid_sets.insert(set_guid);
                continue;
            }

            if (!set_guids.contains(set_guid))
            {
                BOT_LOG_ERROR("server.loading", "Table `characters_npcbot_gear_set_item` contains values '{} {}' for non-existent item set {} (player {}). Removing!",
                    uint32(slot), item_id, uint32(set_id), player_guidlow);
                invalid_sets.insert(set_guid);
                continue;
            }

            if (set_id >= MAX_BOT_EQUIPMENT_SETS)
            {
                BOT_LOG_ERROR("server.loading", "Table `characters_npcbot_gear_set_item` contains invalid set id {} (player {}). Removing!",
                    uint32(set_id), player_guidlow);
                invalid_sets.insert(set_guid);
                continue;
            }

            if (slot >= BOT_INVENTORY_SIZE)
            {
                BOT_LOG_ERROR("server.loading", "Table `characters_npcbot_gear_set_item` contains invalid slot {} for item set {} (player {}). Removing!",
                    uint32(slot), uint32(set_id), player_guidlow);
                invalid_sets.insert(set_guid);
                continue;
            }

            if (!sObjectMgr->GetItemTemplate(item_id))
            {
                BOT_LOG_ERROR("server.loading", "Table `characters_npcbot_gear_set_item` contains invalid item id {} in slot {} for item set {} (player {}). Removing!",
                    item_id, uint32(slot), uint32(set_id), player_guidlow);
                invalid_sets.insert(set_guid);
                continue;
            }

            ObjectGuid player_guid = ObjectGuid::Create<HighGuid::Player>(player_guidlow);

            UpdateBotItemSet(player_guid, set_id, slot, item_id);

        } while (result->NextRow());
    }

    if (!invalid_sets.empty())
    {
        CharacterDatabaseTransaction ctrans = CharacterDatabase.BeginTransaction();
        for (uint64 set_guid : invalid_sets)
        {
            set_guids.erase(set_guid);
            auto [player_guidlow, set_id] = unpack_set_guid(set_guid);
            ObjectGuid player_guid = ObjectGuid::Create<HighGuid::Player>(player_guidlow);
            _botStoredGearSetMap.at(player_guid).at(set_id).clear();
            ctrans->PAppend("DELETE FROM characters_npcbot_gear_set_item WHERE owner = {} and set_id = {}", player_guidlow, uint32(set_id));
        }

        std::set<ObjectGuid::LowType> invalid_players;
        for (auto const& [guid, itemsets] : _botStoredGearSetMap)
        {
            if (std::ranges::all_of(itemsets, [](NpcBotItemSet const& arr) { return arr.is_empty(); }))
            {
                invalid_players.insert(guid.GetCounter());
                ctrans->PAppend("DELETE FROM characters_npcbot_gear_set WHERE owner = {}", guid.GetCounter());
                ctrans->PAppend("DELETE FROM characters_npcbot_gear_set_item WHERE owner = {}", guid.GetCounter());
            }
        }
        CharacterDatabase.CommitTransaction(ctrans);

        for (ObjectGuid::LowType player_guidlow : invalid_players)
        {
            player_guids.erase(player_guidlow);
            _botStoredGearSetMap.erase(ObjectGuid::Create<HighGuid::Player>(player_guidlow));
        }
    }

    BOT_LOG_INFO("server.loading", ">> Loaded {} NPCBot item sets for {} bot owners in {} ms", uint32(set_guids.size()), uint32(player_guids.size()), GetMSTimeDiffToNow(oldMSTime));
}

void BotDataMgr::LoadNpcBotMgrData()
{
    BOT_LOG_INFO("server.loading", "Loading NPCBot managers data...");

    uint32 oldMSTime = getMSTime();

    //                                                   0      1            2            3                  4                  5                 6                  7
    QueryResult result = CharacterDatabase.Query("SELECT owner, dist_follow, dist_attack, attack_range_mode, attack_angle_mode, engage_delay_dps, engage_delay_heal, flags FROM characters_npcbot_settings");
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 idx = 0;
            ObjectGuid player_guid = ObjectGuid::Create<HighGuid::Player>(fields[  idx].GetUInt32());

            if (!sCharacterCache->HasCharacterCacheEntry(player_guid))
            {
                BOT_LOG_ERROR("server.loading", "Player {} found in table `characters_npcbot_settings` doesn't exist!", player_guid.GetCounter());
                BotDataMgr::RemoveNpcBotMgrDataFromDB(player_guid);
                continue;
            }

            uint8 dist_follow        = fields[++idx].GetUInt8();
            uint8 dist_attack        = fields[++idx].GetUInt8();
            uint8 attack_range_mode  = fields[++idx].GetUInt8();
            uint8 attack_angle_mode  = fields[++idx].GetUInt8();
            uint32 engage_delay_dps  = fields[++idx].GetUInt32();
            uint32 engage_delay_heal = fields[++idx].GetUInt32();
            uint32 flags             = fields[++idx].GetUInt32();

            if (dist_follow > 100)
            {
                BOT_LOG_WARN("server.loading", "Bot follow distance has invalid value {} > 100 for player {}, reduced!", uint32(dist_follow), player_guid.GetCounter());
                dist_follow = 100;
            }
            if (dist_attack > 50)
            {
                BOT_LOG_WARN("server.loading", "Bot attack distance has invalid value {} > 50 for player {}, reduced!", uint32(dist_attack), player_guid.GetCounter());
                dist_attack = 50;
            }
            if (attack_range_mode > BOT_ATTACK_RANGE_END)
            {
                BOT_LOG_WARN("server.loading", "Bot attack range mode has invalid value {} for player {}, reset to default!", uint32(attack_range_mode), player_guid.GetCounter());
                attack_range_mode = BOT_ATTACK_RANGE_SHORT;
            }
            if (attack_angle_mode > BOT_ATTACK_ANGLE_END)
            {
                BOT_LOG_WARN("server.loading", "Bot attack angle mode has invalid value {} for player {}, reset to default!", uint32(attack_angle_mode), player_guid.GetCounter());
                attack_angle_mode = BOT_ATTACK_ANGLE_NORMAL;
            }
            if (engage_delay_dps > 10 * IN_MILLISECONDS)
            {
                BOT_LOG_WARN("server.loading", "Bot dps engage timer has invalid value {} for player {}, reduced!", engage_delay_dps, player_guid.GetCounter());
                engage_delay_dps = BotCfg::GetEngageDelayDPSDefault();
            }
            if (engage_delay_heal > 10 * IN_MILLISECONDS)
            {
                BOT_LOG_WARN("server.loading", "Bot heal engage timer has invalid value {} for player {}, reduced!", engage_delay_heal, player_guid.GetCounter());
                engage_delay_heal = BotCfg::GetEngageDelayHealDefault();
            }
            if (flags & ~NPCBOT_MGR_FLAG_MASK_ALL_ALLOWED)
            {
                BOT_LOG_WARN("server.loading", "Bot manager flags have invalid value {} for player {}, removing invalid flags!", flags, player_guid.GetCounter());
                flags &= NPCBOT_MGR_FLAG_MASK_ALL_ALLOWED;
            }

            _botMgrsData.emplace(std::piecewise_construct, std::forward_as_tuple(player_guid), std::forward_as_tuple(dist_follow, dist_attack, attack_range_mode, attack_angle_mode, engage_delay_dps, engage_delay_heal, flags));

        } while (result->NextRow());

        BOT_LOG_INFO("server.loading", ">> Loaded NPCBot manager data for {} bot owners in {} ms", uint32(_botMgrsData.size()), GetMSTimeDiffToNow(oldMSTime));
    }
    else
        BOT_LOG_INFO("server.loading", ">> Bot managers data is not loaded. Table `characters_npcbot_settings` is empty!");
}

void BotDataMgr::DeleteOldLogs()
{
    uint32 month_cutoff = static_cast<uint32>(GameTime::GetGameTime() - static_cast<time_t>(BOT_LOG_KEEP_DAYS) * DAY);
    CharacterDatabase.PExecute("DELETE FROM `characters_npcbot_logs` WHERE timestamp IS NOT NULL AND timestamp < FROM_UNIXTIME({})", month_cutoff);
    BOT_LOG_INFO("server.loading", "Deleting NPCBot log entries older than {} days...", BOT_LOG_KEEP_DAYS);
}

void BotDataMgr::LoadWanderMap(bool reload, bool force_all_maps)
{
    using WanderNodeLink = WanderNode::WanderNodeLink;
    using SpawnMapEx = std::map<uint32, bool>;
    using SpawnVector = std::vector<WanderNode const*>;

    const std::array<uint32, 4> ALL_CONTINENT_MAPS = { 0u, 1u, 530u, 571u };

    if (WanderNode::GetAllWPsCount() > 0u)
    {
        if (!reload)
            return;

        WanderNode::RemoveAllWPs();
    }

    _wpMinSpawnLevelPerMapId.clear();
    _wpMaxSpawnLevelPerMapId.clear();

    uint32 botoldMSTime = getMSTime();

    BOT_LOG_INFO("server.loading", "Setting up wander map...");

    //                                             0  1     2 3 4 5 6      7      8        9        10    11   12
    QueryResult wres = WorldDatabase.Query("SELECT id,mapid,x,y,z,o,zoneId,areaId,minlevel,maxlevel,flags,name,links,minwaittime,maxwaittime,proximity FROM creature_template_npcbot_wander_nodes ORDER BY mapid,id");
    if (!wres)
    {
        BOT_WP_LOG_FATAL("server.loading", "Failed to load wander points: table `creature_template_npcbot_wander_nodes` is empty!");
        ASSERT(false);
    }

    const uint32 maxof_minclasslvl_nr = GetMinLevelForBotClass(BOT_CLASS_DEATH_KNIGHT); // 55
    const uint32 maxof_minclasslvl_ex = GetMinLevelForBotClass(BOT_CLASS_DREADLORD); // 60

    std::unordered_map<uint32, std::pair<WanderNode*, std::vector<std::pair<std::string, std::string>>>> links_to_create;
    std::array<SpawnMapEx, 3> SpawnMapsEx{};
    SpawnVector all_spawn_nodes;
    all_spawn_nodes.reserve(wres->GetRowCount() >> 8);
    for (SpawnMapEx& smap : SpawnMapsEx)
        for (uint32 mapId : ALL_CONTINENT_MAPS)
            if (BotCfg::IsBotGenerationEnabledWorldMapId(mapId))
                smap.emplace(mapId, false);

    uint32 disabled_nodes = 0;
    do
    {
        Field* fields = wres->Fetch();
        uint32 index = 0;

        uint32 id             = fields[  index].GetUInt32();
        uint32 mapId          = fields[++index].GetUInt16();
        float x               = fields[++index].GetFloat();
        float y               = fields[++index].GetFloat();
        float z               = fields[++index].GetFloat();
        float o               = fields[++index].GetFloat();
        uint32 zoneId         = fields[++index].GetUInt32();
        uint32 areaId         = fields[++index].GetUInt32();
        uint8 minLevel        = fields[++index].GetUInt8();
        uint8 maxLevel        = fields[++index].GetUInt8();
        EnumFlag<BotWPFlags> flags = static_cast<BotWPFlags>(fields[++index].GetUInt32());
        std::string name      = fields[++index].GetString();
        std::string_view lstr = fields[++index].GetStringView();
        uint32 minwaittime    = fields[++index].GetUInt32();
        uint32 maxwaittime    = fields[++index].GetUInt32();
        float proximity       = fields[++index].GetFloat();

        WanderNode::nextWPId = std::max<uint32>(WanderNode::nextWPId, id);

        MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
        if (!mapEntry)
        {
            BOT_WP_LOG_ERROR("server.loading", "WP {} has invalid map id {}!", id, mapId);
            continue;
        }

        if (minLevel == 1u && maxLevel == DEFAULT_MAX_LEVEL)
            BOT_WP_LOG_WARN("server.loading", "WP {} has no levels set.", id);

        if (!minLevel || !maxLevel || minLevel > DEFAULT_MAX_LEVEL || maxLevel > DEFAULT_MAX_LEVEL || minLevel > maxLevel)
        {
            BOT_WP_LOG_WARN("server.loading", "WP {} has invalid levels min {} max {}! Setting to default...",
                id, uint32(minLevel), uint32(maxLevel));
            minLevel = 1;
            maxLevel = DEFAULT_MAX_LEVEL;
        }

        if (flags >= BotWPFlags::BOTWP_FLAG_END)
        {
            BOT_WP_LOG_WARN("server.loading", "WP {} has invalid flags {}! Removing all invalid flags...", id, flags.AsUnderlyingType());
            flags &= BotWPFlags::BOTWP_FLAGS_ALL_VALID;
        }

        const auto nonbg_flags = BotWPFlags::BOTWP_FLAG_BG_FLAG_PICKUP_TARGET | BotWPFlags::BOTWP_FLAG_BG_FLAG_DELIVER_TARGET;
        if (flags.HasFlag(nonbg_flags) && !mapEntry->IsBattleground())
        {
            BOT_WP_LOG_WARN("server.loading", "WP {} has BG-only flags {} for non-BG map {}! Removing...", id, (flags & nonbg_flags).AsUnderlyingType(), mapEntry->ID);
            flags &= ~nonbg_flags;
        }

        const std::array conflicting_flags{
            std::pair{BotWPFlags::BOTWP_FLAG_ALLIANCE_ONLY, BotWPFlags::BOTWP_FLAG_HORDE_ONLY },
            std::pair{BotWPFlags::BOTWP_FLAG_CAN_BACKTRACK_FROM, BotWPFlags::BOTWP_FLAG_MOVEMENT_FORCE_JUMP_END },
            std::pair{BotWPFlags::BOTWP_FLAG_BG_MISC_OBJECTIVE_1, BotWPFlags::BOTWP_FLAG_BG_MISC_OBJECTIVE_2 },
            std::pair{BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_1, BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_2 },
            std::pair{BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_1, BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_3 },
            std::pair{BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_1, BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_4 },
            std::pair{BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_1, BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_5 },
            std::pair{BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_2, BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_3 },
            std::pair{BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_2, BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_4 },
            std::pair{BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_2, BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_5 },
            std::pair{BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_3, BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_4 },
            std::pair{BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_3, BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_5 },
            std::pair{BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_4, BotWPFlags::BOTWP_FLAG_BG_OPTIONAL_PICKUP_5 },
        };
        for (std::pair<BotWPFlags, BotWPFlags> const& p : conflicting_flags)
        {
            const BotWPFlags cflags = p.first | p.second;
            if ((flags & cflags) == cflags)
            {
                BOT_WP_LOG_WARN("server.loading", "WP {} has conflicting flags {}+{}! Removing both...", id, AsUnderlyingType(p.first), AsUnderlyingType(p.second));
                flags &= ~cflags;
            }
        }

        if (!force_all_maps && mapEntry->IsContinent() && !BotCfg::IsBotGenerationEnabledWorldMapId(mapId))
        {
            ++disabled_nodes;
            continue;
        }

        WanderNode* wp = new WanderNode(id, mapId, x, y, z, o, zoneId, areaId, std::move(name));
        wp->SetLevels(minLevel, maxLevel);
        wp->SetFlags(BotWPFlags(flags));
        wp->SetWaitTime(minwaittime, maxwaittime);
        wp->SetProximity(proximity);

        if (wp->HasFlag(BotWPFlags::BOTWP_FLAG_SPAWN) && !lstr.empty())
        {
            all_spawn_nodes.push_back(wp);

            if (!wp->HasFlag(BotWPFlags::BOTWP_FLAG_ALLIANCE_OR_HORDE_ONLY) && wp->GetLevels().second <= 10)
                BOT_WP_LOG_WARN("server.loading", "WP {} is a start location but has no HORDE or ALLIANCE flag assigned! Only Neutral bots will spawn there!", id);
        }

        if (lstr.empty())
        {
            BOT_WP_LOG_ERROR("server.loading", "WP {} has no links!", id);
            continue;
        }
        std::vector<std::string_view> tok = Bcore::Tokenize(lstr, ' ', false);
        for (std::vector<std::string_view>::size_type i = 0; i != tok.size(); ++i)
        {
            std::vector<std::string_view> link_str = Bcore::Tokenize(tok[i], ':', false);
            ASSERT(link_str.size() == 2u, "Invalid links_str format: '%s'", std::string(tok[i].data(), tok[i].length()).c_str());
            ASSERT(link_str[0].find(" ") == std::string_view::npos);
            ASSERT(link_str[1].find(" ") == std::string_view::npos);
            ASSERT(Bcore::StringTo<uint32>(link_str[0]) != std::nullopt, "Invalid links_str format: '%s'", std::string(tok[i].data(), tok[i].length()).c_str());
            ASSERT(Bcore::StringTo<uint32>(link_str[1]) != std::nullopt, "Invalid links_str format: '%s'", std::string(tok[i].data(), tok[i].length()).c_str());

            std::pair<std::string, std::string> tok_pair = { std::string(link_str[0].data(), link_str[0].length()), std::string(link_str[1].data(), link_str[1].length()) };
            auto lit = links_to_create.find(id);
            if (lit == links_to_create.cend())
                links_to_create[id] = { wp, {std::move(tok_pair)} };
            else
                lit->second.second.push_back(std::move(tok_pair));
        }

    } while (wres->NextRow());

    auto& [spawn_node_exists_a, spawn_node_exists_h, spawn_node_exists_n] = SpawnMapsEx;
    for (WanderNode const* wp : all_spawn_nodes)
    {
        uint32 mapId = wp->GetMapId();
        auto [minLevel, maxLevel] = wp->GetLevels();

        spawn_node_exists_a[mapId] |= (maxLevel >= maxof_minclasslvl_nr && bot_ai::IsWanderNodeAvailableForBotFaction(wp, FACTION_TEMPLATE_ALLIANCE_DEFAULT, false, true));
        spawn_node_exists_h[mapId] |= (maxLevel >= maxof_minclasslvl_nr && bot_ai::IsWanderNodeAvailableForBotFaction(wp, FACTION_TEMPLATE_HORDE_DEFAULT, false, true));
        spawn_node_exists_n[mapId] |= (maxLevel >= maxof_minclasslvl_ex && bot_ai::IsWanderNodeAvailableForBotFaction(wp, FACTION_TEMPLATE_NEUTRAL_HOSTILE, false, true));

        decltype(_wpMinSpawnLevelPerMapId)::const_iterator mincit = _wpMinSpawnLevelPerMapId.find(mapId);
        _wpMinSpawnLevelPerMapId[mapId] = std::min<uint8>((mincit != _wpMinSpawnLevelPerMapId.cend()) ? mincit->second : uint8(DEFAULT_MAX_LEVEL), minLevel);
        decltype(_wpMaxSpawnLevelPerMapId)::const_iterator maxcit = _wpMaxSpawnLevelPerMapId.find(mapId);
        _wpMaxSpawnLevelPerMapId[mapId] = std::max<uint8>((maxcit != _wpMaxSpawnLevelPerMapId.cend()) ? maxcit->second : 1u, maxLevel);
    }

    bool spawn_node_minclasslvl_exists_all = true;
    for (auto [map_id, exists] : spawn_node_exists_a) //copying 8 bytes
    {
        if (!exists)
        {
            BOT_WP_LOG_FATAL("server.loading", "No valid Alliance spawn node for at least level {} on map {}! Spawning wandering bots is impossible! Aborting.",
                maxof_minclasslvl_nr, map_id);
            spawn_node_minclasslvl_exists_all = false;
        }
    }
    for (auto [map_id, exists] : spawn_node_exists_h) //copying 8 bytes
    {
        if (!exists)
        {
            BOT_LOG_FATAL("server.loading", "No valid Horde spawn node for at least level {} on map {}! Spawning wandering bots is impossible! Aborting.",
                maxof_minclasslvl_nr, map_id);
            spawn_node_minclasslvl_exists_all = false;
        }
    }
    for (auto [map_id, exists] : spawn_node_exists_n) //copying 8 bytes
    {
        if (!exists)
        {
            if (sMapStore.LookupEntry(map_id)->IsBattlegroundOrArena())
                BOT_LOG_INFO("server.loading", "No valid Neutral spawn node for at least level {} on non-continent map {}.", maxof_minclasslvl_ex, map_id);
            else
            {
                BOT_WP_LOG_FATAL("server.loading", "No valid Neutral spawn node for at least level {} on map {}! Spawning wandering bots is impossible! Aborting.",
                    maxof_minclasslvl_ex, map_id);
                spawn_node_minclasslvl_exists_all = false;
            }
        }
    }
    if (!spawn_node_minclasslvl_exists_all)
        ABORT();

    const uint8 TEAMS_COUNT = TEAM_NEUTRAL + 1;
    std::array team_strs{ "Alliance"sv, "Horde"sv, "Neutral"sv };
    std::array<bool, DEFAULT_MAX_LEVEL> spawn_node_levels[TEAMS_COUNT]{ { false } };
    uint8 min_spawn_level = DEFAULT_MAX_LEVEL;
    uint8 max_spawn_level = 0;
    for (WanderNode const* wp : all_spawn_nodes)
    {
        if (sMapStore.LookupEntry(wp->GetMapId())->IsContinent() && BotCfg::IsBotGenerationEnabledWorldMapId(wp->GetMapId()))
        {
            auto [minLevel, maxLevel] = wp->GetLevels();
            min_spawn_level = std::min<uint32>(min_spawn_level, minLevel);
            max_spawn_level = std::max<uint32>(max_spawn_level, maxLevel);
        }
    }
    for (WanderNode const* wp : all_spawn_nodes)
    {
        if (sMapStore.LookupEntry(wp->GetMapId())->IsContinent())
        {
            auto [minLevel, maxLevel] = wp->GetLevels();
            minLevel = std::max<uint8>(minLevel, 1);
            maxLevel = std::min<uint8>(maxLevel, max_spawn_level);
            for (uint8 k = 0; k < TEAMS_COUNT; ++k)
            {
                if ((k == 0 && bot_ai::IsWanderNodeAvailableForBotFaction(wp, FACTION_TEMPLATE_ALLIANCE_DEFAULT, false, true)) ||
                    (k == 1 && bot_ai::IsWanderNodeAvailableForBotFaction(wp, FACTION_TEMPLATE_HORDE_DEFAULT, false, true)) ||
                    (k == 2 && bot_ai::IsWanderNodeAvailableForBotFaction(wp, FACTION_TEMPLATE_NEUTRAL_HOSTILE, false, true)))
                {
                    for (size_t i = minLevel; i <= maxLevel; ++i)
                        spawn_node_levels[k][i - 1] = true;
                }
            }
        }
    }
    for (uint8 k = 0; k < TEAMS_COUNT; ++k)
    {
        auto const& vec = spawn_node_levels[k];
        for (size_t i = min_spawn_level; i <= max_spawn_level; ++i)
        {
            if (vec[i - 1] == false)
                BOT_LOG_ERROR("server.loading", "No {} spawn node found for level {}! Wandering bots may cause a crash!", team_strs[k], i);
        }
    }

    float mindist = 50000.f;
    float maxdist = 0.f;
    for (auto const& vt : links_to_create)
    {
        for (auto const& p : vt.second.second)
        {
            uint32 lid = *Bcore::StringTo<uint32>(p.first);
            uint32 lweight = *Bcore::StringTo<uint32>(p.second);

            if (lweight >= 1000)
                BOT_WP_LOG_WARN("server.loading", "WP {} has link {} with suspicious weight of {}, error?", vt.first, lid, lweight);

            if (lid == vt.first)
            {
                BOT_WP_LOG_ERROR("server.loading", "WP {} has link {} which links to itself! Skipped.", vt.first, lid);
                continue;
            }

            WanderNode* lwp = WanderNode::FindInAllWPs(lid);
            if (!lwp)
            {
                BOT_WP_LOG_ERROR("server.loading", "WP {} has link {} which does not exist!", vt.first, lid);
                continue;
            }
            if (lwp->GetMapId() != vt.second.first->GetMapId())
            {
                BOT_WP_LOG_ERROR("server.loading", "WP {} map {} has link {} ON A DIFFERENT MAP {}!", vt.first, vt.second.first->GetMapId(), lid, lwp->GetMapId());
                continue;
            }

            bool is_continent = sMapStore.LookupEntry(vt.second.first->GetMapId())->IsContinent();
            float lwpdist2d = vt.second.first->GetExactDist2d(lwp);
            if (lwpdist2d > MAX_WANDER_NODE_DISTANCE)
                BOT_WP_LOG_WARN("server.loading", "Warning! Link distance between WP {} and {} is too great ({})", vt.first, lid, lwpdist2d);
            if (lwpdist2d < MIN_WANDER_NODE_DISTANCE && is_continent)
                BOT_WP_LOG_WARN("server.loading", "Warning! Link distance between WP {} and {} is low ({})", vt.first, lid, lwpdist2d);

            WanderNodeLink newlink{ .wp = lwp, .weight = lweight };
            vt.second.first->Link(std::move(newlink));

            if (is_continent)
            {
                float dist2d = vt.second.first->GetExactDist2d(lwp);
                if (dist2d < mindist)
                    mindist = dist2d;
                if (dist2d > maxdist)
                    maxdist = dist2d;
            }
        }

        if (uint32 avg_weight = vt.second.first->GetAverageLinkWeight())
        {
            for (WanderNodeLink const& wpl : vt.second.first->GetLinks())
            {
                if (wpl.weight == 0)
                    BOT_WP_LOG_WARN("server.loading", "WP {} has link {} with weight of 0 (average {})! Link will be inaccessible!", vt.first, wpl.Id(), avg_weight);
                else if (float(wpl.weight) < avg_weight / 100.f)
                    BOT_WP_LOG_WARN("server.loading", "WP {} has link {} with weight of {} below 1% average ({}), error?", vt.first, wpl.Id(), wpl.weight, avg_weight);
            }
        }
    }

    std::set<WanderNode const*> tops;
    WanderNode::DoForAllWPs([&](WanderNode const* wp) {
        auto const& wplinks = wp->GetLinks();
        if (!tops.contains(wp) && wplinks.size() == 1u)
        {
            BOT_LOG_DEBUG("server.loading", "Node {} ('{}') has single connection!", wp->GetWPId(), wp->GetName());
            WanderNode const* tn = wplinks.begin()->wp;
            WanderNode const* prev = nullptr;
            std::vector<WanderNode const*> sc_chain;
            sc_chain.push_back(wp);
            tops.emplace(wp);
            while (tn != wp)
            {
                auto const& tnlinks = tn->GetLinks();
                if (tnlinks.size() != 2u || !tn->HasLink(prev ? prev : wp))
                {
                    sc_chain.push_back(tn);
                    break;
                }
                prev = sc_chain.back();
                sc_chain.push_back(tn);
                tn = std::ranges::find_if_not(tnlinks, [=](std::remove_cvref_t<decltype(tnlinks)>::value_type const& lwp) { return lwp.wp == prev; })->wp;
            }
            if (sc_chain.back()->GetLinks().size() == 1u && prev && sc_chain.back()->GetLinks().front().wp == prev)
            {
                BOT_LOG_DEBUG("server.loading", "Node {} ('{}') has single connection!", tn->GetWPId(), tn->GetName());
                tops.emplace(sc_chain.back());
                std::ostringstream ss;
                ss << "Node " << (sc_chain.size() == 2u ? "pair " : "chain ");
                for (std::size_t i{}; i < sc_chain.size(); ++i)
                {
                    ss << sc_chain[i]->GetWPId();
                    if (i < sc_chain.size() - 1u)
                        ss << '-';
                }
                ss << " is isolated!";
                BOT_LOG_INFO("server.loading", "{}", ss.str());
            }
        }
    });

    BOT_LOG_INFO("server.loading", ">> Loaded {} bot wander nodes ({} disabled) on {} maps (total {} tops) in {} ms",
        uint32(WanderNode::GetAllWPsCount()), disabled_nodes, uint32(WanderNode::GetWPMapsCount()), uint32(tops.size()), GetMSTimeDiffToNow(botoldMSTime));
}

void BotDataMgr::GenerateWanderingBots()
{
    uint32 wandering_bots_desired = BotCfg::GetDesiredWanderingBotsCount();

    if (wandering_bots_desired == 0)
        return;

    BOT_LOG_INFO("server.loading", "Spawning wandering bots...");

    uint32 oldMSTime = getMSTime();

    uint32 const maxbots = sBotGen->GetSpareBotsCount();
    uint32 const enabledbots = sBotGen->GetEnabledBotsCount();

    // By leewheel 20260529 - do not ASSERT on misconfiguration; spawn as many as possible so worldserver can start.
    if (maxbots < wandering_bots_desired)
    {
        BOT_LOG_ERROR("server.loading",
            "Only {} out of {} enabled-class bot templates are available for wandering spawn (desired {}). Spawning up to {} instead.",
            maxbots, enabledbots, wandering_bots_desired, maxbots);
        wandering_bots_desired = maxbots;
    }

    if (!wandering_bots_desired)
    {
        BOT_LOG_ERROR("server.loading", "Cannot spawn wandering bots: no spare bot templates. Set NpcBot.WanderingBots.Continents.Count to 0 or add npcbot creature templates.");
        return;
    }

    uint32 spawned_count = 0;
    if (!sBotGen->GenerateWanderingBotsToSpawn(wandering_bots_desired, -1, -1, false, nullptr, nullptr, spawned_count))
        BOT_LOG_ERROR("server.loading", "Failed to queue all {} wandering bots ({} queued). Check wander nodes / maps.", wandering_bots_desired, spawned_count);

    if (!spawned_count)
    {
        BOT_LOG_ERROR("server.loading", "No wandering bots were queued. Verify `creature_template_npcbot_wander_nodes` and NpcBot.WanderingBots.Continents.* settings.");
        return;
    }

    BOT_LOG_INFO("server.loading", ">> Set up spawning of {} wandering bots in {} ms", spawned_count, GetMSTimeDiffToNow(oldMSTime));
}

void BotDataMgr::GenerateDungeonBots(Player const* leader, Group const* group, Map const* map)
{
    using enum lfg::LfgRoles;
    using roles_arr = std::array<uint8, 3>;

    if (!BotCfg::IsNpcBotModEnabled() || !BotCfg::IsNpcBotDungeonFinderBotGenerationEnabled())
        return;

    if (!group->isLFGGroup())
        return;

    // Group::GetMembersCount() already includes NPCBots added via AddMember(Creature).
    // Do not also iterate GetFirstBotMember() — that double-counts and breaks fill math.
    uint32 party_slots_used = group->GetMembersCount();

    // LFG fillers may exist on a member's BotMgr before group membership is linked.
    for (GroupReference const* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player const* member = itr->GetSource();
        if (!member || !member->HaveBot())
            continue;

        for (auto const& [guid, bot] : *member->GetBotMgr()->GetBotMap())
        {
            if (!bot || !bot->IsSummon() || group->IsMember(guid))
                continue;
            if (IsServiceHireSource(GetNpcBotHireSource(bot->GetEntry())))
                ++party_slots_used;
        }
    }

    if (party_slots_used >= MAX_GROUP_SIZE)
        return;

    const uint32 bots_to_generate_count = static_cast<uint32>(MAX_GROUP_SIZE) - party_slots_used;
    if (!bots_to_generate_count)
        return;

    BOT_LOG_INFO("npcbots", "DungeonBots: player: {} ({}), map '{}', party slots: {}, bots to generate: {}",
        leader->GetName(), leader->GetGUID().GetCounter(), map->GetId(), party_slots_used, bots_to_generate_count);

    roles_arr existing_roles{};
    auto collect_existing_roles = [&existing_roles](ObjectGuid guid) {
        const uint8 roles = sLFGMgr->GetRoles(guid);
        if ((roles & PLAYER_ROLE_TANK) && existing_roles[0] < 1)
            existing_roles[0]++;
        else if ((roles & PLAYER_ROLE_HEALER) && existing_roles[1] < 1)
            existing_roles[1]++;
        else if (existing_roles[2] < 3)
            existing_roles[2]++;
    };
    for (GroupReference const* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        if (Player const* gplayer = itr->GetSource())
            collect_existing_roles(gplayer->GetGUID());
    for (GroupBotReference const* itr = group->GetFirstBotMember(); itr != nullptr; itr = itr->next())
        if (Creature const* gbot = itr->GetSource())
            collect_existing_roles(gbot->GetGUID());

    BOT_LOG_INFO("npcbots", "DungeonBots: existing roles: [{}, {}, {}]", uint32(existing_roles[0]), uint32(existing_roles[1]), uint32(existing_roles[2]));

    roles_arr desired_roles{ 1, 1, 3 };
    for (auto i : NPCBots::index_array<std::size_t, std::size(existing_roles)>)
        desired_roles[i] -= existing_roles[i];

    BOT_LOG_INFO("npcbots", "DungeonBots: desired roles: [{}, {}, {}]", uint32(desired_roles[0]), uint32(desired_roles[1]), uint32(desired_roles[2]));

    if (!sBotGen->GenerateDungeonBotsToSpawn(leader, bots_to_generate_count, std::move(desired_roles)))
        BOT_LOG_WARN("npcbots", "DungeonBots: Failed to spawn {} bots", bots_to_generate_count);
}

bool BotDataMgr::GenerateBattlegroundBots(Player const* groupLeader, [[maybe_unused]] Group const* group, BattlegroundQueue* queue, PvPDifficultyEntry const* bracketEntry, GroupQueueInfo const* gqinfo)
{
    if (!BotCfg::IsBotGenerationEnabledBGs())
        return true;

    BattlegroundQueueTypeId bgqTypeId = queue->GetQueueId();
    BattlegroundTypeId bgTypeId = BattlegroundTypeId(bgqTypeId.BattlemasterListId);
    uint32 ammr = gqinfo->ArenaMatchmakerRating;
    BattlegroundBracketId bracketId = bracketEntry->GetBracketId();

    uint32 tarteamplayers = BotCfg::GetBGTargetTeamPlayersCount(bgTypeId);

    if (tarteamplayers == 0)
    {
        BOT_LOG_INFO("npcbots", "[Disabled] BG {} wandering bots generation is disabled (not implemented?)", uint32(bgTypeId));
        return true;
    }

    //find running BG
    auto const& all_bgs = sBattlegroundMgr->GetBgDataStore();
    for (auto const& [bg_type_id, bg_data] : all_bgs)
    {
        if (bg_type_id == bgTypeId)
        {
            for (auto const& [_, bg_ptr] : bg_data.m_Battlegrounds)
            {
                Battleground const* real_bg = bg_ptr.get();
                if (real_bg->GetInstanceID() != 0 && real_bg->GetBracketId() == bracketId && real_bg->GetStatus() < STATUS_WAIT_LEAVE && real_bg->HasFreeSlots())
                {
                    if (real_bg->GetFreeSlotsForTeam(groupLeader->GetTeam()) < gqinfo->Players.size())
                    {
                        BOT_LOG_INFO("npcbots", "[Already running 1] Found running non-full BG {} instance {}. Not generating bots: queuing group or player (leader {}) CANNOT join existing BG, prevent borrowing bots",
                            uint32(bgTypeId), real_bg->GetInstanceID(), groupLeader->GetGUID().GetCounter());
                    }
                    else
                    {
                        BOT_LOG_INFO("npcbots", "[Already running 2] Found running non-full BG {} instance {}. Not generating bots: queuing group or player (leader {}) CAN join existing BG",
                            uint32(bgTypeId), real_bg->GetInstanceID(), groupLeader->GetGUID().GetCounter());
                    }
                    return true;
                }
            }
        }
    }

    Battleground const* bg_template = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);

    if (!bg_template)
        return false;

    uint32 minteamplayers = bg_template->GetMinPlayersPerTeam();
    uint32 maxteamplayers = bg_template->GetMaxPlayersPerTeam();

    uint32 normalCount = tarteamplayers;
    RoundToInterval(normalCount, minteamplayers, maxteamplayers);
    if (tarteamplayers != normalCount)
    {
        BOT_LOG_ERROR("npcbots", "NpcBot.WanderingBots.BG.TargetTeamPlayersCount value {} for BG {} '{}' is out of bounds ({}-{})! Normalized to {}!",
            tarteamplayers, uint32(bgTypeId), bg_template->GetName(), minteamplayers, maxteamplayers, normalCount);
        tarteamplayers = normalCount;
    }

    uint32 queued_players_a = 0;
    uint32 queued_players_h = 0;
    for (uint8 i = 0; i < BG_QUEUE_GROUP_TYPES_COUNT; ++i)
    {
        for (GroupQueueInfo const* qgr : queue->m_QueuedGroups[i])
        {
            if (qgr->Team == ALLIANCE)
                queued_players_a += qgr->Players.size();
            else
                queued_players_h += qgr->Players.size();
        }
    }

    uint32 needed_bots_count_a = (queued_players_a < tarteamplayers) ? (tarteamplayers - queued_players_a) : 0;
    uint32 needed_bots_count_h = (queued_players_h < tarteamplayers) ? (tarteamplayers - queued_players_h) : 0;

    ASSERT(needed_bots_count_a <= maxteamplayers);
    ASSERT(needed_bots_count_h <= maxteamplayers);

    if (needed_bots_count_a + needed_bots_count_h == 0)
    {
        BOT_LOG_INFO("npcbots", "[No bots required] Failed to generate bots for BG {} inited by player {} ({})",
            uint32(bgTypeId), groupLeader->GetName(), groupLeader->GetGUID().GetCounter());
        return true;
    }

    uint32 spare_bots_a = sBotGen->GetSpareBotsCount(TEAM_ALLIANCE);
    uint32 spare_bots_h = sBotGen->GetSpareBotsCount(TEAM_HORDE);

    if (queued_players_a + spare_bots_a < minteamplayers)
    {
        BOT_LOG_INFO("npcbots", "[Not enough A bots] Failed to generate bots for BG {} inited by player {} ({})",
            uint32(bgTypeId), groupLeader->GetName(), groupLeader->GetGUID().GetCounter());
        return false;
    }
    if (queued_players_h + spare_bots_h < minteamplayers)
    {
        BOT_LOG_INFO("npcbots", "[Not enough H bots] Failed to generate bots for BG {} inited by player {} ({})",
            uint32(bgTypeId), groupLeader->GetName(), groupLeader->GetGUID().GetCounter());
        return false;
    }

    needed_bots_count_a = std::min<uint32>(needed_bots_count_a, spare_bots_a);
    needed_bots_count_h = std::min<uint32>(needed_bots_count_h, spare_bots_h);

    uint32 spawned_a = 0;
    uint32 spawned_h = 0;
    std::array<NpcBotRegistry, 2> spawned_bots;
    auto& [spawned_bots_a, spawned_bots_h] = spawned_bots;

    if (needed_bots_count_a)
    {
        if (!sBotGen->GenerateWanderingBotsToSpawn(needed_bots_count_a, bg_template->GetMapId(), ALLIANCE, true, bracketEntry, &spawned_bots_a, spawned_a))
        {
            BOT_LOG_WARN("npcbots", "Failed to spawn {} ALLIANCE bots for BG {} '{}' queued A {} H {} req A {} H {} spare {}",
                needed_bots_count_a, uint32(bg_template->GetTypeID()), bg_template->GetName(),
                queued_players_a, queued_players_h, needed_bots_count_a, needed_bots_count_h, spare_bots_a);
            for (NpcBotRegistry const& registry1 : spawned_bots)
                for (Creature const* bot : registry1)
                    DespawnWandererBot(bot->GetEntry());
            return false;
        }
    }
    if (needed_bots_count_h)
    {
        if (!sBotGen->GenerateWanderingBotsToSpawn(needed_bots_count_h, bg_template->GetMapId(), HORDE, true, bracketEntry, &spawned_bots_h, spawned_h))
        {
            BOT_LOG_WARN("npcbots", "Failed to spawn {} HORDE bots for BG {} '{}' queued A {} H {} req A {} H {} spare {}",
                needed_bots_count_h, uint32(bg_template->GetTypeID()), bg_template->GetName(),
                queued_players_a, queued_players_h, needed_bots_count_a, needed_bots_count_h, spare_bots_h);
            for (NpcBotRegistry const& registry2 : spawned_bots)
                for (Creature const* bot : registry2)
                    DespawnWandererBot(bot->GetEntry());
            return false;
        }
    }

    ASSERT(uint32(spawned_bots_a.size()) == needed_bots_count_a);
    ASSERT(uint32(spawned_bots_h.size()) == needed_bots_count_h);

    botBGJoinEvents[groupLeader->GetGUID()].AddEventAtOffset([ammr = ammr, bgqTypeId = bgqTypeId]() {
        sBattlegroundMgr->ScheduleQueueUpdate(ammr, bgqTypeId);
    }, Seconds(2));

    uint8 maxlevel = BotCfg::IsBotLevelCappedByConfigBGFirstPlayer() ? groupLeader->GetLevel() : 0;
    for (NpcBotRegistry const& registry3 : spawned_bots)
    {
        uint32 seconds_delay = 5;
        for (Creature const* bot : registry3)
        {
            bot->GetBotAI()->SetBotCommandState(BOT_COMMAND_STAY);
            bot->GetBotAI()->canUpdate = false;

            const_cast<Creature*>(bot)->SetPvP(true);
            if (maxlevel && bot->GetLevel() > maxlevel)
                const_cast<Creature*>(bot)->SetLevel(maxlevel);
            queue->AddBotAsGroup(bot->GetGUID(), GetTeamIdForFaction(bot->GetFaction()) == TEAM_HORDE ? HORDE : ALLIANCE,
                bracketEntry, false, gqinfo->ArenaTeamRating, ammr);

            seconds_delay = std::min<uint32>(uint32(MINUTE * 2), seconds_delay + std::max<uint32>(1u, uint32((MINUTE / 2) / std::max<uint32>(needed_bots_count_a, needed_bots_count_h))));

            BotBattlegroundEnterEvent* bbe = new BotBattlegroundEnterEvent(groupLeader->GetGUID(), bot->GetGUID(), bgqTypeId,
                botBGJoinEvents[groupLeader->GetGUID()].CalculateTime(Milliseconds(uint32(INVITE_ACCEPT_WAIT_TIME) + uint32(BG_START_DELAY_2M))).count());
            botBGJoinEvents[groupLeader->GetGUID()].AddEventAtOffset(bbe, Seconds(seconds_delay));
        }
    }

    return true;
}

ItemPerBotClassPerBotCategoryMap const& BotDataMgr::GetWanderingBotsSortedGearMap()
{
    return _botsExtraCreatureSortedGear;
}

void BotDataMgr::CreateGeneratedBotsSortedGear()
{
    BOT_LOG_INFO("server.loading", "Sorting wandering bot's gear...");

    uint32 oldMSTime = getMSTime();

    std::unordered_set<uint32> disabled_item_ids;
    QueryResult dires = WorldDatabase.Query("SELECT id FROM creature_template_npcbot_disabled_items");
    if (dires)
    {
        do
        {
            uint32 id = dires->Fetch()->GetUInt32();
            disabled_item_ids.insert(id);

        } while (dires->NextRow());

        BOT_LOG_INFO("server.loading", ">> Loaded {} disabled wandering bots gear items", uint32(disabled_item_ids.size()));
    }
    else
        BOT_LOG_INFO("server.loading", ">> Loaded 0 disabled wandering bots gear items. Table `creature_template_npcbot_disabled_items` is empty!");

    const std::map<uint32, uint8> InvTypeToBotSlot = {
        {INVTYPE_HEAD, BOT_SLOT_HEAD},
        {INVTYPE_SHOULDERS, BOT_SLOT_SHOULDERS},
        {INVTYPE_CHEST, BOT_SLOT_CHEST},
        {INVTYPE_ROBE, BOT_SLOT_CHEST},
        {INVTYPE_WAIST, BOT_SLOT_WAIST},
        {INVTYPE_LEGS, BOT_SLOT_LEGS},
        {INVTYPE_FEET, BOT_SLOT_FEET},
        {INVTYPE_WRISTS, BOT_SLOT_WRIST},
        {INVTYPE_HANDS, BOT_SLOT_HANDS}
    };

    const uint32 InvTypeMaskAccessory = (1u << INVTYPE_FINGER) | (1u << INVTYPE_TRINKET) | (1u << INVTYPE_CLOAK) | (1u << INVTYPE_NECK) | (1u << INVTYPE_SHIELD);

    auto push_gear_to_classes = [](ItemTemplate const& itt, uint32 category_mask, uint8 slot, uint8 lstep, std::initializer_list<BotClasses> const& cs) {
        for (auto category : NPCBots::index_array<uint8, NpcBotGeneratedCategory::BOT_GENERATION_CATEGORIES_COUNT>)
        {
            if (!(category_mask & (1u << category)))
                continue;
            for (BotClasses c : cs)
            {
                if (c == BOT_CLASS_SPHYNX && ((1u << itt.InventoryType) & InvTypeMaskAccessory))
                    continue;
                if (!itt.AllowableClass || itt.AllowableClass >= ((1u << MAX_CLASSES) - 1) || !!(itt.AllowableClass & (1u << (c - 1))))
                    _botsExtraCreatureSortedGear[category][c][slot][lstep].push_back(itt.ItemId);
            }
        }
    };

    const std::initializer_list<BotClasses> IntUsers = { BOT_CLASS_PALADIN, BOT_CLASS_PRIEST, BOT_CLASS_SHAMAN, BOT_CLASS_MAGE, BOT_CLASS_WARLOCK, BOT_CLASS_DRUID, BOT_CLASS_SPHYNX, BOT_CLASS_ARCHMAGE, BOT_CLASS_DREADLORD, BOT_CLASS_NECROMANCER, BOT_CLASS_SEA_WITCH, BOT_CLASS_CRYPT_LORD };
    const std::initializer_list<BotClasses> StrUsers = { BOT_CLASS_WARRIOR, BOT_CLASS_DEATH_KNIGHT, BOT_CLASS_SPELLBREAKER, BOT_CLASS_CRYPT_LORD, BOT_CLASS_PALADIN };
    const std::initializer_list<BotClasses> AgiUsers = { BOT_CLASS_HUNTER, BOT_CLASS_SHAMAN, BOT_CLASS_ROGUE, BOT_CLASS_DRUID, BOT_CLASS_BM, BOT_CLASS_DARK_RANGER };

    uint32 relics_pushed_to_pool = 0;
    ItemTemplateContainer const& all_item_templates = sObjectMgr->GetItemTemplateStore();
    for (auto const& [_, proto] : all_item_templates)
    {
        bool const isRelicItem = proto.Class == ITEM_CLASS_ARMOR && (
            proto.InventoryType == INVTYPE_RELIC
            || proto.SubClass == ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_LIBRAM
            || proto.SubClass == ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_IDOL
            || proto.SubClass == ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_TOTEM
            || proto.SubClass == ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_SIGIL);

        // Relics: dedicated pool path (often skipped by RequiredSpell / ItemLevel / quality rules below).
        if (isRelicItem)
        {
            if (disabled_item_ids.contains(proto.ItemId))
                continue;
            if (proto.RequiredLevel < 1 || proto.RequiredLevel > DEFAULT_MAX_LEVEL)
                continue;
            if ((proto.Quality < ITEM_QUALITY_UNCOMMON || proto.Quality > ITEM_QUALITY_EPIC) && proto.Quality != ITEM_QUALITY_HEIRLOOM)
                continue;

            uint32 relic_gencat_mask = 0;
            if (proto.StatsCount > 0 && std::ranges::any_of(proto.ItemStat, [](_ItemStat const& stat) {
                return stat.ItemStatType == ITEM_MOD_RESILIENCE_RATING && stat.ItemStatValue > 0;
            }))
                relic_gencat_mask = 1u << NpcBotGeneratedCategory::BOT_GENERATED_WANDERING;
            else
                relic_gencat_mask = NPCBOT_GENERATED_CATEGORY_MASK_ALL;

            const uint8 reqLstepRelic = (((proto.RequiredLevel == 1) ? 0 : proto.RequiredLevel) + ITEM_SORTING_LEVEL_STEP - 1) / ITEM_SORTING_LEVEL_STEP;
            const uint8 minStepRelic = proto.RequiredLevel <= 1 ? 0 : (proto.RequiredLevel - 1) / ITEM_SORTING_LEVEL_STEP;
            const uint8 maxStepRelic = std::min<uint8>(reqLstepRelic, LEVEL_STEPS - 1);

            BotClasses relic_class = BOT_CLASS_NONE;
            switch (proto.SubClass)
            {
                case ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_LIBRAM: relic_class = BOT_CLASS_PALADIN; break;
                case ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_IDOL:   relic_class = BOT_CLASS_DRUID; break;
                case ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_TOTEM:  relic_class = BOT_CLASS_SHAMAN; break;
                case ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_SIGIL:  relic_class = BOT_CLASS_DEATH_KNIGHT; break;
                default: continue;
            }

            for (auto category : NPCBots::index_array<uint8, NpcBotGeneratedCategory::BOT_GENERATION_CATEGORIES_COUNT>)
            {
                if (!(relic_gencat_mask & (1u << category)))
                    continue;
                for (uint8 step = minStepRelic; step <= maxStepRelic; ++step)
                {
                    _botsExtraCreatureSortedGear[category][relic_class][BOT_SLOT_RANGED][step].push_back(proto.ItemId);
                    ++relics_pushed_to_pool;
                }
            }
            continue;
        }

        if ((!proto.ItemLevel && !isRelicItem) || (proto.RequiredSpell && !isRelicItem))
            continue;

        bool skip = false;
        switch (proto.Quality)
        {
            case ITEM_QUALITY_NORMAL:
                if (proto.InventoryType != INVTYPE_BODY)
                {
                    if (std::ranges::any_of(proto.Effects, [](ItemEffect const& spell) { return !!spell.SpellID; }))
                        skip = true;
                    if (proto.RequiredLevel > 14)
                        skip = true;
                }
                break;
            case ITEM_QUALITY_UNCOMMON:
                if (proto.RequiredLevel > 75)
                    skip = true;
            [[fallthrough]];
            case ITEM_QUALITY_RARE:
            //    if (proto.RequiredLevel > 75 && proto.RequiredLevel < 80)
            //        skip = true;
            //[[fallthrough]];
            case ITEM_QUALITY_EPIC:
                if (!(proto.RequiredLevel >= 2 && proto.RequiredLevel <= DEFAULT_MAX_LEVEL))
                    skip = true;
                else if (proto.Class == ITEM_CLASS_WEAPON ||
                    proto.InventoryType == INVTYPE_RELIC || proto.InventoryType == INVTYPE_SHIELD)
                {
                    // weapons / relics / shields do not require random enchants or stat rolls to enter the pool
                }
                else if (!proto.RandomProperty && !proto.RandomSuffix && !proto.StatsCount)
                    skip = true;
                break;
            default:
                skip = true;
                break;
        }
        if (skip)
            continue;

        if (disabled_item_ids.contains(proto.ItemId))
        {
            //BOT_LOG_INFO("server.loading", "Item {} is disabled...", proto.ItemId);
            continue;
        }

        uint32 gencat_mask = 0;
        if (proto.StatsCount > 0 && std::ranges::any_of(proto.ItemStat, [](_ItemStat const& stat) {
            return (stat.ItemStatType == ITEM_MOD_DEFENSE_SKILL_RATING || stat.ItemStatType == ITEM_MOD_DODGE_RATING ||
                stat.ItemStatType == ITEM_MOD_PARRY_RATING || stat.ItemStatType == ITEM_MOD_BLOCK_VALUE) &&
                stat.ItemStatValue > 0;
        }))
        {
            gencat_mask = 1u << NpcBotGeneratedCategory::BOT_GENERATED_DUNGEON;
        }
        else if (proto.StatsCount > 0 && std::ranges::any_of(proto.ItemStat, [](_ItemStat const& stat) {
            return stat.ItemStatType == ITEM_MOD_RESILIENCE_RATING && stat.ItemStatValue > 0;
        }))
        {
            gencat_mask = 1u << NpcBotGeneratedCategory::BOT_GENERATED_WANDERING;
        }
        else
            gencat_mask = NPCBOT_GENERATED_CATEGORY_MASK_ALL;

        const uint8 reqLstep = (((proto.RequiredLevel == 1) ? 0 : proto.RequiredLevel) + ITEM_SORTING_LEVEL_STEP - 1) / ITEM_SORTING_LEVEL_STEP;
        const bool is_caster_item = proto.StatsCount > 0 && std::ranges::any_of(proto.ItemStat, [](_ItemStat const& stat) {
            return (stat.ItemStatType == ITEM_MOD_INTELLECT || stat.ItemStatType == ITEM_MOD_SPELL_POWER ||
                stat.ItemStatType == ITEM_MOD_SPELL_PENETRATION || stat.ItemStatType == ITEM_MOD_MANA_REGENERATION) &&
                stat.ItemStatValue > 0;
        });

        // Relics / ranged: place into every level step bracket this item is actually equippable at (not into step 0..max).
        auto push_gear_to_classes_through_step = [&](uint8 slot, std::initializer_list<BotClasses> const& cs) {
            const uint8 minStep = proto.RequiredLevel <= 1 ? 0 : (proto.RequiredLevel - 1) / ITEM_SORTING_LEVEL_STEP;
            const uint8 maxStep = std::min<uint8>(reqLstep, LEVEL_STEPS - 1);
            for (uint8 step = minStep; step <= maxStep; ++step)
                push_gear_to_classes(proto, gencat_mask, slot, step, cs);
        };

        switch (proto.Class)
        {
            case ITEM_CLASS_ARMOR:
            {
                const bool is_strength_item = proto.StatsCount > 0 && std::ranges::any_of(proto.ItemStat, [](_ItemStat const& stat) {
                    return stat.ItemStatType == ITEM_MOD_STRENGTH && stat.ItemStatValue > 0;
                });
                const bool is_agility_item = proto.StatsCount > 0 && std::ranges::any_of(proto.ItemStat, [](_ItemStat const& stat) {
                    return stat.ItemStatType == ITEM_MOD_AGILITY && stat.ItemStatValue > 0;
                });
                switch (proto.InventoryType)
                {
                    case INVTYPE_NECK:
                        if (proto.Quality < ITEM_QUALITY_UNCOMMON)
                            break;
                        if (is_caster_item || (reqLstep < LEVEL_STEPS - 1 && proto.StatsCount == 0))
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_NECK, reqLstep, IntUsers);
                        if (is_strength_item || (reqLstep < LEVEL_STEPS - 1 && proto.StatsCount == 0))
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_NECK, reqLstep, StrUsers);
                        if (is_agility_item || (reqLstep < LEVEL_STEPS - 1 && proto.StatsCount == 0))
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_NECK, reqLstep, AgiUsers);
                        break;
                    case INVTYPE_FINGER:
                        if (proto.Quality < ITEM_QUALITY_UNCOMMON)
                            break;
                        if (is_caster_item || (reqLstep < LEVEL_STEPS - 1 && proto.StatsCount == 0))
                        {
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_FINGER1, reqLstep, IntUsers);
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_FINGER2, reqLstep, IntUsers);
                        }
                        if (is_strength_item || (reqLstep < LEVEL_STEPS - 1 && proto.StatsCount == 0))
                        {
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_FINGER1, reqLstep, StrUsers);
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_FINGER2, reqLstep, StrUsers);
                        }
                        if (is_agility_item || (reqLstep < LEVEL_STEPS - 1 && proto.StatsCount == 0))
                        {
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_FINGER1, reqLstep, AgiUsers);
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_FINGER2, reqLstep, AgiUsers);
                        }
                        break;
                    case INVTYPE_TRINKET:
                        if (proto.Quality < ITEM_QUALITY_UNCOMMON)
                            break;
                        if (!is_strength_item && !is_agility_item)
                        {
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_TRINKET1, reqLstep, IntUsers);
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_TRINKET2, reqLstep, IntUsers);
                        }
                        if (!is_caster_item)
                        {
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_TRINKET1, reqLstep, StrUsers);
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_TRINKET2, reqLstep, StrUsers);
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_TRINKET1, reqLstep, AgiUsers);
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_TRINKET2, reqLstep, AgiUsers);
                        }
                        break;
                    case INVTYPE_CLOAK:
                        if (is_caster_item || (reqLstep < LEVEL_STEPS - 1 && proto.StatsCount == 0))
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_BACK, reqLstep, IntUsers);
                        if (is_strength_item || (reqLstep < LEVEL_STEPS - 1 && proto.StatsCount == 0))
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_BACK, reqLstep, StrUsers);
                        if (is_agility_item || (reqLstep < LEVEL_STEPS - 1 && proto.StatsCount == 0))
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_BACK, reqLstep, AgiUsers);
                        break;
                    case INVTYPE_BODY:
                        if (proto.Quality > ITEM_QUALITY_UNCOMMON)
                            break;
                        push_gear_to_classes(proto, gencat_mask, BOT_SLOT_BODY, reqLstep, IntUsers);
                        push_gear_to_classes(proto, gencat_mask, BOT_SLOT_BODY, reqLstep, StrUsers);
                        push_gear_to_classes(proto, gencat_mask, BOT_SLOT_BODY, reqLstep, AgiUsers);
                        break;
                    case INVTYPE_RELIC:
                        push_gear_to_classes_through_step(BOT_SLOT_RANGED, { BOT_CLASS_PALADIN, BOT_CLASS_SHAMAN, BOT_CLASS_DRUID, BOT_CLASS_DEATH_KNIGHT });
                        break;
                    case INVTYPE_HOLDABLE:
                        if (proto.Quality < ITEM_QUALITY_UNCOMMON)
                            break;
                        push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_PRIEST, BOT_CLASS_MAGE, BOT_CLASS_WARLOCK, BOT_CLASS_DRUID });
                        break;
                    case INVTYPE_SHIELD:
                        if (proto.Armor == 0)
                            break;
                        push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_PALADIN, BOT_CLASS_SPELLBREAKER });
                        if (!is_caster_item)
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_WARRIOR });
                        if (is_caster_item || proto.RequiredLevel < 60 || (proto.RequiredLevel < 69 && (proto.RandomProperty || proto.RandomSuffix)))
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_SHAMAN });
                        break;
                    case INVTYPE_HEAD:
                    case INVTYPE_SHOULDERS:
                    case INVTYPE_CHEST:
                    case INVTYPE_ROBE:
                    case INVTYPE_WAIST:
                    case INVTYPE_LEGS:
                    case INVTYPE_FEET:
                    case INVTYPE_WRISTS:
                    case INVTYPE_HANDS:
                    {
                        if (proto.Armor == 0)
                            break;
                        decltype(InvTypeToBotSlot)::const_iterator ci = InvTypeToBotSlot.find(proto.InventoryType);
                        ASSERT(ci != InvTypeToBotSlot.cend());
                        uint8 slot = ci->second;
                        switch (proto.SubClass)
                        {
                            case ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_CLOTH:
                                if (slot == BOT_SLOT_CHEST && proto.InventoryType != INVTYPE_ROBE)
                                    break;
                                push_gear_to_classes(proto, gencat_mask, slot, reqLstep, { BOT_CLASS_PRIEST, BOT_CLASS_MAGE, BOT_CLASS_WARLOCK, BOT_CLASS_ARCHMAGE, BOT_CLASS_NECROMANCER, BOT_CLASS_SEA_WITCH });
                                break;
                            case ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_LEATHER:
                                if (!is_caster_item)
                                    push_gear_to_classes(proto, gencat_mask, slot, reqLstep, { BOT_CLASS_ROGUE, BOT_CLASS_DARK_RANGER });
                                push_gear_to_classes(proto, gencat_mask, slot, reqLstep, { BOT_CLASS_DRUID });
                                if (proto.RequiredLevel < 40)
                                    push_gear_to_classes(proto, gencat_mask, slot, reqLstep, { BOT_CLASS_HUNTER, BOT_CLASS_SHAMAN });
                                break;
                            case ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_MAIL:
                                if (proto.RequiredLevel < 40)
                                {
                                    if (!is_caster_item)
                                        push_gear_to_classes(proto, gencat_mask, slot, reqLstep, { BOT_CLASS_WARRIOR });
                                    push_gear_to_classes(proto, gencat_mask, slot, reqLstep, { BOT_CLASS_PALADIN });
                                }
                                else
                                    push_gear_to_classes(proto, gencat_mask, slot, reqLstep, { BOT_CLASS_HUNTER, BOT_CLASS_SHAMAN });
                                if (!is_caster_item)
                                    push_gear_to_classes(proto, gencat_mask, slot, reqLstep, { BOT_CLASS_BM, BOT_CLASS_SPELLBREAKER });
                                push_gear_to_classes(proto, gencat_mask, slot, reqLstep, { BOT_CLASS_SPHYNX, BOT_CLASS_CRYPT_LORD });
                                break;
                            case ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_PLATE:
                                push_gear_to_classes(proto, gencat_mask, slot, reqLstep, { BOT_CLASS_PALADIN, BOT_CLASS_SPELLBREAKER });
                                if (!is_caster_item)
                                    push_gear_to_classes(proto, gencat_mask, slot, reqLstep, { BOT_CLASS_WARRIOR, BOT_CLASS_DEATH_KNIGHT, BOT_CLASS_BM });
                                if (is_caster_item || proto.RequiredLevel < 60 || (proto.RequiredLevel < 78 && (proto.RandomProperty || proto.RandomSuffix)))
                                    push_gear_to_classes(proto, gencat_mask, slot, reqLstep, { BOT_CLASS_SPHYNX, BOT_CLASS_DREADLORD, BOT_CLASS_CRYPT_LORD });
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case ITEM_CLASS_WEAPON:
                if (proto.Damage[0].DamageMin < 1.0f || proto.Damage[0].DamageMax < 2.0f || proto.Delay < 1000)
                    break;
                if (proto.RequiredLevel > 75 && proto.Quality < ITEM_QUALITY_EPIC)
                    break;
                switch (proto.SubClass)
                {
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_WAND:
                        if (proto.InventoryType != INVTYPE_RANGED && proto.InventoryType != INVTYPE_RANGEDRIGHT)
                            break;
                        push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_SPHYNX });
                        push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_SPHYNX });
                        push_gear_to_classes_through_step(BOT_SLOT_RANGED, { BOT_CLASS_PRIEST, BOT_CLASS_MAGE, BOT_CLASS_WARLOCK });
                        break;
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_GUN:
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_CROSSBOW:
                        if (proto.InventoryType != INVTYPE_RANGED && proto.InventoryType != INVTYPE_RANGEDRIGHT)
                            break;
                        push_gear_to_classes_through_step(BOT_SLOT_RANGED, { BOT_CLASS_WARRIOR, BOT_CLASS_ROGUE, BOT_CLASS_HUNTER });
                        break;
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_BOW:
                        if (proto.InventoryType != INVTYPE_RANGED && proto.InventoryType != INVTYPE_RANGEDRIGHT)
                            break;
                        push_gear_to_classes_through_step(BOT_SLOT_RANGED, { BOT_CLASS_WARRIOR, BOT_CLASS_ROGUE, BOT_CLASS_HUNTER, BOT_CLASS_DARK_RANGER, BOT_CLASS_SEA_WITCH });
                        break;
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_THROWN:
                        if (proto.InventoryType != INVTYPE_THROWN)
                            break;
                        push_gear_to_classes_through_step(BOT_SLOT_RANGED, { BOT_CLASS_WARRIOR, BOT_CLASS_ROGUE });
                        break;
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_STAFF:
                        if (proto.InventoryType != INVTYPE_2HWEAPON)
                            break;
                        if (is_caster_item || proto.RequiredLevel < 50)
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_PRIEST, BOT_CLASS_MAGE, BOT_CLASS_WARLOCK, BOT_CLASS_DRUID, BOT_CLASS_SHAMAN, BOT_CLASS_ARCHMAGE, BOT_CLASS_NECROMANCER, BOT_CLASS_DREADLORD, BOT_CLASS_CRYPT_LORD });
                        break;
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_AXE2:
                        if (proto.InventoryType != INVTYPE_2HWEAPON)
                            break;
                        if (!is_caster_item)
                        {
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_WARRIOR, BOT_CLASS_PALADIN, BOT_CLASS_DEATH_KNIGHT, BOT_CLASS_BM });
                            if (proto.RequiredLevel >= 60 - ITEM_SORTING_LEVEL_STEP)
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_WARRIOR });
                        }
                        push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_HUNTER, BOT_CLASS_SHAMAN, BOT_CLASS_DREADLORD, BOT_CLASS_CRYPT_LORD });
                        break;
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_SWORD2:
                        if (proto.InventoryType != INVTYPE_2HWEAPON)
                            break;
                        if (!is_caster_item)
                        {
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_WARRIOR, BOT_CLASS_PALADIN, BOT_CLASS_DEATH_KNIGHT, BOT_CLASS_BM });
                            if (proto.RequiredLevel >= 60 - ITEM_SORTING_LEVEL_STEP)
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_WARRIOR });
                        }
                        push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_HUNTER, BOT_CLASS_DREADLORD, BOT_CLASS_CRYPT_LORD });
                        break;
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_POLEARM:
                        if (proto.InventoryType != INVTYPE_2HWEAPON)
                            break;
                        if (!is_caster_item)
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_WARRIOR, BOT_CLASS_PALADIN, BOT_CLASS_DEATH_KNIGHT, BOT_CLASS_BM });
                        push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_HUNTER, BOT_CLASS_DRUID, BOT_CLASS_DREADLORD, BOT_CLASS_CRYPT_LORD });
                        break;
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_MACE2:
                        if (proto.InventoryType != INVTYPE_2HWEAPON)
                            break;
                        if (!is_caster_item)
                        {
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_WARRIOR, BOT_CLASS_PALADIN, BOT_CLASS_DEATH_KNIGHT });
                            if (proto.RequiredLevel >= 60 - ITEM_SORTING_LEVEL_STEP)
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_WARRIOR });
                        }
                        push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_DRUID, BOT_CLASS_DREADLORD, BOT_CLASS_CRYPT_LORD });
                        break;
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_AXE:
                        if (proto.InventoryType == INVTYPE_WEAPON || proto.InventoryType == INVTYPE_WEAPONMAINHAND)
                        {
                            if (!is_caster_item)
                            {
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_WARRIOR });
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_ROGUE, BOT_CLASS_SPELLBREAKER });
                            }
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_PALADIN, BOT_CLASS_SHAMAN });
                        }
                        if (proto.InventoryType == INVTYPE_WEAPON || proto.InventoryType == INVTYPE_WEAPONOFFHAND)
                        {
                            if (!is_caster_item)
                            {
                                if (proto.RequiredLevel < 60 - ITEM_SORTING_LEVEL_STEP)
                                    push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_WARRIOR });
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_ROGUE });
                            }
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_SHAMAN });
                        }
                        break;
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_MACE:
                        if (proto.InventoryType == INVTYPE_WEAPON || proto.InventoryType == INVTYPE_WEAPONMAINHAND)
                        {
                            if (!is_caster_item)
                            {
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_WARRIOR });
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_ROGUE, BOT_CLASS_SPELLBREAKER });
                                if (proto.RequiredLevel >= 55)
                                   push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_DEATH_KNIGHT });
                            }
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_PALADIN, BOT_CLASS_SHAMAN });
                            if (is_caster_item || proto.RequiredLevel < 55 || (proto.RequiredLevel < 78 && (proto.RandomProperty || proto.RandomSuffix)))
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_DRUID, BOT_CLASS_PRIEST });
                        }
                        if (proto.InventoryType == INVTYPE_WEAPON || proto.InventoryType == INVTYPE_WEAPONOFFHAND)
                        {
                            if (!is_caster_item)
                            {
                                if (proto.RequiredLevel < 60 - ITEM_SORTING_LEVEL_STEP)
                                    push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_WARRIOR });
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_ROGUE });
                                if (proto.RequiredLevel >= 55)
                                   push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_DEATH_KNIGHT });
                            }
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_SHAMAN });
                        }
                        break;
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_SWORD:
                        if (proto.InventoryType == INVTYPE_WEAPON || proto.InventoryType == INVTYPE_WEAPONMAINHAND)
                        {
                            if (!is_caster_item)
                            {
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_WARRIOR });
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_ROGUE, BOT_CLASS_SPELLBREAKER, BOT_CLASS_DARK_RANGER });
                                if (proto.RequiredLevel >= 55)
                                   push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_DEATH_KNIGHT });
                            }
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_PALADIN });
                            if (is_caster_item || proto.RequiredLevel < 55 || (proto.RequiredLevel < 78 && (proto.RandomProperty || proto.RandomSuffix)))
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_MAGE, BOT_CLASS_WARLOCK });
                        }
                        if (proto.InventoryType == INVTYPE_WEAPON || proto.InventoryType == INVTYPE_WEAPONOFFHAND)
                        {
                            if (!is_caster_item)
                            {
                                if (proto.RequiredLevel < 60 - ITEM_SORTING_LEVEL_STEP)
                                    push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_WARRIOR });
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_ROGUE, BOT_CLASS_DARK_RANGER });
                                if (proto.RequiredLevel >= 55)
                                   push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_DEATH_KNIGHT });
                            }
                        }
                        break;
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_FIST_WEAPON:
                        if (proto.InventoryType == INVTYPE_WEAPON || proto.InventoryType == INVTYPE_WEAPONMAINHAND)
                        {
                            if (!is_caster_item)
                            {
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_WARRIOR });
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_SHAMAN, BOT_CLASS_ROGUE, BOT_CLASS_SPELLBREAKER });
                            }
                        }
                        if (proto.InventoryType == INVTYPE_WEAPON || proto.InventoryType == INVTYPE_WEAPONOFFHAND)
                        {
                            if (!is_caster_item)
                            {
                                if (proto.RequiredLevel < 60 - ITEM_SORTING_LEVEL_STEP)
                                    push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_WARRIOR });
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_SHAMAN, BOT_CLASS_ROGUE });
                            }
                        }
                        break;
                    case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_DAGGER:
                        if (proto.InventoryType == INVTYPE_WEAPON || proto.InventoryType == INVTYPE_WEAPONMAINHAND)
                        {
                            if (!is_caster_item)
                            {
                                if (proto.RequiredLevel < 60 - ITEM_SORTING_LEVEL_STEP)
                                    push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_WARRIOR });
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_ROGUE, BOT_CLASS_SPELLBREAKER, BOT_CLASS_DARK_RANGER });
                            }
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_SHAMAN });
                            if (is_caster_item || proto.RequiredLevel < 55 || (proto.RequiredLevel < 78 && (proto.RandomProperty || proto.RandomSuffix)))
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_MAINHAND, reqLstep, { BOT_CLASS_PRIEST, BOT_CLASS_MAGE, BOT_CLASS_WARLOCK, BOT_CLASS_DRUID, BOT_CLASS_SEA_WITCH });
                        }
                        if (proto.InventoryType == INVTYPE_WEAPON || proto.InventoryType == INVTYPE_WEAPONOFFHAND)
                        {
                            if (!is_caster_item)
                            {
                                if (proto.RequiredLevel < 60 - ITEM_SORTING_LEVEL_STEP)
                                    push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_WARRIOR });
                                push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_ROGUE, BOT_CLASS_DARK_RANGER });
                            }
                            push_gear_to_classes(proto, gencat_mask, BOT_SLOT_OFFHAND, reqLstep, { BOT_CLASS_SHAMAN, BOT_CLASS_SEA_WITCH });
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    static const std::array<std::pair<uint8, std::array<uint32, 3>>, 4> relicPoolSeeds = { {
        { BOT_CLASS_PALADIN,      { 18334, 22401, 23006 } },
        { BOT_CLASS_DEATH_KNIGHT, { 39208, 40714, 0 } },
        { BOT_CLASS_DRUID,        { 25667, 22398, 23197 } },
        { BOT_CLASS_SHAMAN,       { 5175, 22345, 23044 } },
    } };
    for (auto const& [botclass, entries] : relicPoolSeeds)
    {
        for (uint32 candidate : entries)
        {
            if (!candidate || !sObjectMgr->GetItemTemplate(candidate))
                continue;
            for (auto category : NPCBots::index_array<uint8, NpcBotGeneratedCategory::BOT_GENERATION_CATEGORIES_COUNT>)
            {
                for (auto step : NPCBots::index_array<uint8, LEVEL_STEPS>)
                    _botsExtraCreatureSortedGear[category][botclass][BOT_SLOT_RANGED][step].push_back(candidate);
            }
        }
    }

    static const std::array<uint32, 10> bodyPoolSeedCandidates = {
        38u,    // White Shirt
        6098u,  // Apprentice's Shirt
        2575u,  // Red Linen Shirt
        2577u,  // Green Linen Shirt
        2579u,  // Blue Linen Shirt
        4330u,  // Formal White Shirt
        4333u,  // Dark Brown Shirt
        4334u,  // Brown Linen Shirt
        6795u,  // White Swashbuckler's Shirt
        6831u,  // Black Swashbuckler's Shirt
    };
    uint32 body_seeds_pushed = 0;
    for (uint32 candidate : bodyPoolSeedCandidates)
    {
        ItemTemplate const* bodyProto = sObjectMgr->GetItemTemplate(candidate);
        if (!bodyProto || bodyProto->InventoryType != INVTYPE_BODY || disabled_item_ids.contains(candidate))
            continue;
        for (auto category : NPCBots::index_array<uint8, NpcBotGeneratedCategory::BOT_GENERATION_CATEGORIES_COUNT>)
        {
            for (auto c : NPCBots::index_array<uint32, BOT_CLASS_END>)
            {
                if (!((1u << c) & ALL_BOT_CLASSES_MASK))
                    continue;
                for (auto step : NPCBots::index_array<uint8, LEVEL_STEPS>)
                    _botsExtraCreatureSortedGear[category][c][BOT_SLOT_BODY][step].push_back(candidate);
            }
        }
        ++body_seeds_pushed;
    }

    uint32 missingGearPools = 0;
    for (auto category : NPCBots::index_array<uint32, NpcBotGeneratedCategory::BOT_GENERATION_CATEGORIES_COUNT>)
    {
        auto const& class_map = _botsExtraCreatureSortedGear.at(category);
        for (auto c : NPCBots::index_array<uint32, BOT_CLASS_END>)
        {
            if (!((1u << c) & ALL_BOT_CLASSES_MASK))
                continue;

            ItemPerSlot const& ips_arr = class_map.at(c);
            for (uint32 s = BOT_SLOT_MAINHAND; s < BOT_INVENTORY_SIZE; ++s)
            {
                if (s == BOT_SLOT_FINGER2 || s == BOT_SLOT_TRINKET1 || s == BOT_SLOT_TRINKET2)
                    continue;
                if ((s == BOT_SLOT_FINGER1 || s == BOT_SLOT_NECK || s == BOT_SLOT_BACK) && c == BOT_CLASS_SPHYNX)
                    continue;
                if (s == BOT_SLOT_RANGED && !UsesGeneratedBotRangedSlot(c))
                    continue;
                if (s == BOT_SLOT_BODY)
                    continue;
                ItemLeveledArr const& il_arr = ips_arr[s];

                for (auto lstep : NPCBots::index_array<uint32, LEVEL_STEPS>)
                {
                    if ((s == BOT_SLOT_SHOULDERS || s == BOT_SLOT_FINGER1 || s == BOT_SLOT_NECK) && lstep < 4)
                        continue;
                    if ((s == BOT_SLOT_HEAD || s == BOT_SLOT_TRINKET1) && lstep < 6)
                        continue;
                    if (s == BOT_SLOT_OFFHAND &&
                        (lstep < 3 || c == BOT_CLASS_PALADIN || c == BOT_CLASS_HUNTER || c == BOT_CLASS_DEATH_KNIGHT || c == BOT_CLASS_BM || c == BOT_CLASS_ARCHMAGE ||
                            c == BOT_CLASS_SPHYNX || c == BOT_CLASS_DREADLORD || c == BOT_CLASS_NECROMANCER || c == BOT_CLASS_CRYPT_LORD))
                        continue;
                    if ((c == BOT_CLASS_DREADLORD || c == BOT_CLASS_DEATH_KNIGHT) && lstep < 8)
                        continue;
                    if (il_arr[lstep].empty())
                        ++missingGearPools;
                }
            }
        }
    }

    if (missingGearPools)
        BOT_LOG_WARN("server.loading", ">> Wandering bot gear: {} empty class/slot/level pools remain (non-shirt slots only)", missingGearPools);

    BOT_LOG_INFO("server.loading", ">> Sorted wandering bots gear ({} relic entries, {} shirt seeds, paladin relic pool={}) in {} ms",
        relics_pushed_to_pool, body_seeds_pushed, _botsExtraCreatureSortedGear[BOT_GENERATED_WANDERING][BOT_CLASS_PALADIN][BOT_SLOT_RANGED][0].size(),
        GetMSTimeDiffToNow(oldMSTime));

    // By leewheel 20260529 - vendor listings from item_template (not bot equipment).
    WandererVendor::BuildCatalog();
}

bool BotDataMgr::UsesGeneratedBotRangedSlot(uint8 botclass)
{
    switch (botclass)
    {
        case BOT_CLASS_HUNTER:
        case BOT_CLASS_ROGUE:
        case BOT_CLASS_PRIEST:
        case BOT_CLASS_MAGE:
        case BOT_CLASS_WARLOCK:
        case BOT_CLASS_PALADIN:
        case BOT_CLASS_SHAMAN:
        case BOT_CLASS_DRUID:
        case BOT_CLASS_DEATH_KNIGHT:
        case BOT_CLASS_DARK_RANGER:
        case BOT_CLASS_SEA_WITCH:
            return true;
        default:
            return false;
    }
}

uint32 BotDataMgr::GetWandererHeirloomWeaponFallback(uint8 botclass, uint8 slot)
{
    if (slot > BOT_SLOT_RANGED)
        return 0;

    // WotLK heirloom weapons (item_template Quality=7); always present in DB
    enum HeirloomWeapon : uint32
    {
        HEIRLOOM_2H_SWORD   = 42945, // Venerable Dal'Rend's Sacred Charge
        HEIRLOOM_2H_AXE     = 42943, // Venerable Mass of McGraw's Might
        HEIRLOOM_STAFF      = 42947, // Dignified Headmaster's Charge
        HEIRLOOM_1H_SWORD   = 44096, // Battleworn Thrash Blade
        HEIRLOOM_DAGGER     = 44091, // Sharpened Scarlet Kris
        HEIRLOOM_BOW        = 42946, // Charmed Ancient Bone Bow
    };

    switch (botclass)
    {
        case BOT_CLASS_WARRIOR:
            if (slot == BOT_SLOT_MAINHAND)
                return ResolveExistingItemEntry({ HEIRLOOM_2H_SWORD });
            if (slot == BOT_SLOT_OFFHAND)
                return ResolveExistingItemEntry({ HEIRLOOM_1H_SWORD });
            return 0;
        case BOT_CLASS_DEATH_KNIGHT:
            if (slot == BOT_SLOT_MAINHAND)
                return ResolveExistingItemEntry({ HEIRLOOM_2H_SWORD });
            if (slot == BOT_SLOT_OFFHAND)
                return ResolveExistingItemEntry({ HEIRLOOM_1H_SWORD });
            if (slot == BOT_SLOT_RANGED)
                return ResolveExistingItemEntry({ 39208, 40714 }); // Sigil of the Dark Rider / Unbroken Knight
            return 0;
        case BOT_CLASS_PALADIN:
            if (slot == BOT_SLOT_MAINHAND)
                return ResolveExistingItemEntry({ HEIRLOOM_2H_SWORD });
            if (slot == BOT_SLOT_RANGED)
                return ResolveExistingItemEntry({ 18334, 22401, 23006 }); // Libram of Protection / Hope / Light
            return 0;
        case BOT_CLASS_HUNTER:
        case BOT_CLASS_DARK_RANGER:
            if (slot == BOT_SLOT_RANGED)
                return ResolveExistingItemEntry({ HEIRLOOM_BOW, 2506 }); // heirloom bow, Hornwood Recurve Bow
            if (slot == BOT_SLOT_MAINHAND)
                return ResolveExistingItemEntry({ HEIRLOOM_1H_SWORD });
            return 0;
        case BOT_CLASS_ROGUE:
            if (slot <= BOT_SLOT_OFFHAND)
                return ResolveExistingItemEntry({ HEIRLOOM_DAGGER });
            if (slot == BOT_SLOT_RANGED)
                return ResolveExistingItemEntry({ 25877, 3131, 3107 }); // throwing weapons
            return 0;
        case BOT_CLASS_MAGE:
        case BOT_CLASS_PRIEST:
        case BOT_CLASS_WARLOCK:
        case BOT_CLASS_ARCHMAGE:
        case BOT_CLASS_NECROMANCER:
        case BOT_CLASS_SEA_WITCH:
            if (slot == BOT_SLOT_MAINHAND)
                return ResolveExistingItemEntry({ HEIRLOOM_STAFF });
            if (slot == BOT_SLOT_RANGED)
                return ResolveExistingItemEntry({ 11287, 2506, 5210 }); // wands
            return 0;
        case BOT_CLASS_SHAMAN:
            if (slot == BOT_SLOT_MAINHAND)
                return ResolveExistingItemEntry({ HEIRLOOM_2H_AXE });
            if (slot == BOT_SLOT_RANGED)
                return ResolveExistingItemEntry({ 5175, 5177 }); // Earth / Water Totem
            return 0;
        case BOT_CLASS_DRUID:
            if (slot == BOT_SLOT_MAINHAND)
                return ResolveExistingItemEntry({ HEIRLOOM_STAFF });
            if (slot == BOT_SLOT_OFFHAND)
                return ResolveExistingItemEntry({ HEIRLOOM_DAGGER });
            if (slot == BOT_SLOT_RANGED)
                return ResolveExistingItemEntry({ 25667 }); // Idol (relic)
            return 0;
        case BOT_CLASS_BM:
        case BOT_CLASS_SPHYNX:
        case BOT_CLASS_DREADLORD:
        case BOT_CLASS_SPELLBREAKER:
        case BOT_CLASS_CRYPT_LORD:
            if (slot == BOT_SLOT_MAINHAND)
                return ResolveExistingItemEntry({ HEIRLOOM_2H_SWORD });
            if (slot == BOT_SLOT_OFFHAND)
                return ResolveExistingItemEntry({ HEIRLOOM_1H_SWORD });
            return 0;
        default:
            if (slot == BOT_SLOT_MAINHAND)
            {
                if (IsCastingClass(botclass))
                    return ResolveExistingItemEntry({ HEIRLOOM_STAFF });
                return ResolveExistingItemEntry({ HEIRLOOM_1H_SWORD });
            }
            return 0;
    }
}

Item* BotDataMgr::TryCreateWanderingBotEquipItem(uint32 entry)
{
    if (!entry || !sObjectMgr->GetItemTemplate(entry))
        return nullptr;
    return Item::CreateItem(entry, 1, nullptr);
}

static bool IsRelicItemTemplate(ItemTemplate const* proto)
{
    return proto && proto->Class == ITEM_CLASS_ARMOR && (
        proto->InventoryType == INVTYPE_RELIC ||
        proto->SubClass == ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_LIBRAM ||
        proto->SubClass == ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_IDOL ||
        proto->SubClass == ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_TOTEM ||
        proto->SubClass == ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_SIGIL);
}

Item* BotDataMgr::GenerateWanderingBotRelicItem(uint8 category, uint8 botclass, uint8 level, uint32 maxitemlevel)
{
    ASSERT(botclass < BOT_CLASS_END);
    ASSERT(level <= DEFAULT_MAX_LEVEL + 4);

    ItemLeveledArr const& slot_items = _botsExtraCreatureSortedGear[category][botclass][BOT_SLOT_RANGED];
    uint8 const startStep = std::min<uint8>(level / ITEM_SORTING_LEVEL_STEP, LEVEL_STEPS - 1);

    for (int32 step = int32(startStep); step >= 0; --step)
    {
        ItemIdVector const& item_id_vec = slot_items[step];
        if (item_id_vec.empty())
            continue;

        ItemIdVector try_ids;
        try_ids.reserve(item_id_vec.size());
        for (uint32 iid : item_id_vec)
        {
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(iid);
            if (!proto)
                continue;
            if (proto->RequiredLevel > level)
                continue;
            if (maxitemlevel && proto->ItemLevel > maxitemlevel)
                continue;
            try_ids.push_back(iid);
        }

        if (try_ids.empty())
            continue;

        Bcore::Containers::RandomShuffle(try_ids);
        for (uint32 item_id : try_ids)
        {
            if (Item* new_item = TryCreateWanderingBotEquipItem(item_id))
                return new_item;
        }
    }

    if (uint32 const fallbackEntry = GetWandererHeirloomWeaponFallback(botclass, BOT_SLOT_RANGED))
        return TryCreateWanderingBotEquipItem(fallbackEntry);

    return nullptr;
}

Item* BotDataMgr::GenerateWanderingBotItem(uint8 category, uint8 slot, uint8 botclass, uint8 level, uint32 maxitemlevel, std::function<bool(uint8, ItemTemplate const*)> const& check)
{
    ASSERT(slot < BOT_INVENTORY_SIZE);
    ASSERT(botclass < BOT_CLASS_END);
    ASSERT(level <= DEFAULT_MAX_LEVEL + 4);

    auto const& slot_items = _botsExtraCreatureSortedGear[category][botclass][slot];
    ItemIdVector valid_ids;
    valid_ids.reserve(slot_items.back().size());
    const std::array ilevels_to_check{ maxitemlevel, decltype(maxitemlevel){} };
    for (auto i : NPCBots::index_array<size_t, std::size(ilevels_to_check)>)
    {
        const uint32 max_item_lvl = ilevels_to_check[i];

        if (i > 0 && (!valid_ids.empty() || ilevels_to_check[0] == max_item_lvl))
            break;

        // level 1-5 must use step 0; old loop (lvl > ITEM_SORTING_LEVEL_STEP) skipped all low-level bots
        uint8 const startStep = std::min<uint8>(level / ITEM_SORTING_LEVEL_STEP, LEVEL_STEPS - 1);
        for (int32 step = int32(startStep); step >= 0; --step)
        {
            valid_ids.clear();
            ItemIdVector const& item_id_vec = slot_items[step];
            if (item_id_vec.empty())
                continue;

            for (uint32 iid : item_id_vec)
            {
                ItemTemplate const* proto = sObjectMgr->GetItemTemplate(iid);
                if (!proto)
                    continue;
                if (proto->RequiredLevel > level)
                    continue;
                if ((!max_item_lvl || proto->ItemLevel <= max_item_lvl) && check(slot, proto))
                    valid_ids.push_back(iid);
            }

            if (valid_ids.empty())
                continue;

            Bcore::Containers::RandomShuffle(valid_ids);
            for (uint32 item_id : valid_ids)
            {
                if (Item* new_item = Item::CreateItem(item_id, 1, nullptr))
                {
                    if (const uint32 randomPropertyId = GenerateItemRandomPropertyId(item_id))
                        new_item->SetItemRandomProperties(randomPropertyId);
                    return new_item;
                }
            }
        }
    }

    return nullptr;
}

bool BotDataMgr::GenerateWanderingBotItemEnchants(Item* item, uint8 slot, uint8 spec)
{
    bool result = false;

    switch (slot)
    {
        case BOT_SLOT_BODY:
        case BOT_SLOT_TRINKET1:
        case BOT_SLOT_TRINKET2:
            return result;
        default:
            break;
    }

    ItemTemplate const* proto = item->GetTemplate();

    if (proto->RequiredLevel < 60)
        return result;

    static const auto is_enchantable = [](ItemTemplate const* p, SpellInfo const* s) {
        SpellEffectInfo const& e = s->GetEffect(EFFECT_0);
        return e.Effect == SPELL_EFFECT_ENCHANT_ITEM && s->EquippedItemClass == int32(p->Class) && s->BaseLevel <= p->RequiredLevel && e.MiscValue > 0 &&
            (s->EquippedItemClass == ITEM_CLASS_WEAPON ? !!(s->EquippedItemSubClassMask & (1u << p->SubClass)) : !!(s->EquippedItemInventoryTypeMask & (1u << p->InventoryType))) &&
            sSpellItemEnchantmentStore.LookupEntry(uint32(e.MiscValue));
    };

    static const std::array<uint32, 10> weapon_enchants_dk{ 53323, 53331, 53341, 53342, 53343, 53344, 53346, 53347, 62158, 70164 }; //2h only
    static const std::array<uint32, 11> weapon_enchants_caster{ 27968, 27975, 28003, 34010, 44510, 44629, 59619, 59625, 60714, 62948, 62959 };
    static const std::array<uint32, 18> weapon_enchants_melee{ 27971, 27977, 27984, 28004, 42620, 42974, 44524, 44576, 44630, 44633, 46578, 55836, 59619, 59621, 60621, 60691, 60707, 62257 };
    static const std::array<uint32, 34> armor_enchants_caster{ 34003, 34008, 44383, 44488, 44492, 44528, 44555, 44582, 44592, 44612, 44616, 44623, 44635, 47898, 47900, 47901, 57690, 57691, 59636, 59784, 59970, 60609, 60653, 60692, 60767, 61120, 61271, 62256, 60583, 50911, 55016, 55634, 55642, 56034 };
    static const std::array<uint32, 40> armor_enchants_melee{ 34007, 34008, 34009, 44383, 44484, 44488, 44492, 44500, 44513, 44528, 44529, 44575, 44589, 44598, 44612, 44616, 44623, 47898, 47900, 47901, 59777, 59954, 60606, 60609, 60616, 60623, 60663, 60668, 60692, 60763, 61271, 62256, 50903, 50911, 55016, 55777, 57690, 61117, 62201, 59636 };

    //enchants
    SpellInfo const* sInfo = nullptr;
    std::vector<uint32> valid_enchant_ids;
    valid_enchant_ids.reserve(1ull << 6);
    switch (spec)
    {
        case BOT_SPEC_PALADIN_HOLY:
        case BOT_SPEC_PRIEST_DISCIPLINE:
        case BOT_SPEC_PRIEST_HOLY:
        case BOT_SPEC_PRIEST_SHADOW:
        case BOT_SPEC_SHAMAN_ELEMENTAL:
        case BOT_SPEC_SHAMAN_RESTORATION:
        case BOT_SPEC_MAGE_ARCANE:
        case BOT_SPEC_MAGE_FIRE:
        case BOT_SPEC_MAGE_FROST:
        case BOT_SPEC_WARLOCK_AFFLICTION:
        case BOT_SPEC_WARLOCK_DEMONOLOGY:
        case BOT_SPEC_WARLOCK_DESTRUCTION:
        case BOT_SPEC_DRUID_BALANCE:
        case BOT_SPEC_DRUID_RESTORATION:
            switch (proto->Class)
            {
                case ITEM_CLASS_WEAPON:
                    for (uint32 spellId : weapon_enchants_caster)
                    {
                        sInfo = sSpellMgr->AssertSpellInfo(spellId);
                        if (is_enchantable(proto, sInfo))
                            valid_enchant_ids.push_back(uint32(sInfo->GetEffect(EFFECT_0).MiscValue));
                    }
                    break;
                case ITEM_CLASS_ARMOR:
                    for (uint32 spellId : armor_enchants_caster)
                    {
                        sInfo = sSpellMgr->AssertSpellInfo(spellId);
                        if (is_enchantable(proto, sInfo))
                            valid_enchant_ids.push_back(uint32(sInfo->GetEffect(EFFECT_0).MiscValue));
                    }
                    break;
                default:
                    break;
            }
            break;
        case BOT_SPEC_DK_BLOOD:
        case BOT_SPEC_DK_FROST:
        case BOT_SPEC_DK_UNHOLY:
            switch (proto->Class)
            {
                case ITEM_CLASS_WEAPON:
                    for (uint32 spellId : weapon_enchants_dk)
                    {
                        sInfo = sSpellMgr->AssertSpellInfo(spellId);
                        if (is_enchantable(proto, sInfo))
                            valid_enchant_ids.push_back(uint32(sInfo->GetEffect(EFFECT_0).MiscValue));
                    }
                    break;
                default:
                    break;
            }
        [[fallthrough]];
        case BOT_SPEC_WARRIOR_ARMS:
        case BOT_SPEC_WARRIOR_FURY:
        case BOT_SPEC_WARRIOR_PROTECTION:
        case BOT_SPEC_PALADIN_PROTECTION:
        case BOT_SPEC_PALADIN_RETRIBUTION:
        case BOT_SPEC_HUNTER_BEASTMASTERY:
        case BOT_SPEC_HUNTER_MARKSMANSHIP:
        case BOT_SPEC_HUNTER_SURVIVAL:
        case BOT_SPEC_ROGUE_ASSASINATION:
        case BOT_SPEC_ROGUE_COMBAT:
        case BOT_SPEC_ROGUE_SUBTLETY:
        case BOT_SPEC_SHAMAN_ENHANCEMENT:
        case BOT_SPEC_DRUID_FERAL:
            switch (proto->Class)
            {
                case ITEM_CLASS_WEAPON:
                    for (uint32 spellId : weapon_enchants_melee)
                    {
                        sInfo = sSpellMgr->AssertSpellInfo(spellId);
                        if (is_enchantable(proto, sInfo))
                            valid_enchant_ids.push_back(uint32(sInfo->GetEffect(EFFECT_0).MiscValue));
                    }
                    break;
                case ITEM_CLASS_ARMOR:
                    for (uint32 spellId : armor_enchants_melee)
                    {
                        sInfo = sSpellMgr->AssertSpellInfo(spellId);
                        if (is_enchantable(proto, sInfo))
                            valid_enchant_ids.push_back(uint32(sInfo->GetEffect(EFFECT_0).MiscValue));
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    uint32 enchant_id;
    enchant_id = valid_enchant_ids.empty() ? 0 : valid_enchant_ids.size() == 1u ? valid_enchant_ids.front() : Bcore::Containers::SelectRandomContainerElement(valid_enchant_ids);
    if (enchant_id)
    {
        item->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + PERM_ENCHANTMENT_SLOT*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET, enchant_id);
        item->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + PERM_ENCHANTMENT_SLOT*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET, 0);
        item->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + PERM_ENCHANTMENT_SLOT*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET, 0);
        result = true;
    }

    //gems
    constexpr std::array<uint32, 5> gems_caster{ 40132, 40135, 40123, 40127, 40128 };
    constexpr std::array<uint32, 6> gems_melee{ 40136, 40140, 40124, 40125, 40127, 40128 };

    for (auto i : NPCBots::index_array<uint8, MAX_ITEM_PROTO_SOCKETS>)
    {
        valid_enchant_ids.clear();
        switch (spec)
        {
            case BOT_SPEC_PALADIN_HOLY:
            case BOT_SPEC_PRIEST_DISCIPLINE:
            case BOT_SPEC_PRIEST_HOLY:
            case BOT_SPEC_PRIEST_SHADOW:
            case BOT_SPEC_SHAMAN_ELEMENTAL:
            case BOT_SPEC_SHAMAN_RESTORATION:
            case BOT_SPEC_MAGE_ARCANE:
            case BOT_SPEC_MAGE_FIRE:
            case BOT_SPEC_MAGE_FROST:
            case BOT_SPEC_WARLOCK_AFFLICTION:
            case BOT_SPEC_WARLOCK_DEMONOLOGY:
            case BOT_SPEC_WARLOCK_DESTRUCTION:
            case BOT_SPEC_DRUID_BALANCE:
            case BOT_SPEC_DRUID_RESTORATION:
                for (uint32 gId : gems_caster)
                {
                    GemPropertiesEntry const* gprops = sGemPropertiesStore.LookupEntry(sObjectMgr->GetItemTemplate(gId)->GemProperties);
                    if (gprops->Type & proto->Socket[i].Color)
                        valid_enchant_ids.push_back(gprops->EnchantID);
                }
                break;
            case BOT_SPEC_DK_BLOOD:
            case BOT_SPEC_DK_FROST:
            case BOT_SPEC_DK_UNHOLY:
            case BOT_SPEC_WARRIOR_ARMS:
            case BOT_SPEC_WARRIOR_FURY:
            case BOT_SPEC_WARRIOR_PROTECTION:
            case BOT_SPEC_PALADIN_PROTECTION:
            case BOT_SPEC_PALADIN_RETRIBUTION:
            case BOT_SPEC_HUNTER_BEASTMASTERY:
            case BOT_SPEC_HUNTER_MARKSMANSHIP:
            case BOT_SPEC_HUNTER_SURVIVAL:
            case BOT_SPEC_ROGUE_ASSASINATION:
            case BOT_SPEC_ROGUE_COMBAT:
            case BOT_SPEC_ROGUE_SUBTLETY:
            case BOT_SPEC_SHAMAN_ENHANCEMENT:
            case BOT_SPEC_DRUID_FERAL:
                for (uint32 gId : gems_melee)
                {
                    GemPropertiesEntry const* gprops = sGemPropertiesStore.LookupEntry(sObjectMgr->GetItemTemplate(gId)->GemProperties);
                    if (gprops->Type & proto->Socket[i].Color)
                        valid_enchant_ids.push_back(gprops->EnchantID);
                }
                break;
            default:
                break;
        }

        enchant_id = valid_enchant_ids.empty() ? 0 : valid_enchant_ids.size() == 1u ? valid_enchant_ids.front() : Bcore::Containers::SelectRandomContainerElement(valid_enchant_ids);
        if (enchant_id)
        {
            item->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + (uint8(SOCK_ENCHANTMENT_SLOT) + i)*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET, enchant_id);
            item->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + (uint8(SOCK_ENCHANTMENT_SLOT) + i)*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET, 0);
            item->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + (uint8(SOCK_ENCHANTMENT_SLOT) + i)*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET, 0);
            result = true;
        }
    }

    return result;
}

CreatureTemplate const* BotDataMgr::GetBotExtraCreatureTemplate(uint32 entry)
{
    CreatureTemplateContainer::const_iterator cit = _botsExtraCreatureTemplates.find(entry);
    return cit == _botsExtraCreatureTemplates.cend() ? nullptr : &cit->second;
}

EquipmentInfo const* BotDataMgr::GetBotEquipmentInfo(uint32 entry)
{
    decltype(_botsExtraCreatureEquipmentTemplates)::const_iterator cit = _botsExtraCreatureEquipmentTemplates.find(entry);
    if (cit == _botsExtraCreatureEquipmentTemplates.cend())
    {
        int8 eqId = 1;
        return sObjectMgr->GetEquipmentInfo(entry, eqId);
    }
    else
        return cit->second;
}

void BotDataMgr::AddNpcBotData(uint32 entry, uint32 roles, uint8 spec, uint32 faction)
{
    if (!_botsData.contains(entry))
    {
        _botsData.emplace(std::piecewise_construct, std::forward_as_tuple(entry), std::forward_as_tuple(roles, faction, spec));

        CharacterDatabasePreparedStatement* bstmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_NPCBOT);
        //"INSERT INTO characters_npcbot (entry, roles, spec, faction) VALUES (?, ?, ?, ?)", CONNECTION_ASYNC);
        bstmt->setUInt32(0, entry);
        bstmt->setUInt32(1, roles);
        bstmt->setUInt8(2, spec);
        bstmt->setUInt32(3, faction);
        bstmt->setUInt8(4, NPCBOT_HIRE_NORMAL);
        CharacterDatabase.Execute(bstmt);

        return;
    }

    BOT_LOG_ERROR("sql.sql", "BotDataMgr::AddNpcBotData(): trying to add new data but entry already exists! entry = {}", entry);
}

// By leewheel 20260523
uint8 BotDataMgr::GetNpcBotHireSource(uint32 entry)
{
    NpcBotData const* data = SelectNpcBotData(entry);
    return data ? data->hire_source : NPCBOT_HIRE_NORMAL;
}

bool BotDataMgr::IsArenaBotLockedForOwner(uint32 botEntry, uint32 ownerGuidLow)
{
    return IsArenaBotLockedForOwnerImpl(botEntry, ownerGuidLow);
}

void BotDataMgr::SetNpcBotHireSource(uint32 entry, uint8 hireSource)
{
    UpdateNpcBotData(entry, NPCBOT_UPDATE_HIRE_SOURCE, &hireSource);
}

Creature* BotDataMgr::FindFreeHireBotForQuickGroup(Player const* player, uint8 botClass, std::set<uint8> const& usedClasses)
{
    if (!player || !BotCfg::IsClassEnabled(botClass) || usedClasses.count(botClass))
        return nullptr;

    std::vector<uint32> candidates;
    candidates.reserve(32);

    for (auto const& [entry, data] : _botsData)
    {
        if (data.owner != 0)
            continue;

        if (entry == BOT_GIVER_ENTRY || entry == BOT_ENTRY_MIRROR_IMAGE_BM)
            continue;

        NpcBotExtras const* extras = SelectNpcBotExtras(entry);
        if (!extras || extras->bclass != botClass)
            continue;

        if (BotCfg::FilterRaces())
        {
            uint32 const botFaction = GetDefaultFactionForBotRaceClass(extras->bclass, extras->race);
            if (GetTeamForFaction(botFaction) != player->GetTeam())
                continue;
        }

        if (GetMinLevelForBotClass(botClass) > player->GetLevel())
            continue;

        if (IsArenaBotLockedForOwner(entry, player->GetGUID().GetCounter()))
            continue;

        Creature const* bot = FindBot(entry);
        if (!bot || bot->IsWandererBot() || !bot->IsFreeBot())
            continue;

        bot_ai const* ai = bot->GetBotAI();
        if (!ai || !ai->IAmFree())
            continue;

        candidates.push_back(entry);
    }

    if (candidates.empty())
        return nullptr;

    uint32 const pick = candidates[urand(0, uint32(candidates.size() - 1))];
    return const_cast<Creature*>(FindBot(pick));
}
// end By leewheel 20260523

NpcBotData const* BotDataMgr::SelectNpcBotData(uint32 entry)
{
    NpcBotDataMap::const_iterator itr = _botsData.find(entry);
    return itr != _botsData.cend() ? &itr->second : nullptr;
}
void BotDataMgr::UpdateNpcBotData(uint32 entry, NpcBotDataUpdateType updateType, void* data)
{
    NpcBotDataMap::iterator itr = _botsData.find(entry);
    if (itr == _botsData.end())
        return;

    CharacterDatabasePreparedStatement* bstmt;
    switch (updateType)
    {
        case NPCBOT_UPDATE_OWNER:
        {
            if (itr->second.owner == *(uint32*)(data))
                break;
            itr->second.owner = *(uint32*)(data);
            itr->second.hire_time = itr->second.owner ? uint64(GameTime::GetGameTime()) : 1ULL;
            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NPCBOT_OWNER);
            //"UPDATE characters_npcbot SET owner = ?, hire_time = FROM_UNIXTIME(?) WHERE entry = ?", CONNECTION_ASYNC
            bstmt->setUInt32(0, itr->second.owner);
            bstmt->setUInt64(1, itr->second.hire_time);
            bstmt->setUInt32(2, entry);
            CharacterDatabase.Execute(bstmt);
            //break; //no break: erase transmogs
        }
        [[fallthrough]];
        case NPCBOT_UPDATE_TRANSMOG_ERASE:
            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_NPCBOT_TRANSMOG);
            //"DELETE FROM characters_npcbot_transmog WHERE entry = ?", CONNECTION_ASYNC
            bstmt->setUInt32(0, entry);
            CharacterDatabase.Execute(bstmt);
            break;
        case NPCBOT_UPDATE_ROLES:
            itr->second.roles = *(uint32*)(data);
            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NPCBOT_ROLES);
            //"UPDATE character_npcbot SET roles = ? WHERE entry = ?", CONNECTION_ASYNC
            bstmt->setUInt32(0, itr->second.roles);
            bstmt->setUInt32(1, entry);
            CharacterDatabase.Execute(bstmt);
            break;
        case NPCBOT_UPDATE_SPEC:
            itr->second.spec = *(uint8*)(data);
            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NPCBOT_SPEC);
            //"UPDATE characters_npcbot SET spec = ? WHERE entry = ?", CONNECTION_ASYNCH
            bstmt->setUInt8(0, itr->second.spec);
            bstmt->setUInt32(1, entry);
            CharacterDatabase.Execute(bstmt);
            break;
        case NPCBOT_UPDATE_FACTION:
            itr->second.faction = *(uint32*)(data);
            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NPCBOT_FACTION);
            //"UPDATE characters_npcbot SET faction = ? WHERE entry = ?", CONNECTION_ASYNCH
            bstmt->setUInt32(0, itr->second.faction);
            bstmt->setUInt32(1, entry);
            CharacterDatabase.Execute(bstmt);
            break;
        case NPCBOT_UPDATE_SHARED_OWNERS:
        {
            NpcBotData::SharedOwnersContainer const* shared_owners = (NpcBotData::SharedOwnersContainer const*)(data);

            if (std::addressof(itr->second.shared_owners) != shared_owners)
                itr->second.shared_owners = *shared_owners;

            std::vector shared_owners_v(itr->second.shared_owners.cbegin(), itr->second.shared_owners.cend());
            std::ranges::sort(shared_owners_v);
            std::ostringstream ss;
            for (uint32 guid_low : shared_owners_v)
                ss << guid_low << ' ';

            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NPCBOT_SHARED_OWNERS);
            //"UPDATE characters_npcbot SET shared_owners = ? WHERE entry = ?", CONNECTION_ASYNC
            bstmt->setStringView(0, ss.view());
            bstmt->setUInt32(1, entry);
            CharacterDatabase.Execute(bstmt);
            break;
        }
        case NPCBOT_UPDATE_DISABLED_SPELLS:
        {
            NpcBotData::DisabledSpellsContainer const* spells = (NpcBotData::DisabledSpellsContainer const*)(data);
            std::ostringstream ss;
            for (uint32 spellId : *spells)
                ss << spellId << ' ';

            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NPCBOT_DISABLED_SPELLS);
            //"UPDATE characters_npcbot SET spells_disabled = ? WHERE entry = ?", CONNECTION_ASYNCH
            bstmt->setStringView(0, ss.view());
            bstmt->setUInt32(1, entry);
            CharacterDatabase.Execute(bstmt);
            break;
        }
        case NPCBOT_UPDATE_MISCVALUES:
        {
            NpcBotData::MiscValuesContainer const* miscvals = (NpcBotData::MiscValuesContainer const*)(data);
            std::ostringstream ss;
            for (auto [misc_type, misc_val] : *miscvals)
                ss << misc_type << ':' << misc_val << ' ';

            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NPCBOT_MISCVALUES);
            //"UPDATE characters_npcbot SET miscvalues = ? WHERE entry = ?", CONNECTION_ASYNCH
            bstmt->setStringView(0, ss.view());
            bstmt->setUInt32(1, entry);
            CharacterDatabase.Execute(bstmt);
            break;
        }
        case NPCBOT_UPDATE_HIRE_SOURCE: // By leewheel 20260523
            itr->second.hire_source = *(uint8*)(data);
            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NPCBOT_HIRE_SOURCE);
            bstmt->setUInt8(0, itr->second.hire_source);
            bstmt->setUInt32(1, entry);
            CharacterDatabase.Execute(bstmt);
            break;
        case NPCBOT_UPDATE_EQUIPS:
        {
            Item** items = (Item**)(data);

            // By leewheel 20260520 - service bots: in-memory only (no item_instance spam)
            if (IsServiceHireSource(itr->second.hire_source))
            {
                for (uint8 k = BOT_SLOT_MAINHAND; k != BOT_INVENTORY_SIZE; ++k)
                    itr->second.equips[k] = items[k] ? items[k]->GetGUID().GetCounter() : 0;
                break;
            }

            EquipmentInfo const* einfo = BotDataMgr::GetBotEquipmentInfo(entry);

            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NPCBOT_EQUIP);
            //"UPDATE character_npcbot SET equipMhEx = ?, equipOhEx = ?, equipRhEx = ?, equipHead = ?, equipShoulders = ?, equipChest = ?, equipWaist = ?, equipLegs = ?,
            //equipFeet = ?, equipWrist = ?, equipHands = ?, equipBack = ?, equipBody = ?, equipFinger1 = ?, equipFinger2 = ?, equipTrinket1 = ?, equipTrinket2 = ?, equipNeck = ? WHERE entry = ?", CONNECTION_ASYNC
            CharacterDatabasePreparedStatement* stmt;
            uint8 k;
            for (k = BOT_SLOT_MAINHAND; k != BOT_INVENTORY_SIZE; ++k)
            {
                itr->second.equips[k] = items[k] ? items[k]->GetGUID().GetCounter() : 0;
                if (Item const* botitem = items[k])
                {
                    bool standard = false;
                    for (auto i : NPCBots::index_array<uint8, MAX_EQUIPMENT_ITEMS>)
                    {
                        if (einfo->ItemEntry[i] == botitem->GetEntry())
                        {
                            itr->second.equips[k] = 0;
                            bstmt->setUInt32(k, 0);
                            standard = true;
                            break;
                        }
                    }
                    if (standard)
                        continue;

                    uint8 index = 0;
                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_ITEM_INSTANCE);
                    //REPLACE INTO item_instance (itemEntry, owner_guid, creatorGuid, giftCreatorGuid, count, duration, charges, flags, enchantments, randomPropertyId, durability, playedTime, text, guid)
                    //VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", CONNECTION_ASYNC : 0-13
                    stmt->setUInt32(  index, botitem->GetEntry());
                    stmt->setUInt32(++index, botitem->GetOwnerGUID().GetCounter());
                    stmt->setUInt32(++index, botitem->GetGuidValue(ITEM_FIELD_CREATOR).GetCounter());
                    stmt->setUInt32(++index, botitem->GetGuidValue(ITEM_FIELD_GIFTCREATOR).GetCounter());
                    stmt->setUInt32(++index, botitem->GetCount());
                    stmt->setUInt32(++index, botitem->GetUInt32Value(ITEM_FIELD_DURATION));

                    std::ostringstream ssSpells;
                    for (auto i : NPCBots::index_array<uint8, MAX_ITEM_PROTO_SPELLS>)
                        ssSpells << botitem->GetSpellCharges(i) << ' ';
                    stmt->setString(++index, ssSpells.str());

                    stmt->setUInt32(++index, botitem->GetUInt32Value(ITEM_FIELD_FLAGS));

                    std::ostringstream ssEnchants;
                    for (auto i : NPCBots::index_array<uint8, MAX_ENCHANTMENT_SLOT>)
                    {
                        ssEnchants << botitem->GetEnchantmentId(EnchantmentSlot(i)) << ' ';
                        ssEnchants << botitem->GetEnchantmentDuration(EnchantmentSlot(i)) << ' ';
                        ssEnchants << botitem->GetEnchantmentCharges(EnchantmentSlot(i)) << ' ';
                    }
                    stmt->setString(++index, ssEnchants.str());

                    stmt->setInt16 (++index, botitem->GetItemRandomPropertyId());
                    stmt->setUInt16(++index, botitem->GetUInt32Value(ITEM_FIELD_DURABILITY));
                    stmt->setUInt32(++index, botitem->GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME));
                    stmt->setString(++index, botitem->GetText());
                    stmt->setUInt32(++index, botitem->GetGUID().GetCounter());

                    trans->Append(stmt);

                    Item::DeleteFromInventoryDB(trans, botitem->GetGUID().GetCounter()); //prevent duplicates

                    bstmt->setUInt32(k, botitem->GetGUID().GetCounter());
                }
                else
                    bstmt->setUInt32(k, uint32(0));
            }

            bstmt->setUInt32(k, entry);
            trans->Append(bstmt);
            CharacterDatabase.CommitTransaction(trans);
            break;
        }
        case NPCBOT_UPDATE_ERASE:
        {
            NpcBotDataMap::iterator bitr = _botsData.find(entry);
            ASSERT(bitr != _botsData.end());
            _botsData.erase(bitr);
            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_NPCBOT);
            //"DELETE FROM characters_npcbot WHERE entry = ?", CONNECTION_ASYNC
            bstmt->setUInt32(0, entry);
            CharacterDatabase.Execute(bstmt);
            break;
        }
        default:
            BOT_LOG_ERROR("sql.sql", "BotDataMgr:UpdateNpcBotData: unhandled updateType {}", uint32(updateType));
            break;
    }
}
void BotDataMgr::UpdateNpcBotDataAll(uint32 playerGuid, NpcBotDataUpdateType updateType, void* data)
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    CharacterDatabasePreparedStatement* bstmt;
    uint32 newowner = *(uint32*)(data);
    switch (updateType)
    {
        case NPCBOT_UPDATE_OWNER:
            ASSERT(newowner == 0);
            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NPCBOT_EQUIP_RESET_ALL);
            //"UPDATE characters_npcbot SET equipMhEx = 0, equipOhEx = 0, equipRhEx = 0, equipHead = 0, equipShoulders = 0, equipChest = 0, equipWaist = 0, equipLegs = 0, equipFeet = 0, "
            //"equipWrist = 0, equipHands = 0, equipBack = 0, equipBody = 0, equipFinger1 = 0, equipFinger2 = 0, equipTrinket1 = 0, equipTrinket2 = 0, equipNeck = 0 WHERE owner = ?", CONNECTION_ASYNC
            bstmt->setUInt32(0, playerGuid);
            trans->Append(bstmt);
            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_NPCBOT_TRANSMOG_ALL);
            //"DELETE FROM characters_npcbot_transmog WHERE entry IN (SELECT entry FROM characters_npcbot WHERE owner = ?)", CONNECTION_ASYNC
            bstmt->setUInt32(0, playerGuid);
            trans->Append(bstmt);
            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NPCBOT_SHARED_OWNERS_ALL);
            //"UPDATE characters_npcbot SET shared_owners = NULL WHERE owner = ?", CONNECTION_ASYNC
            bstmt->setUInt32(0, playerGuid);
            trans->Append(bstmt);
            bstmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_NPCBOT_OWNER_ALL);
            //"UPDATE characters_npcbot SET owner = ?, hire_time = FROM_UNIXTIME(?) WHERE owner = ?", CONNECTION_ASYNC
            bstmt->setUInt32(0, newowner);
            bstmt->setUInt64(1, uint64(1ULL));
            bstmt->setUInt32(2, playerGuid);
            trans->Append(bstmt);
            break;
        default:
            BOT_LOG_ERROR("sql.sql", "BotDataMgr:UpdateNpcBotDataAll: unhandled updateType {}", uint32(updateType));
            break;
    }

    if (trans->GetSize() > 0)
        CharacterDatabase.CommitTransaction(trans);
}

void BotDataMgr::SaveNpcBotStats(NpcBotStats const& stats)
{
    CharacterDatabasePreparedStatement* bstmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_NPCBOT_STATS);
    //"REPLACE INTO characters_npcbot_stats
    //(entry, maxhealth, maxpower, strength, agility, stamina, intellect, spirit, armor, defense,
    //resHoly, resFire, resNature, resFrost, resShadow, resArcane, blockPct, dodgePct, parryPct, critPct,
    //attackPower, spellPower, spellPen, hastePct, hitBonusPct, expertise, armorPenPct) VALUES
    //(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", CONNECTION_ASYNC

    uint32 index = 0;
    bstmt->setUInt32(  index, stats.entry);
    bstmt->setUInt32(++index, stats.maxhealth);
    bstmt->setUInt32(++index, stats.maxpower);
    bstmt->setUInt32(++index, stats.strength);
    bstmt->setUInt32(++index, stats.agility);
    bstmt->setUInt32(++index, stats.stamina);
    bstmt->setUInt32(++index, stats.intellect);
    bstmt->setUInt32(++index, stats.spirit);
    bstmt->setUInt32(++index, stats.armor);
    bstmt->setUInt32(++index, stats.defense);
    bstmt->setUInt32(++index, stats.resHoly);
    bstmt->setUInt32(++index, stats.resFire);
    bstmt->setUInt32(++index, stats.resNature);
    bstmt->setUInt32(++index, stats.resFrost);
    bstmt->setUInt32(++index, stats.resShadow);
    bstmt->setUInt32(++index, stats.resArcane);
    bstmt->setFloat (++index, stats.blockPct);
    bstmt->setFloat (++index, stats.dodgePct);
    bstmt->setFloat (++index, stats.parryPct);
    bstmt->setFloat (++index, stats.critPct);
    bstmt->setUInt32(++index, stats.attackPower);
    bstmt->setUInt32(++index, stats.spellPower);
    bstmt->setUInt32(++index, stats.spellPen);
    bstmt->setFloat (++index, stats.hastePct);
    bstmt->setFloat (++index, stats.hitBonusPct);
    bstmt->setUInt32(++index, stats.expertise);
    bstmt->setFloat (++index, stats.armorPenPct);

    CharacterDatabase.Execute(bstmt);
}

NpcBotAppearanceData const* BotDataMgr::SelectNpcBotAppearance(uint32 entry)
{
    NpcBotAppearanceDataMap::const_iterator itr = _botsAppearanceData.find(entry);
    return itr != _botsAppearanceData.cend() ? &itr->second : nullptr;
}

std::string_view BotDataMgr::GetNpcBotAppearanceName(uint32 entry)
{
    if (NpcBotAppearanceData const* appData = SelectNpcBotAppearance(entry))
        if (!appData->name.empty() && appData->name != "unk")
            return appData->name;

    return {};
}

std::string BotDataMgr::GetNpcBotDisplayName(uint32 entry, LocaleConstant locale)
{
    CreatureTemplate const* creatureTemplate = sObjectMgr->GetCreatureTemplate(entry);
    if (!creatureTemplate)
        return {};

    std::string_view creatureName = creatureTemplate->Name;

    // Runtime-generated wanderers/dungeon bots: appearance table name (see QueryHandler).
    if (CreatureTemplate const* extraTemplate = GetBotExtraCreatureTemplate(entry))
    {
        if (std::string_view appName = GetNpcBotAppearanceName(entry); !appName.empty())
            creatureName = appName;
        else if (uint32 const origEntry = extraTemplate->KillCredit[0])
            if (std::string_view origName = GetNpcBotAppearanceName(origEntry); !origName.empty())
                creatureName = origName;
    }
    else if (CreatureLocale const* creatureInfo = sObjectMgr->GetCreatureLocale(entry))
    {
        if (creatureInfo->Name.size() > locale && !creatureInfo->Name[locale].empty() && Utf8FitTo(creatureInfo->Name[locale], {}))
            creatureName = creatureInfo->Name[locale];
    }

    return std::string(creatureName);
}

NpcBotExtras const* BotDataMgr::SelectNpcBotExtras(uint32 entry)
{
    NpcBotExtrasMap::const_iterator itr = _botsExtras.find(entry);
    return itr != _botsExtras.cend() ? &itr->second : nullptr;
}

NpcBotTransmogData const* BotDataMgr::SelectNpcBotTransmogs(uint32 entry)
{
    NpcBotTransmogDataMap::const_iterator itr = _botsTransmogData.find(entry);
    return itr != _botsTransmogData.cend() ? &itr->second : nullptr;
}
void BotDataMgr::UpdateNpcBotTransmogData(uint32 entry, uint8 slot, uint32 item_id, int32 fake_id, bool update_db)
{
    ASSERT(slot < BOT_TRANSMOG_INVENTORY_SIZE);

    _botsTransmogData.try_emplace(entry, NpcBotTransmogData{});
    _botsTransmogData.at(entry).transmogs[slot] = { item_id, fake_id };

    if (update_db)
    {
        CharacterDatabasePreparedStatement* bstmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_NPCBOT_TRANSMOG);
        //"REPLACE INTO characters_npcbot_transmog (entry, slot, item_id, fake_id) VALUES (?, ?, ?, ?)", CONNECTION_ASYNC
        bstmt->setUInt32(0, entry);
        bstmt->setUInt8(1, slot);
        bstmt->setUInt32(2, item_id);
        bstmt->setInt32(3, fake_id);
        CharacterDatabase.Execute(bstmt);
    }
}

void BotDataMgr::ResetNpcBotTransmogData(uint32 entry, bool update_db)
{
    NpcBotTransmogDataMap::iterator itr = _botsTransmogData.find(entry);
    if (itr == _botsTransmogData.end())
        return;

    auto& transmog_data = itr->second;

    if (update_db)
    {
        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        for (auto i : NPCBots::index_array<uint8, BOT_TRANSMOG_INVENTORY_SIZE>)
        {
            if (transmog_data.transmogs[i].first == 0 && transmog_data.transmogs[i].second == -1)
                continue;

            CharacterDatabasePreparedStatement* bstmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_NPCBOT_TRANSMOG);
            //"REPLACE INTO characters_npcbot_transmog (entry, slot, item_id, fake_id) VALUES (?, ?, ?, ?)", CONNECTION_ASYNC
            bstmt->setUInt32(0, entry);
            bstmt->setUInt8(1, i);
            bstmt->setUInt32(2, 0);
            bstmt->setInt32(3, -1);
            trans->Append(bstmt);
        }

        if (trans->GetSize() > 0)
            CharacterDatabase.CommitTransaction(trans);
    }

    for (auto i : NPCBots::index_array<uint8, BOT_TRANSMOG_INVENTORY_SIZE>)
        transmog_data.transmogs[i] = { 0, -1 };
}

void BotDataMgr::RegisterBot(Creature const* bot)
{
    std::unique_lock lock(*GetLock());

    if (_existingBots.contains(bot))
    {
        BOT_LOG_ERROR("entities.unit", "BotDataMgr::RegisterBot: bot {} ({}) already registered!", bot->GetEntry(), bot->GetName());
        return;
    }

    _existingBots.insert(bot);
    //BOT_LOG_ERROR("entities.unit", "BotDataMgr::RegisterBot: registered bot {} ({})", bot->GetEntry(), bot->GetName());
}
void BotDataMgr::UnregisterBot(Creature const* bot)
{
    std::unique_lock lock(*GetLock());

    if (!_existingBots.contains(bot))
    {
        BOT_LOG_ERROR("entities.unit", "BotDataMgr::UnregisterBot: bot {} ({}) not found!", bot->GetEntry(), bot->GetName());
        return;
    }

    _existingBots.erase(bot);

    if (auto ditr = _botsExtraCreaturesToDespawn.find(bot->GetEntry()); ditr != _botsExtraCreaturesToDespawn.cend())
    {
        sBotGen->CleanExtraBotData(bot);
        _botsExtraCreaturesToDespawn.erase(ditr);
    }

    //BOT_LOG_ERROR("entities.unit", "BotDataMgr::UnregisterBot: unregistered bot {} ({})", bot->GetEntry(), bot->GetName());
}
Creature const* BotDataMgr::FindBot(uint32 entry)
{
    std::shared_lock lock(*GetLock());

    auto it = std::ranges::find_if(_existingBots, [entry](Creature const* bot) { return bot->GetEntry() == entry; });
    return it != _existingBots.cend() ? *it : nullptr;
}
Creature const* BotDataMgr::FindBot(std::string_view name, LocaleConstant loc, std::vector<uint32> const* not_ids)
{
    std::wstring wname;
    if (Utf8toWStr(name, wname))
    {
        wstrToLower(wname);
        std::shared_lock lock(*GetLock());
        for (Creature const* bot : _existingBots)
        {
            if (not_ids && std::ranges::find(*not_ids, bot->GetEntry()) != not_ids->cend())
                continue;

            std::string_view basename = bot->GetName();
            if (CreatureLocale const* creatureInfo = sObjectMgr->GetCreatureLocale(bot->GetEntry()))
            {
                if (creatureInfo->Name.size() > loc && !creatureInfo->Name[loc].empty())
                    basename = creatureInfo->Name[loc];
            }

            std::wstring wbname;
            if (!Utf8toWStr(basename, wbname))
                continue;

            wstrToLower(wbname);
            if (wbname == wname)
                return bot;
        }
    }

    return nullptr;
}

NpcBotRegistry const& BotDataMgr::GetExistingNPCBots()
{
    return _existingBots;
}

void BotDataMgr::GetNPCBotGuidsByOwner(std::vector<ObjectGuid> &guids_vec, ObjectGuid owner_guid, bool count_shared)
{
    ASSERT(AllBotsLoaded());

    std::shared_lock lock(*GetLock());

    for (Creature const* bot : _existingBots)
    {
        if (_botsData.at(bot->GetEntry()).owner == owner_guid.GetCounter() || (count_shared && _botsData.at(bot->GetEntry()).shared_owners.contains(owner_guid.GetCounter())))
            guids_vec.push_back(bot->GetGUID());
    }
}

ObjectGuid BotDataMgr::GetNPCBotGuid(uint32 entry)
{
    ASSERT(AllBotsLoaded());

    std::shared_lock lock(*GetLock());

    for (Creature const* bot : _existingBots)
    {
        if (bot->GetEntry() == entry)
            return bot->GetGUID();
    }

    return ObjectGuid::Empty;
}

std::vector<uint32> BotDataMgr::GetExistingNPCBotIds()
{
    ASSERT(AllBotsLoaded());

    std::vector<uint32> existing_ids;
    existing_ids.reserve(_botsData.size());
    for (auto const& [bot_id, _] : _botsData)
        existing_ids.push_back(bot_id);

    return existing_ids;
}

uint8 BotDataMgr::GetOwnedBotsCount(ObjectGuid owner_guid, uint32 class_mask, bool count_shared)
{
    uint8 count = 0;
    for (auto const& [bot_id, bot_data] : _botsData)
        if ((bot_data.owner == owner_guid.GetCounter() || (count_shared && bot_data.shared_owners.contains(owner_guid.GetCounter()))) &&
            (!class_mask || !!(class_mask & (1u << (_botsExtras.at(bot_id).bclass - 1)))))
            ++count;
    return count;
}

void BotDataMgr::CollectOwnedBotEntries(uint32 ownerLowGuid, bool includeShared, std::vector<uint32>& entries)
{
    entries.clear();
    std::shared_lock lock(*GetLock());
    entries.reserve(_botsData.size());
    for (auto const& [entry, bot_data] : _botsData)
    {
        if (bot_data.owner == ownerLowGuid || (includeShared && bot_data.shared_owners.contains(ownerLowGuid)))
            entries.push_back(entry);
    }
}

uint8 BotDataMgr::GetAccountBotsCount(uint32 account_id)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_NPCBOT_ACC_BOT_COUNT);
    stmt->setUInt32(0, account_id);

    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (result)
        return (*result)[0].GetUInt32();

    return 0;
}

uint8 BotDataMgr::GetLevelBonusForBotRank(uint32 rank)
{
    switch (rank)
    {
        case CREATURE_ELITE_RARE:
            return 1;
        case CREATURE_ELITE_ELITE:
            return 2;
        case CREATURE_ELITE_RAREELITE:
            return 3;
        default:
            return 0;
    }
}

uint8 BotDataMgr::GetMinLevelForMapId(uint32 mapId)
{
    decltype(_wpMinSpawnLevelPerMapId)::const_iterator cit = _wpMinSpawnLevelPerMapId.find(mapId);
    if (cit != _wpMinSpawnLevelPerMapId.cend())
        return cit->second;

    switch (mapId)
    {
        case 0:
        case 1:
            return 1;
        case 530:
            return 61;
        case 571:
            return 71;
        default:
            return 1;
    }
}
uint8 BotDataMgr::GetMaxLevelForMapId(uint32 mapId)
{
    decltype(_wpMaxSpawnLevelPerMapId)::const_iterator cit = _wpMaxSpawnLevelPerMapId.find(mapId);
    if (cit != _wpMaxSpawnLevelPerMapId.cend())
        return cit->second;

    switch (mapId)
    {
        case 0:
        case 1:
            return 60;
        case 530:
            return 70;
        case 571:
            return 80;
        default:
            return 80;
    }
}

uint8 BotDataMgr::GetMinLevelForBotClass(uint8 m_class)
{
    switch (m_class)
    {
        case BOT_CLASS_DEATH_KNIGHT:
            return 55;
        case BOT_CLASS_ARCHMAGE:
        case BOT_CLASS_SPELLBREAKER:
        case BOT_CLASS_NECROMANCER:
            return 20;
        case BOT_CLASS_DARK_RANGER:
            return 40;
        case BOT_CLASS_SPHYNX:
        case BOT_CLASS_DREADLORD:
            return 60;
        default:
            return 1;
    }
}

int32 BotDataMgr::GetBotBaseReputation(Creature const* bot, FactionEntry const* factionEntry)
{
    if (!factionEntry)
        return 0;

    if (bot->IsNPCBotPet())
        bot = bot->GetBotPetAI()->GetPetsOwner();

    uint32 raceMask = GetDefaultFactionForBotRaceClass(bot->GetBotClass(), bot->GetRace()) == FACTION_TEMPLATE_NEUTRAL_HOSTILE ? 0 : bot->GetRaceMask();
    uint32 classMask = bot->GetClassMask();

    int32 minRep = 42999;
    for (auto i : NPCBots::index_array<uint8, std::size(decltype(FactionEntry::ReputationBase){})>)
    {
        if (raceMask == 0)
            minRep = std::min<int32>(minRep, factionEntry->ReputationBase[i]);
        if ((factionEntry->ReputationRaceMask[i] & raceMask || (factionEntry->ReputationRaceMask[i] == 0 && factionEntry->ReputationClassMask[i] != 0)) &&
            (factionEntry->ReputationClassMask[i] & classMask || factionEntry->ReputationClassMask[i] == 0))
        {
            return factionEntry->ReputationBase[i];
        }
    }

    return std::min<int32>(minRep, 0);
}

uint32 BotDataMgr::GetDefaultFactionForBotRaceClass(uint8 bot_class, uint8 bot_race)
{
    if (bot_class >= BOT_CLASS_EX_START)
        return uint32(FACTION_TEMPLATE_NEUTRAL_HOSTILE);

    ChrRacesEntry const* rentry = sChrRacesStore.LookupEntry(bot_race);
    return rentry ? rentry->FactionID : uint32(FACTION_TEMPLATE_NEUTRAL_HOSTILE);
}

TeamId BotDataMgr::GetTeamIdForFaction(uint32 factionTemplateId)
{
    if (FactionTemplateEntry const* fte = sFactionTemplateStore.LookupEntry(factionTemplateId))
    {
        if (fte->FactionGroup & FACTION_MASK_ALLIANCE)
            return TEAM_ALLIANCE;
        else if (fte->FactionGroup & FACTION_MASK_HORDE)
            return TEAM_HORDE;
    }

    return TEAM_NEUTRAL;
}

uint32 BotDataMgr::GetTeamForFaction(uint32 factionTemplateId)
{
    switch (GetTeamIdForFaction(factionTemplateId))
    {
        case TEAM_ALLIANCE:
            return ALLIANCE;
        case TEAM_HORDE:
            return HORDE;
        default:
            return TEAM_OTHER;
    }
}

uint32 BotDataMgr::GetFirstRoleInMask(uint32 roles_mask)
{
    for (uint32 i = 1u; i < BOT_MAX_ROLE; i <<= 1u)
        if (roles_mask & i)
            return i;
    return roles_mask;
}

uint32 BotDataMgr::LFGToBotRoles(uint8 roles_mask)
{
    using enum lfg::LfgRoles;
    uint32 new_roles = 0;
    if (roles_mask & PLAYER_ROLE_TANK)
        new_roles |= BOT_ROLE_TANK;
    if (roles_mask & PLAYER_ROLE_HEALER)
        new_roles |= BOT_ROLE_HEAL;
    if (roles_mask & PLAYER_ROLE_DAMAGE)
        new_roles |= BOT_ROLE_DPS;
    return new_roles;
}
uint8 BotDataMgr::BotToLFGRoles(uint32 roles_mask, bool first_in_mask)
{
    using enum lfg::LfgRoles;
    uint8 new_roles = 0;
    if (roles_mask & BOT_ROLE_TANK)
        new_roles |= PLAYER_ROLE_TANK;
    if (roles_mask & BOT_ROLE_HEAL)
        if (!first_in_mask || !new_roles)
            new_roles |= PLAYER_ROLE_HEALER;
    if (roles_mask & BOT_ROLE_DPS)
        if (!first_in_mask || !new_roles)
            new_roles |= PLAYER_ROLE_DAMAGE;
    return new_roles;
}
uint32 BotDataMgr::DefaultRolesForClass(uint8 m_class, uint8 spec)
{
    uint32 roleMask = BOT_ROLE_DPS;

    //if (bot_ai::IsHealingClass(m_class))
    //    roleMask |= BOT_ROLE_HEAL;

    if (!BotDataMgr::IsMeleeClass(m_class))
    {
        switch (spec)
        {
            case BOT_SPEC_SHAMAN_ENHANCEMENT:
            case BOT_SPEC_DRUID_FERAL:
                break;
            default:
                roleMask |= BOT_ROLE_RANGED;
                break;
        }
    }

    return roleMask;
}

uint32 BotDataMgr::GetDefaultAutolootRoleMask()
{
    return BOT_ROLE_AUTOLOOT | BOT_ROLE_MASK_LOOTING;
}

uint32 BotDataMgr::GetViableRolesForClass(uint8 bot_class)
{
    uint32 roles;
    switch (bot_class)
    {
        case BOT_CLASS_WARRIOR:     roles = BOT_ROLE_DPS | BOT_ROLE_TANK;                 break;
        case BOT_CLASS_PALADIN:     roles = BOT_ROLE_DPS | BOT_ROLE_TANK | BOT_ROLE_HEAL; break;
        case BOT_CLASS_HUNTER:      roles = BOT_ROLE_DPS;                                 break;
        case BOT_CLASS_ROGUE:       roles = BOT_ROLE_DPS;                                 break;
        case BOT_CLASS_PRIEST:      roles = BOT_ROLE_DPS | BOT_ROLE_HEAL;                 break;
        case BOT_CLASS_DEATH_KNIGHT:roles = BOT_ROLE_DPS | BOT_ROLE_TANK;                 break;
        case BOT_CLASS_SHAMAN:      roles = BOT_ROLE_DPS | BOT_ROLE_HEAL;                 break;
        case BOT_CLASS_MAGE:        roles = BOT_ROLE_DPS;                                 break;
        case BOT_CLASS_WARLOCK:     roles = BOT_ROLE_DPS;                                 break;
        case BOT_CLASS_DRUID:       roles = BOT_ROLE_DPS | BOT_ROLE_TANK | BOT_ROLE_HEAL; break;
        case BOT_CLASS_CRYPT_LORD:  roles = BOT_ROLE_DPS | BOT_ROLE_TANK;                 break;
        case BOT_CLASS_BM:
        case BOT_CLASS_SPHYNX:
        case BOT_CLASS_ARCHMAGE:
        case BOT_CLASS_DREADLORD:
        case BOT_CLASS_SPELLBREAKER:
        case BOT_CLASS_DARK_RANGER:
        case BOT_CLASS_NECROMANCER:
        case BOT_CLASS_SEA_WITCH:   roles = BOT_ROLE_DPS;                                 break;
        default:                    roles = BOT_ROLE_DPS;                                 break;
    }
    return roles;
}

uint8 BotDataMgr::SelectBotSpecForRoles(uint8 bot_class, uint32 bot_roles)
{
    std::vector<uint8> specs;
    specs.reserve(3);
    switch (bot_class)
    {
        case BOT_CLASS_WARRIOR:
            if (bot_roles & BOT_ROLE_TANK)
                specs.push_back(BOT_SPEC_WARRIOR_PROTECTION);
            else
            {
                specs.push_back(BOT_SPEC_WARRIOR_ARMS);
                specs.push_back(BOT_SPEC_WARRIOR_FURY);
            }
            break;
        case BOT_CLASS_PALADIN:
            if (bot_roles & BOT_ROLE_HEAL)
                specs.push_back(BOT_SPEC_PALADIN_HOLY);
            else if (bot_roles & BOT_ROLE_TANK)
                specs.push_back(BOT_SPEC_PALADIN_PROTECTION);
            else
                specs.push_back(BOT_SPEC_PALADIN_RETRIBUTION);
            break;
        case BOT_CLASS_HUNTER:
            specs.push_back(BOT_SPEC_HUNTER_BEASTMASTERY);
            specs.push_back(BOT_SPEC_HUNTER_MARKSMANSHIP);
            specs.push_back(BOT_SPEC_HUNTER_SURVIVAL);
            break;
        case BOT_CLASS_ROGUE:
            specs.push_back(BOT_SPEC_ROGUE_ASSASINATION);
            specs.push_back(BOT_SPEC_ROGUE_COMBAT);
            specs.push_back(BOT_SPEC_ROGUE_SUBTLETY);
            break;
        case BOT_CLASS_PRIEST:
            if (bot_roles & BOT_ROLE_HEAL)
            {
                specs.push_back(BOT_SPEC_PRIEST_DISCIPLINE);
                specs.push_back(BOT_SPEC_PRIEST_HOLY);
            }
            else
                specs.push_back(BOT_SPEC_PRIEST_SHADOW);
            break;
        case BOT_CLASS_DEATH_KNIGHT:
            specs.push_back(BOT_SPEC_DK_BLOOD);
            specs.push_back(BOT_SPEC_DK_UNHOLY);
            if (bot_roles & BOT_ROLE_TANK)
                specs.push_back(BOT_SPEC_DK_FROST);
            break;
        case BOT_CLASS_SHAMAN:
            if (bot_roles & BOT_ROLE_HEAL)
                specs.push_back(BOT_SPEC_SHAMAN_RESTORATION);
            else if (bot_roles & BOT_ROLE_TANK)
                specs.push_back(BOT_SPEC_SHAMAN_ENHANCEMENT);
            else
            {
                specs.push_back(BOT_SPEC_SHAMAN_ELEMENTAL);
                specs.push_back(BOT_SPEC_SHAMAN_ENHANCEMENT);
            }
            break;
        case BOT_CLASS_MAGE:
            specs.push_back(BOT_SPEC_MAGE_ARCANE);
            specs.push_back(BOT_SPEC_MAGE_FIRE);
            specs.push_back(BOT_SPEC_MAGE_FROST);
            break;
        case BOT_CLASS_WARLOCK:
            specs.push_back(BOT_SPEC_WARLOCK_AFFLICTION);
            specs.push_back(BOT_SPEC_WARLOCK_DEMONOLOGY);
            specs.push_back(BOT_SPEC_WARLOCK_DESTRUCTION);
            break;
        case BOT_CLASS_DRUID:
            if (bot_roles & BOT_ROLE_HEAL)
                specs.push_back(BOT_SPEC_DRUID_RESTORATION);
            else if (bot_roles & BOT_ROLE_TANK)
                specs.push_back(BOT_SPEC_DRUID_FERAL);
            else
            {
                specs.push_back(BOT_SPEC_DRUID_BALANCE);
                specs.push_back(BOT_SPEC_DRUID_FERAL);
            }
            break;
        default:
            break;
    }

    uint8 spec = specs.empty() ? BotDataMgr::SelectSpecForClass(bot_class) : specs.size() == std::size_t(1) ? specs.front() : Bcore::Containers::SelectRandomContainerElement(specs);
    return spec;
}

uint8 BotDataMgr::SelectSpecForClass(uint8 m_class)
{
    std::vector<uint8> specs;
    specs.reserve(3);
    switch (m_class)
    {
        case BOT_CLASS_WARRIOR: //any
            specs.push_back(BOT_SPEC_WARRIOR_ARMS);
            specs.push_back(BOT_SPEC_WARRIOR_FURY);
            specs.push_back(BOT_SPEC_WARRIOR_PROTECTION);
            break;
        case BOT_CLASS_PALADIN: //retri
            specs.push_back(BOT_SPEC_PALADIN_RETRIBUTION);
            break;
        case BOT_CLASS_HUNTER: //any
            specs.push_back(BOT_SPEC_HUNTER_BEASTMASTERY);
            specs.push_back(BOT_SPEC_HUNTER_MARKSMANSHIP);
            specs.push_back(BOT_SPEC_HUNTER_SURVIVAL);
            break;
        case BOT_CLASS_ROGUE: //any
            specs.push_back(BOT_SPEC_ROGUE_ASSASINATION);
            specs.push_back(BOT_SPEC_ROGUE_COMBAT);
            specs.push_back(BOT_SPEC_ROGUE_SUBTLETY);
            break;
        case BOT_CLASS_PRIEST: //discipline, shadow
            specs.push_back(BOT_SPEC_PRIEST_DISCIPLINE);
            specs.push_back(BOT_SPEC_PRIEST_SHADOW);
            break;
        case BOT_CLASS_DEATH_KNIGHT: //any
            specs.push_back(BOT_SPEC_DK_BLOOD);
            specs.push_back(BOT_SPEC_DK_FROST);
            specs.push_back(BOT_SPEC_DK_UNHOLY);
            break;
        case BOT_CLASS_SHAMAN: //elem, enh
            specs.push_back(BOT_SPEC_SHAMAN_ELEMENTAL);
            specs.push_back(BOT_SPEC_SHAMAN_ENHANCEMENT);
            break;
        case BOT_CLASS_MAGE: //fire, frost
            specs.push_back(BOT_SPEC_MAGE_FIRE);
            specs.push_back(BOT_SPEC_MAGE_FROST);
            break;
        case BOT_CLASS_WARLOCK: //affli, destr
            specs.push_back(BOT_SPEC_WARLOCK_AFFLICTION);
            specs.push_back(BOT_SPEC_WARLOCK_DESTRUCTION);
            break;
        case BOT_CLASS_DRUID: //balance, feral
            specs.push_back(BOT_SPEC_DRUID_BALANCE);
            specs.push_back(BOT_SPEC_DRUID_FERAL);
            break;
        default:
            specs.push_back(BOT_SPEC_DEFAULT);
            break;
    }

    return specs.size() == 1 ? specs.front() : Bcore::Containers::SelectRandomContainerElement(specs);
}

uint32 BotDataMgr::TextForClass(uint8 botclass)
{
    switch (botclass)
    {
        case BOT_CLASS_WARRIOR:       return BOT_TEXT_CLASS_WARRIOR;
        case BOT_CLASS_PALADIN:       return BOT_TEXT_CLASS_PALADIN;
        case BOT_CLASS_HUNTER:        return BOT_TEXT_CLASS_HUNTER;
        case BOT_CLASS_ROGUE:         return BOT_TEXT_CLASS_ROGUE;
        case BOT_CLASS_PRIEST:        return BOT_TEXT_CLASS_PRIEST;
        case BOT_CLASS_DEATH_KNIGHT:  return BOT_TEXT_CLASS_DEATH_KNIGHT;
        case BOT_CLASS_SHAMAN:        return BOT_TEXT_CLASS_SHAMAN;
        case BOT_CLASS_MAGE:          return BOT_TEXT_CLASS_MAGE;
        case BOT_CLASS_WARLOCK:       return BOT_TEXT_CLASS_WARLOCK;
        case BOT_CLASS_DRUID:         return BOT_TEXT_CLASS_DRUID;
        case BOT_CLASS_BM:            return BOT_TEXT_CLASS_BM;
        case BOT_CLASS_SPHYNX:        return BOT_TEXT_CLASS_SPHYNX;
        case BOT_CLASS_ARCHMAGE:      return BOT_TEXT_CLASS_ARCHMAGE;
        case BOT_CLASS_DREADLORD:     return BOT_TEXT_CLASS_DREADLORD;
        case BOT_CLASS_SPELLBREAKER:  return BOT_TEXT_CLASS_SPELLBREAKER;
        case BOT_CLASS_DARK_RANGER:   return BOT_TEXT_CLASS_DARK_RANGER;
        case BOT_CLASS_NECROMANCER:   return BOT_TEXT_CLASS_NECROMANCER;
        case BOT_CLASS_SEA_WITCH:     return BOT_TEXT_CLASS_SEAWITCH;
        case BOT_CLASS_CRYPT_LORD:    return BOT_TEXT_CLASS_CRYPT_LORD;
        default:                      return BOT_TEXT_UNKNOWN;
    }
}

uint32 BotDataMgr::TextForSpec(uint8 spec)
{
    switch (spec)
    {
        case BOT_SPEC_WARRIOR_ARMS:         return BOT_TEXT_SPEC_ARMS;
        case BOT_SPEC_WARRIOR_FURY:         return BOT_TEXT_SPEC_FURY;
        case BOT_SPEC_WARRIOR_PROTECTION:   return BOT_TEXT_SPEC_PROTECTION;
        case BOT_SPEC_PALADIN_HOLY:         return BOT_TEXT_SPEC_HOLY;
        case BOT_SPEC_PALADIN_PROTECTION:   return BOT_TEXT_SPEC_PROTECTION;
        case BOT_SPEC_PALADIN_RETRIBUTION:  return BOT_TEXT_SPEC_RETRIBUTION;
        case BOT_SPEC_HUNTER_BEASTMASTERY:  return BOT_TEXT_SPEC_BEASTMASTERY;
        case BOT_SPEC_HUNTER_MARKSMANSHIP:  return BOT_TEXT_SPEC_MARKSMANSHIP;
        case BOT_SPEC_HUNTER_SURVIVAL:      return BOT_TEXT_SPEC_SURVIVAL;
        case BOT_SPEC_ROGUE_ASSASINATION:   return BOT_TEXT_SPEC_ASSASINATION;
        case BOT_SPEC_ROGUE_COMBAT:         return BOT_TEXT_SPEC_COMBAT;
        case BOT_SPEC_ROGUE_SUBTLETY:       return BOT_TEXT_SPEC_SUBTLETY;
        case BOT_SPEC_PRIEST_DISCIPLINE:    return BOT_TEXT_SPEC_DISCIPLINE;
        case BOT_SPEC_PRIEST_HOLY:          return BOT_TEXT_SPEC_HOLY;
        case BOT_SPEC_PRIEST_SHADOW:        return BOT_TEXT_SPEC_SHADOW;
        case BOT_SPEC_DK_BLOOD:             return BOT_TEXT_SPEC_BLOOD;
        case BOT_SPEC_DK_FROST:             return BOT_TEXT_SPEC_FROST;
        case BOT_SPEC_DK_UNHOLY:            return BOT_TEXT_SPEC_UNHOLY;
        case BOT_SPEC_SHAMAN_ELEMENTAL:     return BOT_TEXT_SPEC_ELEMENTAL;
        case BOT_SPEC_SHAMAN_ENHANCEMENT:   return BOT_TEXT_SPEC_ENHANCEMENT;
        case BOT_SPEC_SHAMAN_RESTORATION:   return BOT_TEXT_SPEC_RESTORATION;
        case BOT_SPEC_MAGE_ARCANE:          return BOT_TEXT_SPEC_ARCANE;
        case BOT_SPEC_MAGE_FIRE:            return BOT_TEXT_SPEC_FIRE;
        case BOT_SPEC_MAGE_FROST:           return BOT_TEXT_SPEC_FROST;
        case BOT_SPEC_WARLOCK_AFFLICTION:   return BOT_TEXT_SPEC_AFFLICTION;
        case BOT_SPEC_WARLOCK_DEMONOLOGY:   return BOT_TEXT_SPEC_DEMONOLOGY;
        case BOT_SPEC_WARLOCK_DESTRUCTION:  return BOT_TEXT_SPEC_DESTRUCTION;
        case BOT_SPEC_DRUID_BALANCE:        return BOT_TEXT_SPEC_BALANCE;
        case BOT_SPEC_DRUID_FERAL:          return BOT_TEXT_SPEC_FERAL;
        case BOT_SPEC_DRUID_RESTORATION:    return BOT_TEXT_SPEC_RESTORATION;
        case BOT_SPEC_DEFAULT: default:     return BOT_TEXT_SPEC_UNKNOWN;
    }
}

bool BotDataMgr::IsValidSpecForClass(uint8 m_class, uint8 spec)
{
    switch (m_class)
    {
        case BOT_CLASS_WARRIOR:
            switch (spec)
            {
                case BOT_SPEC_WARRIOR_ARMS:
                case BOT_SPEC_WARRIOR_FURY:
                case BOT_SPEC_WARRIOR_PROTECTION:
                    return true;
                default:
                    break;
            }
            break;
        case BOT_CLASS_PALADIN:
            switch (spec)
            {
                case BOT_SPEC_PALADIN_HOLY:
                case BOT_SPEC_PALADIN_PROTECTION:
                case BOT_SPEC_PALADIN_RETRIBUTION:
                    return true;
                default:
                    break;
            }
            break;
        case BOT_CLASS_HUNTER:
            switch (spec)
            {
                case BOT_SPEC_HUNTER_BEASTMASTERY:
                case BOT_SPEC_HUNTER_MARKSMANSHIP:
                case BOT_SPEC_HUNTER_SURVIVAL:
                    return true;
                default:
                    break;
            }
            break;
        case BOT_CLASS_ROGUE:
            switch (spec)
            {
                case BOT_SPEC_ROGUE_ASSASINATION:
                case BOT_SPEC_ROGUE_COMBAT:
                case BOT_SPEC_ROGUE_SUBTLETY:
                    return true;
                default:
                    break;
            }
            break;
        case BOT_CLASS_PRIEST:
            switch (spec)
            {
                case BOT_SPEC_PRIEST_DISCIPLINE:
                case BOT_SPEC_PRIEST_HOLY:
                case BOT_SPEC_PRIEST_SHADOW:
                    return true;
                default:
                    break;
            }
            break;
        case BOT_CLASS_DEATH_KNIGHT:
            switch (spec)
            {
                case BOT_SPEC_DK_BLOOD:
                case BOT_SPEC_DK_FROST:
                case BOT_SPEC_DK_UNHOLY:
                    return true;
                default:
                    break;
            }
            break;
        case BOT_CLASS_SHAMAN:
            switch (spec)
            {
                case BOT_SPEC_SHAMAN_ELEMENTAL:
                case BOT_SPEC_SHAMAN_ENHANCEMENT:
                case BOT_SPEC_SHAMAN_RESTORATION:
                    return true;
                default:
                    break;
            }
            break;
        case BOT_CLASS_MAGE:
            switch (spec)
            {
                case BOT_SPEC_MAGE_ARCANE:
                case BOT_SPEC_MAGE_FIRE:
                case BOT_SPEC_MAGE_FROST:
                    return true;
                default:
                    break;
            }
            break;
        case BOT_CLASS_WARLOCK:
            switch (spec)
            {
                case BOT_SPEC_WARLOCK_AFFLICTION:
                case BOT_SPEC_WARLOCK_DEMONOLOGY:
                case BOT_SPEC_WARLOCK_DESTRUCTION:
                    return true;
                default:
                    break;
            }
            break;
        case BOT_CLASS_DRUID:
            switch (spec)
            {
                case BOT_SPEC_DRUID_BALANCE:
                case BOT_SPEC_DRUID_FERAL:
                case BOT_SPEC_DRUID_RESTORATION:
                    return true;
                default:
                    break;
            }
            break;
        case BOT_CLASS_BM:
        case BOT_CLASS_SPHYNX:
        case BOT_CLASS_ARCHMAGE:
        case BOT_CLASS_DREADLORD:
        case BOT_CLASS_SPELLBREAKER:
        case BOT_CLASS_DARK_RANGER:
        case BOT_CLASS_NECROMANCER:
        case BOT_CLASS_SEA_WITCH:
        case BOT_CLASS_CRYPT_LORD:
            return spec == BOT_SPEC_DEFAULT;
        default:
            break;
    }
    return false;
}

bool BotDataMgr::IsMeleeClass(uint8 m_class)
{
    return IsBotClassMask(m_class, MELEE_BOT_CLASSES_MASK);
}
bool BotDataMgr::IsTankingClass(uint8 m_class)
{
    return IsBotClassMask(m_class, TANKING_BOT_CLASSES_MASK);
}
bool BotDataMgr::IsBlockingClass(uint8 m_class)
{
    return IsBotClassMask(m_class, BLOCKING_BOT_CLASSES_MASK);
}
bool BotDataMgr::IsCastingClass(uint8 m_class)
{
    //Class can benefit from spellpower
    return IsBotClassMask(m_class, CASTING_BOT_CLASSES_MASK);
}
bool BotDataMgr::IsHealingClass(uint8 m_class)
{
    return IsBotClassMask(m_class, HEALING_BOT_CLASSES_MASK);
}
bool BotDataMgr::IsHumanoidClass(uint8 m_class)
{
    return IsBotClassMask(m_class, HUMANOID_BOT_CLASSES_MASK);
}
bool BotDataMgr::IsHeroExClass(uint8 m_class)
{
    return IsBotClassMask(m_class, HERO_BOT_CLASSES_MASK);
}
bool BotDataMgr::IsMeleeSpec(uint8 spec)
{
    return IsBotSpecMask(spec, BOT_SPEC_MASK_MELEE);
}

bool BotDataMgr::CanDepositBotBankItemsCount(ObjectGuid playerGuid, uint32 items_count)
{
    if (uint32 capacity = BotCfg::GetGearBankCapacity())
    {
        uint32 stored_count = GetBotBankItemsCount(playerGuid);
        if (stored_count + items_count > capacity)
            return false;
    }
    return true;
}

BotBankItemContainer const* BotDataMgr::GetBotBankItems(ObjectGuid playerGuid)
{
    decltype(_botStoredGearMap)::iterator mci = _botStoredGearMap.find(playerGuid);
    return mci != _botStoredGearMap.cend() ? &mci->second : nullptr;
}

uint32 BotDataMgr::GetBotBankItemsCount(ObjectGuid playerGuid)
{
    if (BotBankItemContainer const* botBankItems = GetBotBankItems(playerGuid))
        return static_cast<uint32>(botBankItems->size());
    return 0;
}

Item* BotDataMgr::WithdrawBotBankItem(ObjectGuid playerGuid, ObjectGuid::LowType itemGuidLow)
{
    decltype(_botStoredGearMap)::iterator mci = _botStoredGearMap.find(playerGuid);
    if (mci != _botStoredGearMap.cend())
    {
        auto ici = std::ranges::find_if(mci->second, [=](Item const* item) { return item->GetGUID().GetCounter() == itemGuidLow; });
        if (ici != mci->second.cend())
        {
            Item* item = *ici;
            mci->second.erase(ici);
            return item;
        }
    }

    return nullptr;
}

void BotDataMgr::DepositBotBankItem(ObjectGuid playerGuid, Item* item)
{
    _botStoredGearMap[playerGuid].insert(item);
}

void BotDataMgr::SaveNpcBotStoredGear(ObjectGuid playerGuid, CharacterDatabaseTransaction trans)
{
    decltype(_botStoredGearMap)::iterator mci = _botStoredGearMap.find(playerGuid);
    // we don't check if container is empty!
    // we have to be able to erase items always
    if (mci == _botStoredGearMap.cend())
        return;

    trans->PAppend("DELETE FROM characters_npcbot_gear_storage WHERE guid = {}", mci->first.GetCounter());
    for (Item* item : mci->second)
    {
        //order is important here
        item->SaveToDB(trans);
        item->DeleteFromInventoryDB(trans);
        trans->PAppend("INSERT INTO characters_npcbot_gear_storage (guid, item_guid) VALUES ({}, {})", mci->first.GetCounter(), item->GetGUID().GetCounter());
    }
}

uint32 BotDataMgr::GetBotItemSetsCount(ObjectGuid playerGuid)
{
    if (BotItemSetsArray const* item_sets = GetBotItemSets(playerGuid))
        return std::ranges::count_if(NPCBots::index_array<uint8, MAX_BOT_EQUIPMENT_SETS>, [=](uint8 i) { return !!item_sets->at(i); });
    return 0;
}

BotItemSetsArray const* BotDataMgr::GetBotItemSets(ObjectGuid playerGuid)
{
    decltype(_botStoredGearSetMap)::const_iterator sci = _botStoredGearSetMap.find(playerGuid);
    return sci != _botStoredGearSetMap.cend() ? &sci->second : nullptr;
}

NpcBotItemSet const* BotDataMgr::GetBotItemSet(ObjectGuid playerGuid, uint8 set_id)
{
    if (BotItemSetsArray const* item_sets = GetBotItemSets(playerGuid))
        return &item_sets->at(set_id);
    return nullptr;
}

NpcBotItemSet& BotDataMgr::CreateNewBotItemSet(ObjectGuid playerGuid)
{
    auto [itr, _] = _botStoredGearSetMap.try_emplace(playerGuid);
    auto& item_sets = itr->second;

    for (auto i : NPCBots::index_array<uint8, MAX_BOT_EQUIPMENT_SETS>)
    {
        if (!item_sets[i])
            return item_sets[i];
    }

    //should not happen
    BOT_LOG_ERROR("npcbots", "CreateNewBotItemSet: item set limit was exhausted by player {}. Using last offset!", playerGuid.ToString());
    const uint8 max_offset = MAX_BOT_EQUIPMENT_SETS - 1;
    item_sets[max_offset].clear();
    return item_sets[max_offset];
}

void BotDataMgr::UpdateBotItemSet(ObjectGuid playerGuid, uint8 set_id, std::string&& set_name)
{
    _botStoredGearSetMap[playerGuid][set_id].name = std::move(set_name);
}

void BotDataMgr::UpdateBotItemSet(ObjectGuid playerGuid, uint8 set_id, uint8 slot, uint32 item_id)
{
    _botStoredGearSetMap[playerGuid][set_id].items[slot] = item_id;
}

void BotDataMgr::DeleteBotItemSet(ObjectGuid playerGuid, uint8 set_id)
{
    _botStoredGearSetMap.at(playerGuid).at(set_id).clear();
}

void BotDataMgr::SaveNpcBotItemSets(ObjectGuid playerGuid, CharacterDatabaseTransaction trans)
{
    decltype(_botStoredGearSetMap)::const_iterator sci = _botStoredGearSetMap.find(playerGuid);
    if (sci == _botStoredGearSetMap.cend())
        return;

    trans->PAppend("DELETE FROM characters_npcbot_gear_set WHERE owner = {}", sci->first.GetCounter());
    trans->PAppend("DELETE FROM characters_npcbot_gear_set_item WHERE owner = {}", sci->first.GetCounter());
    for (auto i : NPCBots::index_array<uint32, MAX_BOT_EQUIPMENT_SETS>)
    {
        NpcBotItemSet const& item_set = sci->second[i];
        if (!!item_set)
        {
            trans->PAppend("INSERT INTO characters_npcbot_gear_set (owner, set_id, set_name) VALUES ({}, {}, '{}')", sci->first.GetCounter(), i, item_set.name);
            for (auto j : NPCBots::index_array<uint32, BOT_INVENTORY_SIZE>)
            {
                if (item_set.items[j])
                {
                    trans->PAppend("INSERT INTO characters_npcbot_gear_set_item (owner, set_id, slot, item_id) VALUES ({}, {}, {}, {})",
                        sci->first.GetCounter(), i, j, item_set.items[j]);
                }
            }
        }
    }
}

NpcBotMgrData* BotDataMgr::SelectOrCreateNpcBotMgrData(ObjectGuid playerGuid)
{
    std::unique_lock lock(*GetLock());
    decltype(_botMgrsData)::iterator bmdi = _botMgrsData.find(playerGuid);
    if (bmdi == _botMgrsData.cend())
    {
        CharacterDatabase.PExecute("INSERT INTO characters_npcbot_settings (owner) VALUES ({})", playerGuid.GetCounter());
        auto placed = _botMgrsData.emplace(std::piecewise_construct, std::forward_as_tuple(playerGuid), std::forward_as_tuple(BotCfg::GetFollowDistDefault(), 0, BOT_ATTACK_RANGE_SHORT, BOT_ATTACK_ANGLE_NORMAL, 0, 0, 0));
        return &placed.first->second;
    }

    return &bmdi->second;
}

void BotDataMgr::EraseNpcBotMgrData(ObjectGuid playerGuid)
{
    std::unique_lock lock(*GetLock());
    decltype(_botMgrsData)::iterator bmci = _botMgrsData.find(playerGuid);
    if (bmci == _botMgrsData.cend())
        return;

    RemoveNpcBotMgrDataFromDB(playerGuid);
    _botMgrsData.erase(bmci);
}

void BotDataMgr::RemoveNpcBotMgrDataFromDB(ObjectGuid playerGuid)
{
    CharacterDatabase.PExecute("DELETE FROM characters_npcbot_settings WHERE owner = {}", playerGuid.GetCounter());
}

void BotDataMgr::SaveNpcBotMgrData(ObjectGuid playerGuid, CharacterDatabaseTransaction trans)
{
    std::shared_lock lock(*GetLock());
    decltype(_botMgrsData)::iterator bmdi = _botMgrsData.find(playerGuid);
    if (bmdi == _botMgrsData.cend())
        return;

    NpcBotMgrData const& md = bmdi->second;
    trans->PAppend("DELETE FROM characters_npcbot_settings WHERE owner = {}", bmdi->first.GetCounter());
    trans->PAppend("INSERT INTO characters_npcbot_settings (owner,dist_follow,dist_attack,attack_range_mode,attack_angle_mode,engage_delay_dps,engage_delay_heal,flags) VALUES ({},{},{},{},{},{},{},{})",
        bmdi->first.GetCounter(), md.dist_follow, md.dist_attack, md.attack_range_mode, md.attack_angle_mode, md.engage_delay_dps, md.engage_delay_heal,
        (md.flags & NPCBOT_MGR_FLAG_MASK_ALL_DB_ALLOWED));
}

bool BotDataMgr::EnsureArenaBotProfile(ObjectGuid playerGuid, uint8 bracketType, std::string_view teamName)
{
    // By leewheel 20260528 - create arena bot profile row lazily for selected bracket (2v2/3v3/5v5).
    if (!playerGuid.IsPlayer() || (bracketType != 2 && bracketType != 3 && bracketType != 5))
        return false;

    ObjectGuid::LowType owner = playerGuid.GetCounter();
    QueryResult exists = CharacterDatabase.PQuery("SELECT owner_guid FROM characters_npcbot_arena_profile WHERE owner_guid={} AND bracket_type={} LIMIT 1",
        owner, uint32(bracketType));
    if (exists)
        return true;

    std::string safeName(teamName.empty() ? "Arena Team" : std::string(teamName));
    CharacterDatabase.EscapeString(safeName);
    CharacterDatabase.PExecute("INSERT INTO characters_npcbot_arena_profile (owner_guid, bracket_type, team_name) VALUES ({}, {}, '{}')",
        owner, uint32(bracketType), safeName);
    return true;
}

void BotDataMgr::SetArenaBotTeamName(ObjectGuid playerGuid, uint8 bracketType, std::string_view teamName)
{
    // By leewheel 20260528 - keep bot-arena profile name aligned with chosen/rated team naming.
    if (!playerGuid.IsPlayer() || (bracketType != 2 && bracketType != 3 && bracketType != 5))
        return;

    std::string safeName(teamName.empty() ? "Arena Team" : std::string(teamName));
    CharacterDatabase.EscapeString(safeName);
    CharacterDatabase.PExecute("UPDATE characters_npcbot_arena_profile SET team_name='{}' WHERE owner_guid={} AND bracket_type={}",
        safeName, playerGuid.GetCounter(), uint32(bracketType));
}

std::string BotDataMgr::GetArenaBotTeamName(ObjectGuid playerGuid, uint8 bracketType)
{
    if (!playerGuid.IsPlayer() || (bracketType != 2 && bracketType != 3 && bracketType != 5))
        return {};

    QueryResult res = CharacterDatabase.PQuery("SELECT team_name FROM characters_npcbot_arena_profile WHERE owner_guid={} AND bracket_type={} LIMIT 1",
        playerGuid.GetCounter(), uint32(bracketType));
    if (!res)
        return {};

    return res->Fetch()[0].GetString();
}

std::vector<ArenaBotRosterSlot> BotDataMgr::GetArenaBotRoster(ObjectGuid playerGuid, uint8 bracketType)
{
    std::vector<ArenaBotRosterSlot> roster;
    if (!playerGuid.IsPlayer() || (bracketType != 2 && bracketType != 3 && bracketType != 5))
        return roster;

    QueryResult res = CharacterDatabase.PQuery(
        "SELECT slot_index, bot_entry, bot_class, bot_spec FROM characters_npcbot_arena_roster WHERE owner_guid={} AND bracket_type={} ORDER BY slot_index",
        playerGuid.GetCounter(), uint32(bracketType));

    if (!res)
        return roster;

    do
    {
        Field* f = res->Fetch();
        ArenaBotRosterSlot slot;
        slot.slot = f[0].GetUInt8();
        slot.botEntry = f[1].GetUInt32();
        slot.botClass = f[2].GetUInt8();
        slot.botSpec = f[3].GetUInt8();
        roster.push_back(slot);
    } while (res->NextRow());

    return roster;
}

void BotDataMgr::SaveArenaBotRoster(ObjectGuid playerGuid, uint8 bracketType, std::vector<ArenaBotRosterSlot> const& roster)
{
    if (!playerGuid.IsPlayer() || (bracketType != 2 && bracketType != 3 && bracketType != 5))
        return;

    ObjectGuid::LowType owner = playerGuid.GetCounter();
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM characters_npcbot_arena_roster WHERE owner_guid={} AND bracket_type={}", owner, uint32(bracketType));
    for (ArenaBotRosterSlot const& slot : roster)
    {
        trans->PAppend("INSERT INTO characters_npcbot_arena_roster (owner_guid, bracket_type, slot_index, bot_entry, bot_class, bot_spec) VALUES ({}, {}, {}, {}, {}, {})",
            owner, uint32(bracketType), uint32(slot.slot), uint32(slot.botEntry), uint32(slot.botClass), uint32(slot.botSpec));
    }
    CharacterDatabase.CommitTransaction(trans);
}

uint32 BotDataMgr::GenerateArenaRosterBots(Player const* leader, uint8 arenaType, uint8 desiredBotCount)
{
    // By leewheel 20260528 - spawn fixed roster arena companions for player queue.
    if (!leader || !leader->GetSession() || arenaType < 2 || arenaType > 5)
        return 0;
    if (!BotCfg::IsNpcBotModEnabled())
        return 0;

    std::vector<ArenaBotRosterSlot> roster = GetArenaBotRoster(leader->GetGUID(), arenaType);
    bool rosterChanged = false;
    uint32 const spawned = sBotGen->GenerateArenaBotsToSpawn(leader, arenaType, desiredBotCount, roster, rosterChanged);
    if (rosterChanged)
        SaveArenaBotRoster(leader->GetGUID(), arenaType, roster);
    return spawned;
}

class TC_GAME_API WanderingBotXpGainFormulaScript : public FormulaScript
{
    static constexpr float WANDERING_BOT_XP_GAIN_MULT = 10.0f;

public:
    WanderingBotXpGainFormulaScript() : FormulaScript("WanderingBotXpGainFormulaScript") {}

    void OnGainCalculation(uint32& gain, Player* /*player*/, Unit* unit) override
    {
        if (gain && unit->IsNPCBot() && unit->ToCreature()->IsWandererBot())
            gain *= WANDERING_BOT_XP_GAIN_MULT;
    }
};

class TC_GAME_API BotDataMgrShutdownScript : public WorldScript
{
public:
    BotDataMgrShutdownScript() : WorldScript("BotDataMgrShutdownScript") {}

    void OnShutdown() override
    {
        botSpawnEvents.KillAllEvents(true);
        for (auto& [_, events] : botBGJoinEvents)
            events.KillAllEvents(true);
    }
};

// By leewheel 20260528 - start wanderer wake grace when players enter a continent (immersion / less instant mob clearing).
class BotWandererMapPlayerScript : public PlayerScript
{
public:
    BotWandererMapPlayerScript() : PlayerScript("BotWandererMapPlayerScript") { }

    void OnMapChanged(Player* player) override
    {
        if (!player || player->GetSession()->PlayerLoading())
            return;
        BotDataMgr::NotifyPlayerEnteredWorldMap(player);
    }
};

void AddSC_botdatamgr_scripts()
{
    new WanderingBotXpGainFormulaScript();
    new BotDataMgrShutdownScript();
    new BotWandererMapPlayerScript();
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif
