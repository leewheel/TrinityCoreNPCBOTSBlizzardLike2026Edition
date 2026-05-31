#include "bot_chat_llm.h"

#include "Creature.h"
#include "bot_ai.h"
#include "botcommon.h"

#include <algorithm>
#include <array>
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

// By leewheel 20260528 - Qwen3.x / reasoning models may emit thinking blocks; never show them to players.
static char const kImEndStr[] = { '<', '|', 'i', 'm', '_', 'e', 'n', 'd', '|', '>', '\0' };
static char const kQwenThinkStartStr[] = { '<', 't', 'h', 'i', 'n', 'k', '>', '\0' };
static char const kQwenThinkEndStr[] = { '<', '/', 't', 'h', 'i', 'n', 'k', '>', '\0' };
static char const kRedactedThinkStartStr[] = { '<', 'r', 'e', 'd', 'a', 'c', 't', 'e', 'd', '_', 't', 'h', 'i', 'n', 'k', 'i', 'n', 'g', '>', '\0' };
static char const kRedactedThinkEndStr[] = { '<', '/', 'r', 'e', 'd', 'a', 'c', 't', 'e', 'd', '_', 't', 'h', 'i', 'n', 'k', 'i', 'n', 'g', '>', '\0' };
static std::string_view const kImEnd = kImEndStr;
static std::string_view const kQwenThinkStart = kQwenThinkStartStr;
static std::string_view const kQwenThinkEnd = kQwenThinkEndStr;
static std::string_view const kRedactedThinkStart = kRedactedThinkStartStr;
static std::string_view const kRedactedThinkEnd = kRedactedThinkEndStr;
static std::array<std::string_view, 4> const kThinkStartTags = {
    kRedactedThinkStart,
    kQwenThinkStart,
    "[THINK]",
    "<|channel>thought",
};
static std::array<std::string_view, 4> const kThinkEndTags = {
    kRedactedThinkEnd,
    kQwenThinkEnd,
    "[/THINK]",
    "<channel|>",
};

static void EraseAllSubstrings(std::string& text, std::string_view needle)
{
    if (needle.empty())
        return;
    for (size_t pos = 0; (pos = text.find(needle, pos)) != std::string::npos;)
        text.erase(pos, needle.size());
}

static std::string StripReasoningContent(std::string text)
{
    for (size_t si = 0; si < kThinkStartTags.size(); ++si)
    {
        std::string_view const start = kThinkStartTags[si];
        std::string_view const end = kThinkEndTags[si];
        for (;;)
        {
            size_t const begin = text.find(start);
            if (begin == std::string::npos)
                break;
            size_t const close = text.find(end, begin + start.size());
            if (close == std::string::npos)
            {
                text.erase(begin);
                break;
            }
            text.erase(begin, close + end.size() - begin);
        }
    }

    for (std::string_view tag : kThinkStartTags)
        EraseAllSubstrings(text, tag);
    for (std::string_view tag : kThinkEndTags)
        EraseAllSubstrings(text, tag);

    EraseAllSubstrings(text, "/think");
    EraseAllSubstrings(text, "/no_think");

    return Trim(text);
}

static bool ContainsThinkingMarker(std::string_view text)
{
    for (std::string_view tag : kThinkStartTags)
        if (text.find(tag) != std::string_view::npos)
            return true;
    for (std::string_view tag : kThinkEndTags)
        if (text.find(tag) != std::string_view::npos)
            return true;
    return false;
}

