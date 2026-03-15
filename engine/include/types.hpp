#pragma once 
#include <string> 
#include <vector>

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

std::string fire_rpc_request(const std::string& rpc_url, const std::string& json_payload);

std::string sign_transaction(const std::string& private_key_hex, const std::string& keccak_hash_hex);

std::string get_wallet_nonce(const std::string& rpc_url, const std::string& wallet_address);

std::string get_gas_price(const std::string& rpc_url);

std::string rlp_encode_item(const std::string& hex_input);

std::string rlp_encode_list(const std::vector<std::string>& encoded_items);

std::string bytes_to_hex_string(const std::string& bytes);