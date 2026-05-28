#ifndef BOT_CHAT_LLM_H
#define BOT_CHAT_LLM_H

#include <string>

class Creature;

namespace NpcBotChatLLM
{
struct RuntimeState;

// By leewheel 20260528 - embedded LLM runtime facade (llama.cpp integration point).
class Engine
{
public:
    static Engine& Instance();

    void Configure(bool enabled, std::string modelPath);
    [[nodiscard]] bool IsEnabled() const;
    [[nodiscard]] std::string GenerateReply(Creature const* bot, std::string const& prompt) const;

private:
    RuntimeState* _runtime = nullptr;
    bool _enabled = false;
    std::string _modelPath;
};
}

#endif
