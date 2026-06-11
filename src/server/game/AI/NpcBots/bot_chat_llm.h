#ifndef BOT_CHAT_LLM_H
#define BOT_CHAT_LLM_H

#include <functional>
#include <string>

class Creature;

namespace NpcBotChatLLM
{
struct RuntimeState;

using LlmReplyCallback = std::function<void(std::string const&)>;

// By leewheel 20260528 - embedded LLM runtime facade (llama.cpp integration point).
class Engine
{
public:
    static Engine& Instance();

    void Configure(bool enabled, std::string modelPath, bool useGpu);
    [[nodiscard]] bool IsEnabled() const;

    // Never blocks the world thread; callbacks run from PollCompletedReplies().
    void QueueReply(Creature const* bot, std::string prompt, LlmReplyCallback callback);
    void PollCompletedReplies();

private:
    void EnsureWorker();
    void WorkerLoop();
    [[nodiscard]] std::string GenerateReplyLocked(uint32 botEntry, std::string const& prompt) const;

    RuntimeState* _runtime = nullptr;
    bool _enabled = false;
    bool _useGpu = false;
    std::string _modelPath;
};
}

#endif
