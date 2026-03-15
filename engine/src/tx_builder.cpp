#include "../include/types.hpp" 
#include <string>

// Helper function to strip "0x" and pad any string to exactly 64 characters  
std::string pad_to_32bytes(std::string hex_value) {
    if (hex_value.substr(0, 2) == "0x") {
        hex_value = hex_value.substr(2);
    }
    return std::string(64 - hex_value.length(), '0') + hex_value;
}

std::string build_liquidation_tx(const ExecutionPayload& payload) {
    // 1. The method ID for Aave's liquidationCall(...)
    std::string method_id = "00a718a9";

    // 2. The collateral to seize
    // (For this phase, we hardcode Arbitrum WETH. Later, Python will tell C++ which asset to seize).
    std::string collateral_asset = "0x82aF49447D8a07e3bd95BD0d56f35241523fBab1";

    // 3. Receive aToken?
    // We pass "0" (false) because we want to the raw WETH, not Aave's interest-bearing token.
    std::string receive_a_token = "0"; 

    // 4. Construct the final abi-encoded 
    std::string tx_data = "0x" + method_id 
                                + pad_to_32bytes(collateral_asset)
                                + pad_to_32bytes(payload.debt_asset)
                                + pad_to_32bytes(payload.target_user)
                                + pad_to_32bytes(payload.amount)                                
                                + pad_to_32bytes(receive_a_token);
                                
                                
    return tx_data; 
}