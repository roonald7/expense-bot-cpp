#pragma once

#include "ai_client.hpp"

#include <string>
#include <string_view>

namespace dpp {
class cluster;
}

namespace ragc {

/**
 * @brief AIClient implementation for the Google Gemini REST API.
 *
 * Derives from AIClient so it can be swapped with OllamaClient
 * (or any future backend) without touching Processor or main.cpp.
 */
class GeminiClient : public AIClient
{
public:
    /**
     * @brief Construct a new Gemini Client
     *
     * @param bot Reference to the D++ cluster.
     * @param api_key The Gemini API key.
     */
    explicit GeminiClient(dpp::cluster& bot, std::string_view api_key);

    /**
     * @brief Sends user input to Gemini and requests a structured JSON response.
     */
    nlohmann::json parse_expense(std::string_view user_input) override;

private:
    dpp::cluster& bot_;
    std::string api_key_;

    // Construct the JSON body for the Gemini API request
    std::string build_request_body(std::string_view user_input) const;
};

} // namespace ragc
