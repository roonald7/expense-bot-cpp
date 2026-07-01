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
    std::string gemini_key;
    std::optional<std::string> guild_id;

    /**
     * @brief Load environment configurations and apply strict format/existence validation.
     * @throw std::runtime_error if critical variables are missing or incorrectly structured.
     */
    static Config load();
};

} // namespace ragc
