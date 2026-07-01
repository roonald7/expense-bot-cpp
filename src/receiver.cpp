#include "receiver.hpp"
#include "processor.hpp"
#include "replier.hpp"
#include "worker_pool.hpp"

#include <iostream>

namespace ragc {

Receiver::Receiver(dpp::cluster& bot, Processor& processor, WorkerPool& workers, std::optional<std::string> guild_id)
    : bot_(bot), processor_(processor), workers_(workers), guild_id_(std::move(guild_id))
{
}

void Receiver::wire_events()
{
    bot_.on_log(dpp::utility::cout_logger());

    bot_.on_ready([this](const dpp::ready_t& event) { register_commands(); });

    bot_.on_slashcommand([this](const dpp::slashcommand_t& event) {
        const std::string cmd = event.command.get_command_name();

        if (cmd == "ping") {
            handle_ping(event);
        } else if (cmd == "expense") {
            handle_expense(event);
        }
    });
}

void Receiver::handle_ping(const dpp::slashcommand_t& event)
{
    event.reply("Pong! ExpenseBot is operational.");
}

void Receiver::handle_expense(const dpp::slashcommand_t& event)
{
    std::string input = std::get<std::string>(event.get_parameter("query"));
    auto raw_user_id = event.command.usr.id;
    auto user_id = static_cast<int64_t>(raw_user_id);

    std::cout << "[RECEIVER] Received expense command from " << user_id << "\n";

    event.thinking(false, [this, event, input, user_id](const dpp::confirmation_callback_t& auth) {
        workers_.enqueue([this, event, input, user_id]() {
            auto result = processor_.process_message(input, user_id);

            if (result.has_value()) {
                dpp::message msg(event.command.channel_id, Replier::create_expense_embed(result.value()));
                event.edit_response(msg);
            } else {
                event.edit_response("❌ Failed to parse or save expense. Please try again with more "
                                    "detail.");
            }
        });
    });
}

void Receiver::register_commands()
{
    if (dpp::run_once<struct register_bot_commands>()) {
        dpp::slashcommand ping_cmd("ping", "Check bot status", bot_.me.id);

        dpp::slashcommand exp_cmd("expense", "Log a new expense", bot_.me.id);
        exp_cmd.add_option(dpp::command_option(dpp::co_string, "query", "What did you spend?", true));

        if (guild_id_.has_value()) {
            dpp::snowflake gid(std::stoull(*guild_id_));
            bot_.guild_command_create(ping_cmd, gid);
            bot_.guild_command_create(exp_cmd, gid);
            std::cout << "Receiver: Commands registered to guild " << gid << "\n";
        } else {
            bot_.global_command_create(ping_cmd);
            bot_.global_command_create(exp_cmd);
            std::cout << "Receiver: Commands registered globally.\n";
        }
    }
}

} // namespace ragc
