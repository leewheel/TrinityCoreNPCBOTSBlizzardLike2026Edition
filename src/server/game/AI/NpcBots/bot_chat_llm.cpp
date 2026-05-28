#include "bot_chat_llm.h"

#include "Creature.h"
#include "bot_ai.h"
#include "botcommon.h"

#include <filesystem>
#include <mutex>
#include <optional>
#include <sstream>
#include <string_view>
#include <thread>
#include <vector>

#ifdef TRINITY_NPCBOT_LLM_EMBED
#include "llama.h"
#endif

namespace NpcBotChatLLM
{
#ifdef TRINITY_NPCBOT_LLM_EMBED
struct LlamaLoadSummary
{
    std::string arch;
    std::string modelName;
    std::string quant;
    std::string params;
    std::string fileSize;
    std::string nCtx;
    std::string nCtxTrain;
    std::string backendPtrs;
    std::string cpuMappedMiB;
    std::string cpuRepackMiB;
    std::string kvMiB;
    std::string recurrentMiB;
    std::string computeMiB;
    std::string outputMiB;
};

static std::mutex g_llamaLogMutex;
static bool g_captureLlamaSummary = false;
static LlamaLoadSummary g_llamaSummary;

static std::string Trim(std::string_view sv)
{
    while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' || sv.front() == '\r' || sv.front() == '\n'))
        sv.remove_prefix(1);
    while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t' || sv.back() == '\r' || sv.back() == '\n'))
        sv.remove_suffix(1);
    return std::string(sv);
}

static void CaptureByPrefix(std::string const& line, char const* prefix, std::string& out)
{
    std::string_view sv(line);
    std::string_view p(prefix);
    if (sv.rfind(p, 0) == 0)
        out = Trim(sv.substr(p.size()));
}

static void LlamaLogCallback(ggml_log_level level, char const* text, void* /*user_data*/)
{
    if (!text)
        return;

    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line))
    {
        line = Trim(line);
        if (line.empty() || line == ".")
            continue;

        {
            std::scoped_lock lk(g_llamaLogMutex);
            if (g_captureLlamaSummary)
            {
                CaptureByPrefix(line, "print_info: arch                  = ", g_llamaSummary.arch);
                CaptureByPrefix(line, "print_info: general.name          = ", g_llamaSummary.modelName);
                CaptureByPrefix(line, "print_info: file type   = ", g_llamaSummary.quant);
                CaptureByPrefix(line, "print_info: model params          = ", g_llamaSummary.params);
                CaptureByPrefix(line, "print_info: file size   = ", g_llamaSummary.fileSize);
                CaptureByPrefix(line, "llama_context: n_ctx         = ", g_llamaSummary.nCtx);
                CaptureByPrefix(line, "print_info: n_ctx_train           = ", g_llamaSummary.nCtxTrain);
                CaptureByPrefix(line, "llama_context: backend_ptrs.size() = ", g_llamaSummary.backendPtrs);
                CaptureByPrefix(line, "load_tensors:   CPU_Mapped model buffer size = ", g_llamaSummary.cpuMappedMiB);
                CaptureByPrefix(line, "load_tensors:   CPU_REPACK model buffer size = ", g_llamaSummary.cpuRepackMiB);
                CaptureByPrefix(line, "llama_kv_cache:        CPU KV buffer size = ", g_llamaSummary.kvMiB);
                CaptureByPrefix(line, "llama_memory_recurrent:        CPU RS buffer size = ", g_llamaSummary.recurrentMiB);
                CaptureByPrefix(line, "sched_reserve:        CPU compute buffer size = ", g_llamaSummary.computeMiB);
                CaptureByPrefix(line, "llama_context:        CPU  output buffer size = ", g_llamaSummary.outputMiB);
            }
        }

        // By leewheel 20260528 - keep logs concise: only retain real llama errors.
        if (level >= GGML_LOG_LEVEL_ERROR)
            BOT_LOG_ERROR("npcbots", "NpcBot LLM: {}", line);
    }
}
#endif

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
        llama_log_set(LlamaLogCallback, nullptr);
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
        BOT_LOG_WARN("npcbots", "NpcBot LLM model not found: {}", _modelPath);
        _runtime->loaded = false;
        return;
    }

    llama_model_params mparams = llama_model_default_params();
    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = 2048;
    // By leewheel 20260528 - CPU-friendly default: keep inference threads low to avoid starving gameplay/desktop.
    cparams.n_threads = 4;

    {
        std::scoped_lock lk(g_llamaLogMutex);
        g_captureLlamaSummary = true;
        g_llamaSummary = {};
    }

    _runtime->model = llama_load_model_from_file(_modelPath.c_str(), mparams);
    if (!_runtime->model)
    {
        std::scoped_lock lk(g_llamaLogMutex);
        g_captureLlamaSummary = false;
        BOT_LOG_ERROR("npcbots", "NpcBot LLM failed to load model: {}", _modelPath);
        _runtime->loaded = false;
        return;
    }

    _runtime->context = llama_new_context_with_model(_runtime->model, cparams);
    _runtime->loaded = (_runtime->context != nullptr);
    _runtime->loadedPath = _modelPath;
    {
        std::scoped_lock lk(g_llamaLogMutex);
        g_captureLlamaSummary = false;
    }
    if (_runtime->loaded)
    {
        LlamaLoadSummary summary;
        {
            std::scoped_lock lk(g_llamaLogMutex);
            summary = g_llamaSummary;
        }
        if (summary.nCtx.empty())
            summary.nCtx = std::to_string(llama_n_ctx(_runtime->context));

        BOT_LOG_INFO("npcbots",
            "模型已成功加载\n\n"
            "文件：{}\n"
            "架构：{}\n"
            "模型名：{}\n"
            "量化：{}\n"
            "参数量：{}\n"
            "文件大小：{}\n"
            "LLM线程：{}\n"
            "推理设备\n\n"
            "纯 CPU（所有层都分配到 CPU）\n"
            "没有 GPU 后端参与（backend_ptrs.size() = {}，CPU only）\n"
            "关键运行配置\n\n"
            "上下文：n_ctx = {}\n"
            "n_ctx_train = {}（日志提示当前仅用到训练上限的一小部分，这是正常提示）\n"
            "内存占用（日志中的主要块）\n\n"
            "模型映射缓冲：CPU_Mapped model buffer = {}\n"
            "模型重排缓冲：CPU_REPACK model buffer = {}\n"
            "KV Cache：{}\n"
            "Recurrent 状态：{}\n"
            "计算缓冲：{}\n"
            "输出缓冲：{}",
            _modelPath,
            summary.arch.empty() ? "unknown" : summary.arch,
            summary.modelName.empty() ? "unknown" : summary.modelName,
            summary.quant.empty() ? "unknown" : summary.quant,
            summary.params.empty() ? "unknown" : summary.params,
            summary.fileSize.empty() ? "unknown" : summary.fileSize,
            cparams.n_threads,
            summary.backendPtrs.empty() ? "1" : summary.backendPtrs,
            summary.nCtx.empty() ? "unknown" : summary.nCtx,
            summary.nCtxTrain.empty() ? "unknown" : summary.nCtxTrain,
            summary.cpuMappedMiB.empty() ? "unknown" : summary.cpuMappedMiB,
            summary.cpuRepackMiB.empty() ? "unknown" : summary.cpuRepackMiB,
            summary.kvMiB.empty() ? "unknown" : summary.kvMiB,
            summary.recurrentMiB.empty() ? "unknown" : summary.recurrentMiB,
            summary.computeMiB.empty() ? "unknown" : summary.computeMiB,
            summary.outputMiB.empty() ? "unknown" : summary.outputMiB);
    }
    else
        BOT_LOG_ERROR("npcbots", "NpcBot LLM failed to create context: {}", _modelPath);
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

    std::string fullPrompt = "你是一名资深魔兽世界玩家，只用简体中文、简短口语回复，不谈政治。"
                             "你必须始终保持玩家口吻，不得承认自己是机器人、AI、程序或脚本。"
                             "你尽可能使用简短语句发言，建议每次回复不超过20个汉字。";
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
    // By leewheel 20260528 - limit generation length to reduce per-message CPU time.
    constexpr int32_t maxGen = 32;
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
