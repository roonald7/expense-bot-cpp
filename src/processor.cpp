#include "processor.hpp"
#include <iostream>

namespace ragc {

Processor::Processor(DatabaseProcessor& db, GeminiClient& ai) : db_(db), ai_(ai)
{
}

std::optional<Expense> Processor::process_message(std::string_view raw_content, int64_t user_id)
{
    // 1. Persist the raw request immediately (Security against crashes/503s)
    int64_t request_id = db_.save_pending_request(user_id, std::string(raw_content));

    try {
        // 2. Send to Gemini for semantic parsing
        nlohmann::json extracted = ai_.parse_expense(raw_content);

        if (extracted.is_array()) {
            if (extracted.empty()) {
                throw std::runtime_error("Empty Gemini response");
            }
            extracted = extracted[0];
        }

        // 3. Map JSON to Expense
        Expense expense{};
        expense.amount = extracted.value("amount", 0.0);
        expense.currency = extracted.value("currency", "USD");
        expense.category = extracted.value("category", "Misc");
        expense.description = extracted.value("description", std::string(raw_content));
        expense.user_id = user_id;

        // 4. Persist to PostgreSQL
        if (db_.save_expense(expense)) {
            db_.update_pending_status(request_id, "COMPLETED");
            return expense;
        }

        db_.update_pending_status(request_id, "FAILED", "Failed to save expense to DB");
        return std::nullopt;
    } catch (const std::exception& e) {
        std::string err = e.what();
        std::cerr << "Processor error: " << err << "\n";

        // If Gemini is busy or down (429, 503, Timeout), schedule a persistent
        // retry
        if (err.find("429") != std::string::npos || err.find("503") != std::string::npos ||
            err.find("timeout") != std::string::npos) {
            constexpr int INITIAL_RETRY_DELAY_MINS = 1;
            db_.schedule_retry(request_id, INITIAL_RETRY_DELAY_MINS, err);
            std::cout << "[RETRY] API busy. Scheduled persistent retry for " << request_id << " in 1m\n";
        } else {
            db_.update_pending_status(request_id, "FAILED", err);
        }

        return std::nullopt;
    }
}

void Processor::retry_background_tasks()
{
    auto tasks = db_.get_ready_for_retry();
    if (tasks.empty()) {
        return;
    }

    std::cout << "[RETRY] Attempting to process " << tasks.size() << " ready tasks...\n";

    for (const auto& task : tasks) {
        try {
            nlohmann::json extracted = ai_.parse_expense(task.raw_input);
            if (extracted.is_array() && !extracted.empty()) {
                extracted = extracted[0];
            }

            Expense expense{};
            expense.amount = extracted.value("amount", 0.0);
            expense.currency = extracted.value("currency", "USD");
            expense.category = extracted.value("category", "Misc");
            expense.description = extracted.value("description", task.raw_input);
            expense.user_id = task.user_id;

            if (db_.save_expense(expense)) {
                db_.update_pending_status(task.id, "COMPLETED");
                std::cout << "[RETRY] Task " << task.id << " COMPLETED successfully.\n";
            } else {
                constexpr int DB_FAIL_RETRY_DELAY_MINS = 10;
                db_.schedule_retry(task.id, DB_FAIL_RETRY_DELAY_MINS, "DB Save failed during retry");
            }
        } catch (const std::exception& e) {
            std::string err = e.what();
            // If it's still a 503, push it back further (15 minutes this time)
            constexpr int RETRY_503_DELAY_MINS = 15;
            constexpr int RETRY_OTHER_DELAY_MINS = 30;
            int delay = (err.find("503") != std::string::npos) ? RETRY_503_DELAY_MINS : RETRY_OTHER_DELAY_MINS;
            db_.schedule_retry(task.id, delay, err);
            std::cerr << "[RETRY] Task " << task.id << " failed again: " << err << ". Postponed by " << delay << "m\n";
        }
    }
}

} // namespace ragc
