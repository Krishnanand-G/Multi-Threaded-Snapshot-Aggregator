#pragma once
#include <string>

struct TickMessage {
    std::string symbol;
    double bid;
    double ask;
    long long timestamp;
};
