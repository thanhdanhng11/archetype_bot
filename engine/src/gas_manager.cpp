#include "../include/types.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <iostream> 

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

// Ask the network: "What is the exact base fee right now?"
std::string get_gas_price(const std::string& rpc_url) {
    json rpc_payload = {
        {"jsonrpc", "2.0"},
        {"method", "eth_gasPrice"}, 
        {"param", json::array()},
        {"id", 1}
    }; 

    std::string response = fire_rpc_request(rpc_url, rpc_payload.dump()); 

    try {
        auto j = json::parse(response); 
        if (j.contains("result")) {
            return j["result"].get<std::string>(); // Returns a hex string of the gas price
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse Gas Price: " << e.what() << std::endl;
    }
    return "0x0";
}