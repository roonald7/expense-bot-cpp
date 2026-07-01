#pragma once

#include <string_view>
#include "connection_pool.hpp"
#include "models/expense.hpp"

namespace ragc {

class DatabaseProcessor
{
public:
    /**
     * @brief Construct DatabaseProcessor backed by a thread-safe connection pool.
     *
     * @param db_url    PostgreSQL connection string.
     * @param pool_size Number of concurrent DB connections (default: 4).
     */
    explicit DatabaseProcessor(std::string_view db_url, std::size_t pool_size = 4);

    /** Run sequential schema migrations on startup. */
    void init_tables();

    /** Thread-safe: each call acquires its own connection from the pool. */
    bool save_expense(const Expense& expense);

    // --- Persistency & Retry methods ---
    int64_t save_pending_request(int64_t user_id, const std::string& input);
    void update_pending_status(int64_t request_id, const std::string& status, const std::string& error = "");
    void schedule_retry(int64_t request_id, int minutes_delay, const std::string& error);

    struct PendingTask
    {
        int64_t id;
        int64_t user_id;
        std::string raw_input;
    };
    std::vector<PendingTask> get_ready_for_retry();

private:
    ConnectionPool pool_;
};

} // namespace ragc
