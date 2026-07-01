#pragma once

#include <dpp/dpp.h>
#include <optional>
#include <string>

namespace ragc {

class Processor;
class WorkerPool;

/**
 * @brief The Receiver Layer: Handles all Discord events and command registrations.
 */
class Receiver
{
public:
    /**
     * @brief Construct a new Receiver.
     */
    Receiver(dpp::cluster& bot,
             Processor& processor,
             WorkerPool& workers,
             std::optional<std::string> guild_id = std::nullopt);

    /**
     * @brief Wire up all event handlers (on_ready, on_slashcommand, etc).
     */
    void wire_events();

private:
    void handle_ping(const dpp::slashcommand_t& event);
    void handle_expense(const dpp::slashcommand_t& event);
    void register_commands();

    dpp::cluster& bot_;
    Processor& processor_;
    WorkerPool& workers_;
    std::optional<std::string> guild_id_;
};

} // namespace ragc
