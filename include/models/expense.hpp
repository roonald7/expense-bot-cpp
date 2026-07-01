#pragma once

#include <string>

namespace ragc {

struct Expense
{
    double amount;
    std::string currency;
    std::string category;
    std::string description;
    int64_t user_id;
};

} // namespace ragc
