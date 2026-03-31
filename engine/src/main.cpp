#include <iostream>
#include <hiredis/hiredis.h>
#include <string>
#include <vector>
#include <sstream>
#include "../include/types.hpp"
#include "config.hpp"

// ============================================================
// Phase 5 & 6: Mainnet Configuration (Arbitrum One)
// ============================================================

const std::string GAS_LIMIT = "0x7A120"; // 500,000 gas limit in hex
const int CHAIN_ID_INT = 42161;          // Arbitrum Mainnet Chain ID
const std::string CHAIN_ID_HEX = "0xA4B1"; // 42161 in hex


// Quick helper to convert integer to hex string for the V value
std::string int_to_hex(int value) {
    std::stringstream ss;
    ss << "0x" << std::hex << value;
    return ss.str();
}

int main() {
    std::cout << "[*] Loading Archetype Engine Configuration..." << std::endl;
    EngineConfig config = load_config(".env");
    
    std::cout << "[*] RPC Target: " << config.rpc_url << std::endl;
    std::cout << "[*] Wallet Address: " << config.wallet_address << std::endl;
    std::cout << "[*] Archetype Contract: " << config.archetype_contract << std::endl;
    std::cout << "[*] The Archetype C++ Engine is armed and booting on ARBITRUM MAINNET..." << std::endl;

    redisContext *context = redisConnect("127.0.0.1", 6379);
    if (context == NULL || context->err) {
        std::cerr << "[-] Critical Error: Redis connection failed." << std::endl;
        if (context) redisFree(context);
        return 1;
    }

    // Phase 4: Step 3.3 — C++ subscribes to archetype_execute channel per the neural link spec
    std::cout << "[+] Engine neural link established. Listening on [archetype_execute]..." << std::endl;

    while (true) {
        redisReply *reply = (redisReply *)redisCommand(context, "BLPOP archetype_execute 0");
        if (reply == NULL) break;

        if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 2) {
            std::string raw_payload = reply->element[1]->str;
            
            std::cout << "\n[⚡] TARGET ACQUIRED. INITIATING STRIKE SEQUENCE [⚡]" << std::endl;
            
            // 1. Parse & Build the Bullet
            // Dynamically rip the targets straight from the Redis JSON payload
            ExecutionPayload target_payload = parse_redis_payload(raw_payload);

            // The Trigger Code
            std::string function_selector = "fa83bfae"; 

            // The Final Assembly
            std::string hex_tx_data = "0x" + function_selector + 
                                       pad_to_32bytes(target_payload.target_user) + 
                                       pad_to_32bytes(target_payload.debt_asset) + 
                                       pad_to_32bytes(target_payload.collateral_asset) + 
                                       pad_to_32bytes(target_payload.amount);
            
            // 2. Fetch the Gunpowder
            std::string nonce = get_wallet_nonce(config.rpc_url, config.wallet_address);
            std::string gas_price = get_gas_price(config.rpc_url);
            
            // 3. The Dummy RLP Packaging (EIP-155 Prep)
            std::vector<std::string> unsigned_items = {
                rlp_encode_item(nonce), rlp_encode_item(gas_price), rlp_encode_item(GAS_LIMIT),
                rlp_encode_item(config.archetype_contract), rlp_encode_item("0x0"), rlp_encode_item(hex_tx_data),
                rlp_encode_item(CHAIN_ID_HEX), rlp_encode_item("0x"), rlp_encode_item("0x")
            };

            
            std::string unsigned_rlp_bytes = rlp_encode_list(unsigned_items);
            std::string unsigned_hex = bytes_to_hex_string(unsigned_rlp_bytes);
            
            // 4. Hash and Sign
            std::string keccak_hash = generate_keccak256_hash(unsigned_hex);
            std::vector<std::string> signature = sign_transaction(config.private_key, keccak_hash);
            
            if (signature.size() == 3) {
                // EIP-155 Math: V = recovery_id + (CHAIN_ID * 2) + 35
                int recovery_id = std::stoi(signature[0]);
                int final_v_int = recovery_id + (CHAIN_ID_INT * 2) + 35;
                std::string final_v_hex = int_to_hex(final_v_int);
                
                std::string r_hex = signature[1];
                std::string s_hex = signature[2];

                // 5. The Final RLP Packaging (The Real Bullet)
                std::vector<std::string> signed_items = {
                    rlp_encode_item(nonce), rlp_encode_item(gas_price), rlp_encode_item(GAS_LIMIT),
                    rlp_encode_item(config.archetype_contract), rlp_encode_item("0x0"), rlp_encode_item(hex_tx_data),
                    rlp_encode_item(final_v_hex), rlp_encode_item(r_hex), rlp_encode_item(s_hex)
                };
                
                std::string signed_rlp_bytes = rlp_encode_list(signed_items);
                std::string final_payload = bytes_to_hex_string(signed_rlp_bytes);
                
                std::cout << "      -> Cryptographic signature applied (V, R, S)." << std::endl;
                std::cout << "      -> Broadcasting mathematically perfect transaction..." << std::endl;
                
                // 6. Broadcast to the Blockchain
                std::string rpc_payload = "{\"jsonrpc\":\"2.0\",\"method\":\"eth_sendRawTransaction\",\"params\":[\"" + final_payload + "\"],\"id\":1}";
                std::string network_response = fire_rpc_request(config.rpc_url, rpc_payload);
                
                std::cout << "      -> Network Response: " << network_response << std::endl;
            }
            std::cout << "--------------------------------------------------" << std::endl;
        }
        freeReplyObject(reply);
    }

    redisFree(context);
    return 0;
}