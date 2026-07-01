#include <cstdlib>
#include <iostream>
#include "db_processor.hpp"
#include "gemini_client.hpp"
#include "processor.hpp"
#include "receiver.hpp"
#include "worker_pool.hpp"

struct Config
{
    std::string token;
    std::string db_url;
    std::string gemini_key;
    std::optional<std::string> guild_id;

    static Config load()
    {
        const char* token = std::getenv("DISCORD_BOT_TOKEN");
        const char* db_url = std::getenv("DATABASE_URL");
        const char* gemini_key = std::getenv("GEMINI_API_KEY");
        const char* guild_id = std::getenv("DISCORD_GUILD_ID");

        if (!token || !db_url || !gemini_key) {
            throw std::runtime_error("Missing critical environment variables (TOKEN, DB_URL, or GEMINI_KEY)");
        }

        return {token, db_url, gemini_key, guild_id ? std::make_optional(guild_id) : std::nullopt};
    }
};

#include <chrono>
#include <thread>

int main()
{
    try {
        const auto config = Config::load();
        constexpr std::size_t POOL_SIZE = 4;

        // 1. Infrastructure
        ragc::DatabaseProcessor db(config.db_url, POOL_SIZE);
        db.init_tables();

        dpp::cluster bot(config.token);
        ragc::GeminiClient ai(bot, config.gemini_key);
        ragc::Processor processor(db, ai);
        ragc::WorkerPool workers(POOL_SIZE);

        // AI Reliability: Background thread to retry failed Gemini 503 requests
        std::thread retry_thread([&processor]() {
            std::cout << "[WORKER] Background retry thread started." << std::endl;
            while (true) {
                std::this_thread::sleep_for(std::chrono::minutes(1));
                try {
                    processor.retry_background_tasks();
                } catch (...) {
                }
            }
        });
        retry_thread.detach();

        // 2. Receiver (Event orchestration)
        ragc::Receiver receiver(bot, processor, workers, config.guild_id);
        receiver.wire_events();

        // 3. Start
        std::cout << "ExpenseBot Booting..." << std::endl;
        bot.start(dpp::st_wait);
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}