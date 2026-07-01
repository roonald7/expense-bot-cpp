#include "config.hpp"

#include <cstdlib>
#include <stdexcept>

namespace ragc {

Config Config::load()
{
    const char* raw_token = std::getenv("DISCORD_BOT_TOKEN");
    const char* raw_db_url = std::getenv("DATABASE_URL");
    const char* raw_gemini_key = std::getenv("GEMINI_API_KEY");
    const char* raw_guild_id = std::getenv("DISCORD_GUILD_ID");

    if (!raw_token) {
        throw std::runtime_error("Configuration error: DISCORD_BOT_TOKEN environment variable is missing.");
    }
    std::string token(raw_token);
    if (token.empty()) {
        throw std::runtime_error("Configuration error: DISCORD_BOT_TOKEN is empty.");
    }

    if (!raw_db_url) {
        throw std::runtime_error("Configuration error: DATABASE_URL environment variable is missing.");
    }
    std::string db_url(raw_db_url);
    if (db_url.empty() || (db_url.rfind("postgresql://", 0) != 0 && db_url.rfind("postgres://", 0) != 0)) {
        throw std::runtime_error("Configuration error: DATABASE_URL must start with 'postgresql://' or 'postgres://'.");
    }

    if (!raw_gemini_key) {
        throw std::runtime_error("Configuration error: GEMINI_API_KEY environment variable is missing.");
    }
    std::string gemini_key(raw_gemini_key);
    if (gemini_key.empty()) {
        throw std::runtime_error("Configuration error: GEMINI_API_KEY is empty.");
    }

    std::optional<std::string> guild_id;
    if (raw_guild_id) {
        std::string gid(raw_guild_id);
        if (!gid.empty()) {
            guild_id = std::move(gid);
        }
    }

    return {std::move(token), std::move(db_url), std::move(gemini_key), std::move(guild_id)};
}

} // namespace ragc
