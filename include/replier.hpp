#pragma once

#include <dpp/dpp.h>
#include <string>
#include "models/expense.hpp"

namespace ragc {

/**
 * @brief Responsible for formatting semantic data into visual Discord Rich Embeds.
 */
class Replier
{
public:
    /**
     * @brief Creates a beautiful "Transaction Receipt" card.
     *
     * @param expense The parsed and persisted expense data.
     * @return dpp::embed The formatted Discord embed.
     */
    static dpp::embed create_expense_embed(const Expense& expense);

private:
    /**
     * @brief Maps string categories to relevant Discord emojis.
     */
    static std::string get_category_emoji(const std::string& category);
};

} // namespace ragc
