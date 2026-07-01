#pragma once

#include "ai_client.hpp"

#include <string>
#include <string_view>

namespace dpp {
class cluster;
}

namespace ragc {

/**
 * @brief AIClient implementation for any OpenAI-compatible endpoint.
 *
 * Compatible with local runtimes (Ollama, LM Studio, LocalAI) and any
 * cloud provider that exposes the /v1/chat/completions API.
 *
 * Configuration is read from environment variables at construction time:
 *   - AI_API_URL   — full endpoint, e.g. "http://127.0.0.1:11434/v1/chat/completions"
 *   - AI_API_KEY   — bearer token; use "ollama" for local Ollama instances
 *   - AI_MODEL_NAME — model tag, e.g. "qwen2.5:1.5b"
 */
class OllamaClient : public AIClient
{
public:
    /**
     * @brief Construct the client from pre-loaded config fields.
     *
     * @param bot        D++ cluster used for HTTP requests.
     * @param api_url    Full chat-completions endpoint URL.
     * @param api_key    Bearer token (use "ollama" for local instances).
     * @param model_name Model identifier, e.g. "qwen2.5:1.5b".
     */
    OllamaClient(dpp::cluster& bot,
                 std::string_view api_url,
                 std::string_view api_key,
                 std::string_view model_name);

    /**
     * @brief Send raw user text to the model and return a structured expense JSON.
     * @throw std::runtime_error on HTTP or JSON parsing failures.
     */
    nlohmann::json parse_expense(std::string_view raw_text) override;

private:
    dpp::cluster& bot_;
    std::string   api_url_;
    std::string   api_key_;
    std::string   model_name_;

    /// Build the OpenAI-compatible /v1/chat/completions request body.
    nlohmann::json build_request_body(std::string_view user_prompt) const;

    /// Extract the assistant message text from the API response JSON.
    static std::string extract_content(const nlohmann::json& response);
};

} // namespace ragc
