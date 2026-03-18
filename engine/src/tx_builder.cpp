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
    // 1. The method ID for initiateStrike(address,address,address,uint256)
    std::string method_id = "fa83bfae";

    // 2. Construct the final abi-encoded payload
    std::string tx_data = "0x" + method_id 
                                + pad_to_32bytes(payload.target_user)
                                + pad_to_32bytes(payload.debt_asset)
                                + pad_to_32bytes(payload.collateral_asset)
                                + pad_to_32bytes(payload.amount);
                                
                                
    return tx_data; 
}