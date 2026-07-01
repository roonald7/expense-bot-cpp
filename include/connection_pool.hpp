#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <queue>
#include <string_view>

namespace ragc {

/**
 * @brief Thread-safe PostgreSQL connection pool.
 *
 * Manages a fixed set of connections shared across worker threads.
 * Each thread acquires a connection via RAII (ConnectionGuard), uses it,
 * and automatically returns it to the pool on scope exit.
 */
class ConnectionPool
{
public:
    /**
     * @brief Initializes the pool with N connections.
     *
     * @param db_url   PostgreSQL connection string.
     * @param pool_size Number of concurrent connections to pre-allocate.
     */
    ConnectionPool(std::string_view db_url, std::size_t pool_size);

    /**
     * @brief RAII guard: acquires a connection on construction,
     *        returns it to the pool on destruction.
     */
    class ConnectionGuard
    {
    public:
        ConnectionGuard(ConnectionPool& pool, std::unique_ptr<pqxx::connection> conn);
        ~ConnectionGuard();

        // Non-copyable, movable
        ConnectionGuard(const ConnectionGuard&) = delete;
        ConnectionGuard& operator=(const ConnectionGuard&) = delete;
        ConnectionGuard(ConnectionGuard&&) = default;

        pqxx::connection& get()
        {
            return *conn_;
        }

    private:
        ConnectionPool& pool_;
        std::unique_ptr<pqxx::connection> conn_;
    };

    /**
     * @brief Blocks the calling thread until a connection is available.
     * @return ConnectionGuard RAII wrapper around the acquired connection.
     */
    ConnectionGuard acquire();

private:
    friend class ConnectionGuard;
    void release(std::unique_ptr<pqxx::connection> conn);

    std::string db_url_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::unique_ptr<pqxx::connection>> pool_;
};

} // namespace ragc