static std::string BuildQwenChatPromptFallback(std::string const& scenePrompt)
{
    // Fallback when jinja chat template is unavailable (approximates enable_thinking=false).
    std::ostringstream oss;
    oss << "<|im_start|>system\n"
        << "你是魔兽世界资深玩家。只输出一句简体中文游戏口语。"
        << "直接回应场景里队友/玩家刚说的话，可以寒暄、接梗、开玩笑。"
        << "必须说完整的一句话，以句号/问号/感叹号收束，不要说到一半就停。"
        << "禁止编造刚施放过技能、buff、护盾、专精名称；没发生的事不要提。"
        << "禁止复读「保持节奏」「稳住节奏」「等队友就位」等罐头战术句，除非场景明确正在激烈团本战斗。"
        << "不要输出思考过程，不要输出任何XML或特殊标签。"
        << "不得承认自己是机器人、AI、程序或脚本。建议50-80个汉字，在有限字数内把意思表达清楚。"
        << kImEnd << "\n"
        << "<|im_start|>user\n"
        << "场景信息：" << scenePrompt << "\n"
        << kImEnd << "\n"
        << "<|im_start|>assistant\n"
        << kQwenThinkStart << "\n\n" << kQwenThinkEnd << "\n\n";
    return oss.str();
}

static std::string FilterGenerationPiece(std::string& carry, std::string_view piece, bool& inThinking)
{
    carry.append(piece);
    std::string visible;
    visible.reserve(carry.size());

    while (!carry.empty())
    {
        if (inThinking)
        {
            size_t endPos = std::string::npos;
            size_t endLen = 0;
            for (std::string_view endTag : kThinkEndTags)
            {
                size_t const p = carry.find(endTag);
                if (p != std::string::npos && (endPos == std::string::npos || p < endPos))
                {
                    endPos = p;
                    endLen = endTag.size();
                }
            }
            if (endPos == std::string::npos)
                break;
            carry.erase(0, endPos + endLen);
            inThinking = false;
            continue;
        }

        size_t startPos = std::string::npos;
        size_t startLen = 0;
        for (std::string_view startTag : kThinkStartTags)
        {
            size_t const p = carry.find(startTag);
            if (p != std::string::npos && (startPos == std::string::npos || p < startPos))
            {
                startPos = p;
                startLen = startTag.size();
            }
        }

        if (startPos == std::string::npos)
        {
            visible.append(carry);
            carry.clear();
            break;
        }

        if (startPos > 0)
            visible.append(carry, 0, startPos);
        carry.erase(0, startPos + startLen);
        inThinking = true;
    }

    return visible;
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
    bool useGpu = false;
    std::string loadedPath;
};

Engine& Engine::Instance()
{
    static Engine engine;
    return engine;
}

