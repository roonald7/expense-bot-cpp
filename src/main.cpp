#include "config.hpp"
#include "db_processor.hpp"
#include "ollama_client.hpp"
#include "processor.hpp"
#include "receiver.hpp"
#include "worker_pool.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

auto main() -> int
{
    try {
        const auto config = ragc::Config::load();
        constexpr std::size_t POOL_SIZE = 4;

        // 1. Infrastructure
        ragc::DatabaseProcessor db(config.db_url, POOL_SIZE);
        db.init_tables();

        dpp::cluster bot(config.token);

        // AI backend — swap OllamaClient for GeminiClient (or any AIClient
        // subclass) here without touching any other file.
        ragc::OllamaClient ai(bot, config.ai_api_url, config.ai_api_key, config.ai_model_name);

        ragc::Processor processor(db, ai);
        ragc::WorkerPool workers(POOL_SIZE);

        // Background thread to retry failed AI requests (429 / 503 / timeout)
        std::thread retry_thread([&processor]() {
            std::cout << "[WORKER] Background retry thread started.\n";
            while (true) {
                std::this_thread::sleep_for(std::chrono::minutes(1));
                try {
                    processor.retry_background_tasks();
                } catch (const std::exception& e) {
                    std::cerr << "[WORKER] Error during background task retry: " << e.what() << "\n";
                } catch (...) {
                    std::cerr << "[WORKER] Unknown error during background task retry.\n";
                }
            }
        });
        retry_thread.detach();

        // 2. Receiver (Event orchestration)
        ragc::Receiver receiver(bot, processor, workers, config.guild_id);
        receiver.wire_events();

        // 3. Start
        std::cout << "ExpenseBot Booting...\n";
        bot.start(dpp::st_wait);
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}