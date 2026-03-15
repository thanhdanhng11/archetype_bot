#pragma once 
#include <string> 

// This struct is the strict, compiled version of your Python json payload. 
// It guarantees memory safety and instantaneous access during execution.

struct ExecutionPayload {
    std::string target_user;
    std::string debt_asset;
    std::string amount;
    std::string timestamp;
};

// Function declaration for our parser
ExecutionPayload parse_redis_payload(const std::string& raw_json);

std::string build_liquidation_tx(const ExecutionPayload& payload);