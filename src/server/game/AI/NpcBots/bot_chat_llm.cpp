#include "bot_chat_llm.h"

#include "Creature.h"
#include "bot_ai.h"
#include "botcommon.h"

#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

#ifdef TRINITY_NPCBOT_LLM_EMBED
#include "llama.h"
#endif

namespace NpcBotChatLLM
{
struct RuntimeState
{
#ifdef TRINITY_NPCBOT_LLM_EMBED
    llama_model* model = nullptr;
    llama_context* context = nullptr;
#endif
    std::mutex inferMutex;
    bool loaded = false;
    std::string loadedPath;
};

Engine& Engine::Instance()
{
    static Engine engine;
    return engine;
}

void Engine::Configure(bool enabled, std::string modelPath)
{
    if (!_runtime)
        _runtime = new RuntimeState();

    _enabled = enabled;
    _modelPath = std::move(modelPath);

    if (!_modelPath.empty())
    {
        std::filesystem::path p(_modelPath);
        if (!p.has_parent_path())
        {
            // By leewheel 20260528 - fixed model directory policy: .\ClientData\Ai.Mod\<ModelName>.
            p = std::filesystem::path(".") / "ClientData" / "Ai.Mod" / p;
            _modelPath = p.lexically_normal().string();
        }
    }

#ifdef TRINITY_NPCBOT_LLM_EMBED
    static bool backendInited = false;
    if (!backendInited)
    {
        llama_backend_init();
        backendInited = true;
    }

    if (!_enabled || _modelPath.empty())
        return;

    if (_runtime->loaded && _runtime->loadedPath == _modelPath)
        return;

    if (_runtime->context)
    {
        llama_free(_runtime->context);
        _runtime->context = nullptr;
    }
    if (_runtime->model)
    {
        llama_free_model(_runtime->model);
        _runtime->model = nullptr;
    }

    if (!std::filesystem::exists(_modelPath))
    {
        _runtime->loaded = false;
        return;
    }

    llama_model_params mparams = llama_model_default_params();
    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = 2048;
    cparams.n_threads = std::max<int>(1, std::thread::hardware_concurrency() > 2 ? int(std::thread::hardware_concurrency() - 1) : 1);

    _runtime->model = llama_load_model_from_file(_modelPath.c_str(), mparams);
    if (!_runtime->model)
    {
        _runtime->loaded = false;
        return;
    }

    _runtime->context = llama_new_context_with_model(_runtime->model, cparams);
    _runtime->loaded = (_runtime->context != nullptr);
    _runtime->loadedPath = _modelPath;
#endif
}

bool Engine::IsEnabled() const
{
    if (!_enabled || _modelPath.empty() || !_runtime)
        return false;
#ifdef TRINITY_NPCBOT_LLM_EMBED
    return _runtime->loaded;
#else
    return false;
#endif
}

std::string Engine::GenerateReply(Creature const* bot, std::string const& prompt) const
{
    if (!bot || !IsEnabled())
        return {};

    RuntimeState* rt = _runtime;
    if (!rt || !rt->loaded || !rt->model || !rt->context)
        return {};

#ifdef TRINITY_NPCBOT_LLM_EMBED
    std::scoped_lock lk(rt->inferMutex);
    const llama_vocab* vocab = llama_model_get_vocab(rt->model);
    if (!vocab)
        return {};

    std::string fullPrompt = "你是魔兽世界NPC机器人，只用简体中文、简短口语回复，不谈政治。";
    fullPrompt += "\n场景信息：";
    fullPrompt += prompt;
    fullPrompt += "\n回复：";

    std::vector<llama_token> tokens(fullPrompt.size() + 8);
    int32_t nTok = llama_tokenize(vocab, fullPrompt.c_str(), int32_t(fullPrompt.size()), tokens.data(), int32_t(tokens.size()), true, true);
    if (nTok < 0)
    {
        tokens.resize(-nTok + 8);
        nTok = llama_tokenize(vocab, fullPrompt.c_str(), int32_t(fullPrompt.size()), tokens.data(), int32_t(tokens.size()), true, true);
    }
    if (nTok <= 0)
        return {};
    tokens.resize(nTok);

    llama_memory_clear(llama_get_memory(rt->context), true);
    llama_batch batch = llama_batch_get_one(tokens.data(), int32_t(tokens.size()));
    if (llama_decode(rt->context, batch) < 0)
        return {};

    llama_sampler* smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(smpl, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(smpl, llama_sampler_init_top_p(0.9f, 1));
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.7f));
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    std::string output;
    output.reserve(256);
    constexpr int32_t maxGen = 64;
    for (int32_t i = 0; i < maxGen; ++i)
    {
        llama_token token = llama_sampler_sample(smpl, rt->context, -1);
        if (llama_vocab_is_eog(vocab, token) || token == llama_vocab_eos(vocab))
            break;

        char piece[256];
        int32_t pieceLen = llama_token_to_piece(vocab, token, piece, int32_t(sizeof(piece)), 0, true);
        if (pieceLen > 0)
            output.append(piece, piece + pieceLen);

        llama_sampler_accept(smpl, token);
        llama_batch next = llama_batch_get_one(&token, 1);
        if (llama_decode(rt->context, next) < 0)
            break;
    }

    llama_sampler_free(smpl);
    return output;
#else
    (void)prompt;
    return {};
#endif
}
}
