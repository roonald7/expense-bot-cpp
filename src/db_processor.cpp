#include "db_processor.hpp"
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace ragc {

DatabaseProcessor::DatabaseProcessor(std::string_view db_url, std::size_t pool_size) : pool_(db_url, pool_size)
{
}

void DatabaseProcessor::init_tables()
{
    try {
        // Migrations run once at startup — acquire a dedicated connection
        auto guard = pool_.acquire();
        pqxx::work tx(guard.get());

        // 1. Ensure migration tracking table exists
        tx.exec(R"(
        CREATE TABLE IF NOT EXISTS schema_migrations (
            version INTEGER PRIMARY KEY,
            applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )");

        // 2. Sequential migration definitions
        const std::vector<std::pair<int, std::string>> migrations = {
            {1, R"(
            CREATE TABLE IF NOT EXISTS expenses (
                id SERIAL PRIMARY KEY,
                amount NUMERIC(12, 2) NOT NULL,
                currency VARCHAR(10) NOT NULL,
                category VARCHAR(50) NOT NULL,
                description TEXT,
                user_id BIGINT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )"},
            {2, R"(
            CREATE TABLE IF NOT EXISTS pending_requests (
                id SERIAL PRIMARY KEY,
                user_id BIGINT NOT NULL,
                raw_input TEXT NOT NULL,
                status VARCHAR(20) DEFAULT 'PENDING', -- PENDING, COMPLETED, FAILED
                retry_count INTEGER DEFAULT 0,
                last_error TEXT,
                next_retry_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )"}
            // Add future migrations here: {2, "ALTER TABLE ..."}
        };

        // 3. Determine current schema version
        pqxx::result res = tx.exec("SELECT MAX(version) FROM schema_migrations");
        int current_version = 0;
        if (!res.empty() && !res[0][0].is_null()) {
            current_version = res[0][0].as<int>();
        }

        // 4. Apply pending migrations
        for (const auto& [version, sql] : migrations) {
            if (version > current_version) {
                std::cout << "[DB] Applying migration v" << version << "..." << std::endl;
                tx.exec(sql);
                tx.exec("INSERT INTO schema_migrations (version) VALUES ($1)", pqxx::params{version});
            }
        }

        tx.commit();
        std::cout << "[DB] Schema up to date." << std::endl;
    } catch (const pqxx::sql_error& e) {
        std::cerr << "[DB] Migration Error: " << e.what() << std::endl;
        std::cerr << "[DB] Failed Query: " << e.query() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DB] Error during schema initialization: " << e.what() << std::endl;
    }
}

bool DatabaseProcessor::save_expense(const Expense& expense)
{
    try {
        // Each call gets its own connection — fully thread-safe
        auto guard = pool_.acquire();
        pqxx::work tx(guard.get());

        tx.exec("INSERT INTO expenses (amount, currency, category, description, "
                "user_id) "
                "VALUES ($1, $2, $3, $4, $5)",
                pqxx::params{expense.amount, expense.currency, expense.category, expense.description, expense.user_id});

        tx.commit();
        return true;
    } catch (const pqxx::sql_error& e) {
        std::cerr << "[DB] SQL Error: " << e.what() << std::endl;
        std::cerr << "[DB] Failed Query: " << e.query() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[DB] Standard Error: " << e.what() << std::endl;
        return false;
    }
}

int64_t DatabaseProcessor::save_pending_request(int64_t user_id, const std::string& input)
{
    try {
        auto conn_guard = pool_.acquire();
        pqxx::work tx(conn_guard.get());

        pqxx::result r = tx.exec("INSERT INTO pending_requests (user_id, "
                                 "raw_input) VALUES ($1, $2) RETURNING id",
                                 pqxx::params{user_id, input});

        tx.commit();
        return r[0][0].as<int64_t>();
    } catch (const std::exception& e) {
        std::cerr << "DB Error (save_pending): " << e.what() << std::endl;
        return -1;
    }
}

void DatabaseProcessor::update_pending_status(int64_t request_id, const std::string& status, const std::string& error)
{
    try {
        auto conn_guard = pool_.acquire();
        pqxx::work tx(conn_guard.get());

        tx.exec("UPDATE pending_requests SET status = $1, last_error = $2 "
                "WHERE id = $3",
                pqxx::params{status, error, request_id});

        tx.commit();
    } catch (const std::exception& e) {
        std::cerr << "DB Error (update_status): " << e.what() << std::endl;
    }
}

void DatabaseProcessor::schedule_retry(int64_t request_id, int minutes_delay, const std::string& error)
{
    try {
        auto conn_guard = pool_.acquire();
        pqxx::work tx(conn_guard.get());

        std::string interval = std::to_string(minutes_delay) + " minutes";

        tx.exec("UPDATE pending_requests SET status = 'FAILED', last_error = $1, "
                "retry_count = retry_count + 1, next_retry_at = CURRENT_TIMESTAMP + "
                "($2::interval) "
                "WHERE id = $3",
                pqxx::params{error, interval, request_id});

        tx.commit();
    } catch (const std::exception& e) {
        std::cerr << "DB Error (schedule_retry): " << e.what() << std::endl;
    }
}

std::vector<DatabaseProcessor::PendingTask> DatabaseProcessor::get_ready_for_retry()
{
    std::vector<PendingTask> tasks;
    try {
        auto conn_guard = pool_.acquire();
        pqxx::work tx(conn_guard.get());

        pqxx::result r = tx.exec("SELECT id, user_id, raw_input FROM pending_requests "
                                 "WHERE status != 'COMPLETED' AND next_retry_at <= CURRENT_TIMESTAMP "
                                 "ORDER BY next_retry_at ASC LIMIT 10");

        for (auto const& row : r) {
            tasks.push_back({row[0].as<int64_t>(), row[1].as<int64_t>(), row[2].as<std::string>()});
        }
    } catch (const std::exception& e) {
        std::cerr << "DB Error (get_ready): " << e.what() << std::endl;
    }
    return tasks;
}

} // namespace ragc
