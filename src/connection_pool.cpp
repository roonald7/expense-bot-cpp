#include "connection_pool.hpp"

#include <stdexcept>

namespace ragc {

// ─── Pool construction ────────────────────────────────────────────────────────

ConnectionPool::ConnectionPool(std::string_view db_url, std::size_t pool_size) : db_url_(db_url)
{
    if (pool_size == 0) {
        throw std::invalid_argument("Connection pool size must be > 0");
    }

    for (std::size_t i = 0; i < pool_size; ++i) {
        pool_.push(std::make_unique<pqxx::connection>(db_url_));
    }
}

// ─── Acquire ─────────────────────────────────────────────────────────────────

ConnectionPool::ConnectionGuard ConnectionPool::acquire()
{
    std::unique_lock<std::mutex> lock(mutex_);

    // Block until a connection is available
    cv_.wait(lock, [this] { return !pool_.empty(); });

    auto conn = std::move(pool_.front());
    pool_.pop();

    // Validate connection liveness
    try {
        if (!conn || !conn->is_open()) {
            conn = std::make_unique<pqxx::connection>(db_url_);
        }
    } catch (...) {
        // If reconnection fails, push back a null/dummy connection to maintain pool size or rethrow
        conn = std::make_unique<pqxx::connection>(db_url_);
    }

    return ConnectionGuard(*this, std::move(conn));
}

// ─── Release (called by ~ConnectionGuard) ────────────────────────────────────

void ConnectionPool::release(std::unique_ptr<pqxx::connection> conn)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push(std::move(conn));
    }
    cv_.notify_one(); // Wake up one waiting thread
}

// ─── ConnectionGuard ─────────────────────────────────────────────────────────

ConnectionPool::ConnectionGuard::ConnectionGuard(ConnectionPool& pool, std::unique_ptr<pqxx::connection> conn)
    : pool_(pool), conn_(std::move(conn))
{
}

ConnectionPool::ConnectionGuard::~ConnectionGuard()
{
    if (conn_) {
        pool_.release(std::move(conn_));
    }
}

} // namespace ragc
