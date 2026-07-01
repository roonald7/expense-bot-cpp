#include "ollama_client.hpp"

#include <dpp/dpp.h>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace ragc {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

OllamaClient::OllamaClient(dpp::cluster&    bot,
                            std::string_view api_url,
                            std::string_view api_key,
                            std::string_view model_name)
    : bot_(bot), api_url_(api_url), api_key_(api_key), model_name_(model_name)
{
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

nlohmann::json OllamaClient::parse_expense(std::string_view raw_text)
{
    nlohmann::json body = build_request_body(raw_text);

    // Authorization header for OpenAI-compatible endpoints.
    // For local Ollama the value is simply "Bearer ollama".
    std::multimap<std::string, std::string> headers;
    headers.emplace("Authorization", "Bearer " + api_key_);

    auto promise = std::make_shared<std::promise<dpp::http_request_completion_t>>();
    std::future<dpp::http_request_completion_t> future = promise->get_future();

    bot_.request(
        api_url_,
        dpp::m_post,
        [promise](const dpp::http_request_completion_t& completion) {
            promise->set_value(completion);
        },
        body.dump(),
        "application/json",
        headers);

    auto response = future.get();

    if (response.status != 200) {
        throw std::runtime_error(
            "AI API Error (HTTP " + std::to_string(response.status) + "): " + response.body);
    }

    // Parse the outer API response and extract the model's reply text.
    nlohmann::json api_result = nlohmann::json::parse(response.body);
    std::string    content    = extract_content(api_result);

    // The model is instructed to reply with raw JSON; parse and return it.
    return nlohmann::json::parse(content);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

nlohmann::json OllamaClient::build_request_body(std::string_view user_prompt) const
{
    const std::string system_prompt =
        "You are an expense parsing assistant. "
        "When given a natural language description of an expense, "
        "respond ONLY with a valid JSON object containing exactly these fields: "
        "amount (number), currency (string, e.g. USD), "
        "category (string), description (string). "
        "Do not include any explanation or markdown — raw JSON only.";

    return nlohmann::json{
        {"model", model_name_},
        {"stream", false},
        {"messages",
         nlohmann::json::array({
             {{"role", "system"}, {"content", system_prompt}},
             {{"role", "user"}, {"content", std::string(user_prompt)}},
         })},
    };
}

// static
std::string OllamaClient::extract_content(const nlohmann::json& response)
{
    try {
        return response.at("choices").at(0).at("message").at("content").get<std::string>();
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error(
            std::string("AI response parsing failed: ") + e.what() +
            " | body: " + response.dump());
    }
}

} // namespace ragc
