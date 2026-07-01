#include "worker_pool.hpp"

namespace ragc {

WorkerPool::WorkerPool(std::size_t num_threads)
{
    for (std::size_t i = 0; i < num_threads; ++i) {
        threads_.emplace_back([this] { worker_loop(); });
    }
}

WorkerPool::~WorkerPool()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void WorkerPool::enqueue(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.emplace(std::move(task));
    }
    cv_.notify_one();
}

std::size_t WorkerPool::pending() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

void WorkerPool::worker_loop()
{
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

            if (stop_ && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

} // namespace ragc
