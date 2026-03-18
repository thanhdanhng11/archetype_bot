#include <iostream>
#include <hiredis/hiredis.h>
#include <string>
#include <vector>
#include <sstream>
#include "../include/types.hpp"

// Hardcoded for the architectural dry-run. 
// DO NOT put a real mainnet private key here yet.
const std::string RPC_URL = "https://arb1.arbitrum.io/rpc"; 

const std::string WALLET_ADDRESS = "0x1111111111111111111111111111111111111111"; // 40 characters
const std::string PRIVATE_KEY = "1111111111111111111111111111111111111111111111111111111111111111"; // 64 characters

const std::string ARCHETYPE_CONTRACT = "0x88D5210650e2608bAB7272BC223Ee4E856c9e101"; 
const std::string GAS_LIMIT = "0x7A120"; // 500,000 gas limit in hex
const int CHAIN_ID_INT = 42161;          // Arbitrum Mainnet
const std::string CHAIN_ID_HEX = "0xA4B1"; // 42161 in hex

// Quick helper to convert integer to hex string for the V value
std::string int_to_hex(int value) {
    std::stringstream ss;
    ss << "0x" << std::hex << value;
    return ss.str();
}

int main() {
    std::cout << "[*] The Archetype C++ Engine is armed and booting..." << std::endl;

    redisContext *context = redisConnect("127.0.0.1", 6379);
    if (context == NULL || context->err) {
        std::cerr << "[-] Critical Error: Redis connection failed." << std::endl;
        if (context) redisFree(context);
        return 1;
    }

    std::cout << "[+] Engine neural link established. Awaiting execution signals..." << std::endl;

    while (true) {
        redisReply *reply = (redisReply *)redisCommand(context, "BLPOP liquidation_orders 0");
        if (reply == NULL) break;

        if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 2) {
            std::string raw_payload = reply->element[1]->str;
            
            std::cout << "\n[⚡] TARGET ACQUIRED. INITIATING STRIKE SEQUENCE [⚡]" << std::endl;
            
            // 1. Parse & Build the Bullet
            // Your Live Assassin Contract
            std::string target_contract = "0x88D5210650e2608bAB7272BC223Ee4E856c9e101";

            // Mock Sepolia Test Data
            std::string target_user = "0x1111111111111111111111111111111111111111"; // Fake target
            std::string debt_asset = "0x75faf114eafb1BDbe2F0316DF893fd58CE46AA4d"; // Sepolia USDC
            std::string collateral_asset = "0x980B62Da83eFf3A457E0d4fCEEA2A4F687D03b0b"; // Sepolia WETH
            std::string debt_amount_hex = "00000000000000000000000000000000000000000000000000000000000F4240"; // 1,000,000 in hex (1 USDC) without 0x

            // The Trigger Code
            std::string function_selector = "fa83bfae"; 

            // The Final Assembly
            std::string hex_tx_data = "0x" + function_selector + 
                                       pad_to_32bytes(target_user) + 
                                       pad_to_32bytes(debt_asset) + 
                                       pad_to_32bytes(collateral_asset) + 
                                       pad_to_32bytes(debt_amount_hex);
            
            // 2. Fetch the Gunpowder
            std::string nonce = get_wallet_nonce(RPC_URL, WALLET_ADDRESS);
            std::string gas_price = get_gas_price(RPC_URL);
            
            // 3. The Dummy RLP Packaging (EIP-155 Prep)
            std::vector<std::string> unsigned_items = {
                rlp_encode_item(nonce), rlp_encode_item(gas_price), rlp_encode_item(GAS_LIMIT),
                rlp_encode_item(ARCHETYPE_CONTRACT), rlp_encode_item("0x0"), rlp_encode_item(hex_tx_data),
                rlp_encode_item(CHAIN_ID_HEX), rlp_encode_item("0x"), rlp_encode_item("0x")
            };
            
            std::string unsigned_rlp_bytes = rlp_encode_list(unsigned_items);
            std::string unsigned_hex = bytes_to_hex_string(unsigned_rlp_bytes);
            
            // 4. Hash and Sign
            std::string keccak_hash = generate_keccak256_hash(unsigned_hex);
            std::vector<std::string> signature = sign_transaction(PRIVATE_KEY, keccak_hash);
            
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
                    rlp_encode_item(ARCHETYPE_CONTRACT), rlp_encode_item("0x0"), rlp_encode_item(hex_tx_data),
                    rlp_encode_item(final_v_hex), rlp_encode_item(r_hex), rlp_encode_item(s_hex)
                };
                
                std::string signed_rlp_bytes = rlp_encode_list(signed_items);
                std::string final_payload = bytes_to_hex_string(signed_rlp_bytes);
                
                std::cout << "      -> Cryptographic signature applied (V, R, S)." << std::endl;
                std::cout << "      -> Broadcasting mathematically perfect transaction..." << std::endl;
                
                // 6. Broadcast to the Blockchain
                std::string rpc_payload = "{\"jsonrpc\":\"2.0\",\"method\":\"eth_sendRawTransaction\",\"params\":[\"" + final_payload + "\"],\"id\":1}";
                std::string network_response = fire_rpc_request(RPC_URL, rpc_payload);
                
                std::cout << "      -> Network Response: " << network_response << std::endl;
            }
            std::cout << "--------------------------------------------------" << std::endl;
        }
        freeReplyObject(reply);
    }

    redisFree(context);
    return 0;
}