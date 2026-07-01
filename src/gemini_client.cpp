#include "gemini_client.hpp"

#include <dpp/dpp.h>
#include <future>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>

namespace ragc {

GeminiClient::GeminiClient(dpp::cluster& bot, std::string_view api_key) : bot_(bot), api_key_(api_key)
{
}

nlohmann::json GeminiClient::parse_expense(std::string_view raw_text)
{
    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/"
                      "gemini-2.0-flash-lite:generateContent?key=" +
                      api_key_;

    // Construct payload explicitly to avoid nested bracket errors
    nlohmann::json payload;

    nlohmann::json part;
    part["text"] = "Extract the following expense into a JSON object with "
                   "fields: amount (number), currency (string, e.g. USD), "
                   "category (string), description (string). Input: " +
                   std::string(raw_text);

    nlohmann::json content;
    content["parts"] = nlohmann::json::array({part});

    payload["contents"] = nlohmann::json::array({content});
    payload["generationConfig"]["response_mime_type"] = "application/json";

    // Perform the asynchronous request and wait for the future
    auto promise = std::make_shared<std::promise<dpp::http_request_completion_t>>();
    std::future<dpp::http_request_completion_t> future = promise->get_future();

    bot_.request(
        url,
        dpp::m_post,
        [promise](const dpp::http_request_completion_t& completion) { promise->set_value(completion); },
        payload.dump(),
        "application/json");

    auto response = future.get();

    if (response.status != 200) {
        throw std::runtime_error("Gemini API Error (HTTP " + std::to_string(response.status) + "): " + response.body);
    }

    nlohmann::json result = nlohmann::json::parse(response.body);

    return nlohmann::json::parse(result["candidates"][0]["content"]["parts"][0]["text"].get<std::string>());
}

} // namespace ragc
