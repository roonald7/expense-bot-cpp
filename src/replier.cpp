#include "replier.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace ragc {

dpp::embed Replier::create_expense_embed(const Expense& expense)
{
    dpp::embed embed = dpp::embed()
                           .set_color(dpp::colors::grass_green)
                           .set_title("Expense Registered Successfully")
                           .set_author("ExpenseBot", "https://discord.com", "")
                           .set_timestamp(time(nullptr));

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << expense.amount;
    std::string amount_str = ss.str() + " " + expense.currency;

    std::string emoji = get_category_emoji(expense.category);

    embed.add_field("💰 Amount", amount_str, true);
    embed.add_field("🏷️ Category", emoji + " " + expense.category, true);
    embed.add_field("📝 Description", expense.description, false);

    embed.set_footer(dpp::embed_footer().set_text("Total logged precisely via Gemini AI"));

    return embed;
}

std::string Replier::get_category_emoji(const std::string& category)
{
    std::string cat = category;
    std::transform(
        cat.begin(), cat.end(), cat.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (cat.find("food") != std::string::npos || cat.find("eat") != std::string::npos)
        return "🍔";
    if (cat.find("travel") != std::string::npos || cat.find("uber") != std::string::npos ||
        cat.find("transport") != std::string::npos)
        return "🚗";
    if (cat.find("rent") != std::string::npos || cat.find("home") != std::string::npos)
        return "🏠";
    if (cat.find("sub") != std::string::npos || cat.find("netflix") != std::string::npos ||
        cat.find("spotify") != std::string::npos)
        return "📺";
    if (cat.find("shopping") != std::string::npos || cat.find("buy") != std::string::npos)
        return "🛍️";
    if (cat.find("health") != std::string::npos || cat.find("med") != std::string::npos)
        return "🏥";

    return "📦"; // Default: Miscellaneous
}

} // namespace ragc
