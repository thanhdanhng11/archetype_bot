#include "../include/types.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <iostream> 
#include <sstream>

using json = nlohmann::json; 

// Ask the network: "How many transactions has my wallet sent?"

std::string get_wallet_nonce(const std::string& rpc_url, const std::string& wallet_address) {
    json rpc_payload = {
        {"jsonrpc", "2.0"},
        {"method", "eth_getTransactionCount"}, 
        {"params", {wallet_address, "latest"}}, 
        {"id", 1}
    };

    std::string response = fire_rpc_request(rpc_url, rpc_payload.dump());

    try {
        auto j = json::parse(response);
        if (j.contains("result")) {
            return j["result"].get<std::string>(); // Return a hex string like "0x1b"
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse Nonce: " << e.what() << std::endl;
    }
    return "0x0"; 
}

// Ask the network: "What is the dynamic base fee and priority fee right now?"
std::string get_gas_price(const std::string& rpc_url) {
    // 1. Get Base Fee
    json block_payload = {
        {"jsonrpc", "2.0"},
        {"method", "eth_getBlockByNumber"}, 
        {"params", {"latest", false}},
        {"id", 1}
    }; 
    std::string block_response = fire_rpc_request(rpc_url, block_payload.dump()); 
    uint64_t base_fee = 0;
    try {
        auto j = json::parse(block_response); 
        if (j.contains("result") && j["result"].contains("baseFeePerGas")) {
            std::string hex_str = j["result"]["baseFeePerGas"].get<std::string>();
            if (hex_str.substr(0, 2) == "0x") hex_str = hex_str.substr(2);
            base_fee = std::stoull(hex_str, nullptr, 16);
        }
    } catch (...) {}

    // 2. Get Max Priority Fee
    json priority_payload = {
        {"jsonrpc", "2.0"},
        {"method", "eth_maxPriorityFeePerGas"}, 
        {"params", json::array()},
        {"id", 2}
    }; 
    std::string priority_response = fire_rpc_request(rpc_url, priority_payload.dump()); 
    uint64_t priority_fee = 0;
    try {
        auto j = json::parse(priority_response); 
        if (j.contains("result")) {
            std::string hex_str = j["result"].get<std::string>();
            if (hex_str.substr(0, 2) == "0x") hex_str = hex_str.substr(2);
            priority_fee = std::stoull(hex_str, nullptr, 16);
        }
    } catch (...) {}

    // 3. Dynamic Calculation: Optimize for the next block inclusion
    // Apply a 20% multiplier to the base fee to guarantee inclusion + the priority tip
    uint64_t optimal_gas_price = (base_fee * 120 / 100) + priority_fee;
    
    if (optimal_gas_price == 0) return "0x3B9ACA00"; // 1 gwei fallback for safety bounds

    std::stringstream ss;
    ss << "0x" << std::hex << optimal_gas_price;
    return ss.str();
}