void Engine::Configure(bool enabled, std::string modelPath, bool useGpu)
{
    if (!_runtime)
        _runtime = new RuntimeState();

    _enabled = enabled;
    _useGpu = useGpu;
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

    if (_runtime->loaded && _runtime->loadedPath == _modelPath && _runtime->useGpu == useGpu)
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

    bool const gpuAvailable = llama_supports_gpu_offload();
    if (useGpu)
    {
        if (!gpuAvailable)
        {
            BOT_LOG_ERROR("npcbots",
                "NpcBot.Chat.LLM.Device=gpu but no GPU backend in this worldserver build. "
                "Rebuild with CMake -DWITH_NPCBOT_LLM_GPU=ON (requires NVIDIA CUDA Toolkit). "
                "LLM chat is disabled until GPU build is used.");
            _runtime->loaded = false;
            return;
        }
        mparams.n_gpu_layers = -1; // all layers to VRAM
    }
    else
        mparams.n_gpu_layers = 0;

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
    _runtime->useGpu = gpuAvailable && mparams.n_gpu_layers != 0;

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

        std::string deviceLine;
        if (_runtime->useGpu)
            deviceLine = Bcore::StringFormat(
                "GPU 加速（n_gpu_layers=all，backend_ptrs.size() = {}）",
                summary.backendPtrs.empty() ? "?" : summary.backendPtrs);
        else if (useGpu && !gpuAvailable)
            deviceLine = Bcore::StringFormat(
                "纯 CPU（配置为 gpu，但未编译 GPU 后端；backend_ptrs.size() = {}）",
                summary.backendPtrs.empty() ? "1" : summary.backendPtrs);
        else
            deviceLine = Bcore::StringFormat(
                "纯 CPU（n_gpu_layers=0，backend_ptrs.size() = {}）",
                summary.backendPtrs.empty() ? "1" : summary.backendPtrs);

        BOT_LOG_INFO("npcbots",
            "模型已成功加载\n\n"
            "文件：{}\n"
            "架构：{}\n"
            "模型名：{}\n"
            "量化：{}\n"
            "参数量：{}\n"
            "文件大小：{}\n"
            "LLM线程：{}\n"
            "推理设备：{}\n"
            "配置项 NpcBot.Chat.LLM.Device = {}\n"
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
            deviceLine,
            useGpu ? "gpu" : "cpu",
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

static size_t CountUtf8Codepoints(std::string_view text)
{
    size_t count = 0;
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
        ++count;
        i += n;
    }
    return count;
}

static bool EndsWithSentencePunct(std::string_view text)
{
    return text.ends_with("。") || text.ends_with("！") || text.ends_with("？") ||
        text.ends_with("~") || text.ends_with("…") || text.ends_with(".");
}

static std::string TrimIncompleteSentenceTail(std::string text)
{
    if (text.empty() || EndsWithSentencePunct(text))
        return text;

    static std::array<std::string_view, 5> const punct = { "。", "！", "？", "~", "…" };
    size_t bestCut = std::string::npos;
    for (std::string_view p : punct)
    {
        size_t pos = text.rfind(p);
        if (pos != std::string::npos)
            bestCut = std::max(bestCut, pos + p.size());
    }

    if (bestCut != std::string::npos && bestCut >= text.size() / 2)
        text.resize(bestCut);
    return text;
}

std::string Engine::GenerateReply(Creature const* bot, std::string const& prompt) const
{
    if (!bot || !IsEnabled())
        return {};

    RuntimeState* rt = _runtime;
    if (!rt || !rt->loaded)
        return {};

#ifdef TRINITY_NPCBOT_LLM_EMBED
    if (!rt->model || !rt->context)
        return {};

    std::scoped_lock lk(rt->inferMutex);
    const llama_vocab* vocab = llama_model_get_vocab(rt->model);
    if (!vocab)
        return {};

    std::string const fullPrompt = BuildQwenChatPromptFallback(prompt);

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
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED + bot->GetEntry()));

    std::string output;
    std::string carry;
    bool inThinking = false;
    output.reserve(256);
    carry.reserve(128);
    // By leewheel 20260530 - count UTF-8 codepoints (not bytes); 72 bytes was ~24 Han chars and cut mid-sentence.
    constexpr int32_t maxGen = 160;
    constexpr size_t targetCodepoints = 80;
    constexpr size_t hardCodepointCap = 100;
    size_t visibleCodepoints = 0;
    for (int32_t i = 0; i < maxGen; ++i)
    {
        llama_token token = llama_sampler_sample(smpl, rt->context, -1);
        if (llama_vocab_is_eog(vocab, token) || token == llama_vocab_eos(vocab))
            break;

        char piece[256];
        int32_t pieceLen = llama_token_to_piece(vocab, token, piece, int32_t(sizeof(piece)), 0, true);
        if (pieceLen > 0)
        {
            std::string const visible = FilterGenerationPiece(carry, std::string_view(piece, pieceLen), inThinking);
            if (!visible.empty())
            {
                output += visible;
                visibleCodepoints = CountUtf8Codepoints(output);
            }
        }

        llama_sampler_accept(smpl, token);
        llama_batch next = llama_batch_get_one(&token, 1);
        if (llama_decode(rt->context, next) < 0)
            break;

        if (visibleCodepoints >= targetCodepoints && EndsWithSentencePunct(output))
            break;
        if (visibleCodepoints >= hardCodepointCap)
            break;
    }

    llama_sampler_free(smpl);
    output = StripReasoningContent(std::move(output));
    output = TrimIncompleteSentenceTail(std::move(output));
    if (output.empty() || ContainsThinkingMarker(output))
        return {};
    return output;
#else
    (void)prompt;
    return {};
#endif
}
}
