#include "config.hpp"

#include <cstdlib>
#include <stdexcept>
#include <optional>

namespace ragc {

auto Config::load() -> Config
{
    const char* raw_token      = std::getenv("DISCORD_BOT_TOKEN");
    const char* raw_db_url     = std::getenv("DATABASE_URL");
    const char* raw_gemini_key = std::getenv("GEMINI_API_KEY");
    const char* raw_guild_id   = std::getenv("DISCORD_GUILD_ID");

    // OpenAI-compatible AI backend vars (required for OllamaClient)
    const char* raw_ai_url   = std::getenv("AI_API_URL");
    const char* raw_ai_key   = std::getenv("AI_API_KEY");
    const char* raw_ai_model = std::getenv("AI_MODEL_NAME");

    if (raw_token == nullptr) {
        throw std::runtime_error("Configuration error: DISCORD_BOT_TOKEN environment variable is missing.");
    }
    std::string token(raw_token);
    if (token.empty()) {
        throw std::runtime_error("Configuration error: DISCORD_BOT_TOKEN is empty.");
    }

    if (raw_db_url == nullptr) {
        throw std::runtime_error("Configuration error: DATABASE_URL environment variable is missing.");
    }
    std::string db_url(raw_db_url);
    if (db_url.empty() || (!db_url.starts_with("postgresql://") && !db_url.starts_with("postgres://"))) {
        throw std::runtime_error("Configuration error: DATABASE_URL must start with 'postgresql://' or 'postgres://'.");
    }

    // ---------------------------------------------------------------------------
    // Optional legacy Gemini key — allowed to be empty when using OllamaClient
    // ---------------------------------------------------------------------------
    std::string gemini_key;
    if (raw_gemini_key != nullptr) {
        gemini_key = raw_gemini_key;
    }

    // ---------------------------------------------------------------------------
    // OpenAI-compatible AI backend
    // ---------------------------------------------------------------------------
    if (raw_ai_url == nullptr || std::string(raw_ai_url).empty()) {
        throw std::runtime_error("Configuration error: AI_API_URL environment variable is missing.");
    }
    if (raw_ai_key == nullptr || std::string(raw_ai_key).empty()) {
        throw std::runtime_error("Configuration error: AI_API_KEY environment variable is missing.");
    }
    if (raw_ai_model == nullptr || std::string(raw_ai_model).empty()) {
        throw std::runtime_error("Configuration error: AI_MODEL_NAME environment variable is missing.");
    }

    std::string ai_api_url(raw_ai_url);
    std::string ai_api_key(raw_ai_key);
    std::string ai_model_name(raw_ai_model);

    // ---------------------------------------------------------------------------
    // Optional guild ID (for slash command registration scope)
    // ---------------------------------------------------------------------------
    std::optional<std::string> guild_id;
    if (raw_guild_id != nullptr) {
        std::string gid(raw_guild_id);
        if (!gid.empty()) {
            guild_id = std::move(gid);
        }
    }

    return {std::move(token),
            std::move(db_url),
            std::move(gemini_key),
            std::move(ai_api_url),
            std::move(ai_api_key),
            std::move(ai_model_name),
            std::move(guild_id)};
}

} // namespace ragc
