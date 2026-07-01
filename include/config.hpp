#pragma once

#include <optional>
#include <string>

namespace ragc {

/**
 * @brief Config configuration options loaded and validated from environment variables.
 */
struct Config
{
    std::string token;
    std::string db_url;
    std::string gemini_key;  ///< Legacy Gemini API key (kept for GeminiClient backward compat)

    // --- OpenAI-compatible AI backend (Ollama / LM Studio / cloud proxies) ---
    std::string ai_api_url;    ///< e.g. "http://127.0.0.1:11434/v1/chat/completions"
    std::string ai_api_key;    ///< Bearer token; "ollama" for local instances
    std::string ai_model_name; ///< Model tag, e.g. "qwen2.5:1.5b"

    std::optional<std::string> guild_id;

    /**
     * @brief Load environment configurations and apply strict format/existence validation.
     * @throw std::runtime_error if critical variables are missing or incorrectly structured.
     */
    static Config load();
};

} // namespace ragc
