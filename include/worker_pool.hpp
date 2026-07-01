#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace ragc {

/**
 * @brief Fixed-size thread pool backed by a blocking task queue.
 *
 * Replaces std::thread::detach() with a controlled, observable worker pool.
 * - Bounded: only N threads regardless of traffic spikes.
 * - Graceful shutdown: drains all pending tasks before destroying threads.
 * - Backpressure-ready: queue depth can be exposed as a health metric.
 *
 * Pool size should match ConnectionPool size for zero-wait DB access.
 */
class WorkerPool
{
public:
    /**
     * @brief Starts N worker threads immediately.
     * @param num_threads Number of concurrent workers.
     */
    explicit WorkerPool(std::size_t num_threads);

    /**
     * @brief Signals shutdown, drains the queue, and joins all threads.
     */
    ~WorkerPool();

    // Non-copyable, non-movable — owns OS threads
    WorkerPool(const WorkerPool&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;

    /**
     * @brief Enqueues a callable to be executed by the next available worker.
     * @param task Any callable (lambda, std::function, functor).
     */
    void enqueue(std::function<void()> task);

    /** @brief Number of tasks currently waiting in the queue. */
    std::size_t pending() const;

private:
    void worker_loop();

    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

} // namespace ragc
