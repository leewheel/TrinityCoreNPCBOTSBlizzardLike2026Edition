/*
 * Wanderer bot world-channel vendor (whisper negotiate + COD mail).
 * By leewheel 20260529
 */
#ifndef BOT_WANDERER_VENDOR_H
#define BOT_WANDERER_VENDOR_H

#include <string>
#include <string_view>

class Creature;
class Player;

namespace WandererVendor
{
    // Scan item_template once at startup (BoE rare+ gear + profession materials).
    void BuildCatalog();

    bool IsEnabled();

    // Refresh listing from DB catalog on periodic tick (wanderer only).
    void OnWandererTick(Creature* bot);

    // If this bot should use trade channel hint on next world chat line.
    bool WantsTradeWorldShout(Creature const* bot);

    // Scene-event text for LLM grounding before trade shout.
    std::string BuildTradeSceneEvent(Creature const* bot);

    // Fixed line when LLM is off (world channel hawking).
    std::string BuildTradeShoutFallback(Creature const* bot);

    // Player whisper to bot name: negotiate / buy / COD mail.
    bool TryHandlePlayerWhisper(Player* player, Creature* bot, std::string_view message);
}

#endif
