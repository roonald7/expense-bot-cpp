#pragma once

#include <nlohmann/json.hpp>
#include <string_view>

namespace ragc {

/**
 * @brief Abstract interface for LLM AI clients.
 *
 * Concrete implementations (OllamaClient, GeminiClient, …) derive from this
 * class and fulfil the single contract: given a raw user sentence, return a
 * structured JSON object representing the parsed expense.
 *
 * Swapping AI backends only requires:
 *   1. Implementing a new subclass of AIClient.
 *   2. Passing the new instance to Processor / main.cpp.
 *   No other files need to change.
 */
class AIClient
{
public:
    virtual ~AIClient() = default;

    /**
     * @brief Parse raw user text into a structured expense JSON.
     *
     * Expected JSON shape:
     * @code
     * {
     *   "amount":      <number>,
     *   "currency":    <string>,   // e.g. "USD"
     *   "category":    <string>,
     *   "description": <string>
     * }
     * @endcode
     *
     * @param raw_text  The informal user message (e.g. "spent 50 bucks on lunch").
     * @return nlohmann::json with the fields above.
     * @throw std::runtime_error on HTTP or parsing failures.
     */
    virtual nlohmann::json parse_expense(std::string_view raw_text) = 0;
};

} // namespace ragc
