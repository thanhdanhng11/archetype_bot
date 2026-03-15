#include "../include/types.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>


// 1. Helper: Conver hex string to raw bytes
std::string hex_to_bytes(const std::string& hex) {
    std::string bytes;
    std::string clean_hex = (hex.substr(0, 2) == "0x") ? hex.substr(2) : hex; 

    // Pad with zero if odd length 
    if (clean_hex.length() % 2 != 0) clean_hex = "0" + clean_hex;

    for (size_t i = 0; i < clean_hex.length(); i += 2) {
        bytes.push_back(static_cast<char>(std::stoi(clean_hex.substr(i, 2), nullptr, 16)));
    } 
    return bytes;
}

// 2. Helper: Convert raw bytes back to hex string (for the final json-rpc payload) 
std::string bytes_to_hex_string(const std::string& bytes) {
    std::stringstream ss;  
    ss << std::hex << std::setfill('0'); 
    for (unsigned char c : bytes) {
        ss << std::setw(2) << static_cast<int>(c);
    } 
    return "0x" + ss.str();
}
 
// 3. Core logic: Calculate the  ethereum length prefix  
std::string encode_length(size_t len, int offset) {
    if (len < 56) {
        return std::string(1, static_cast<char>(len + offset)); 
    } else {
        std::string hex_len;
        size_t temp = len; 
        while (temp > 0) {
            hex_len = static_cast<char>(temp & 0xFF) + hex_len;
            temp >>= 8;
        }
        return std::string(1, static_cast<char>(hex_len.length() + offset + 55)) + hex_len;
    }
}
 
// 4. Encode a single string/integer
std::string rlp_encode_item(const std::string& hex_input) {
    if (hex_input == "0x0" || hex_input == "0x") return std::string(1, static_cast<char>(0x80)); 

    std::string bytes = hex_to_bytes(hex_input);
    if (bytes.length() == 1 && static_cast<unsigned char>(bytes[0]) < 0x80) {
        return bytes; // small single bytes don't need a
    }
    return encode_length(bytes.length(), 0x80) + bytes;
} 

// 5. Encode the final list of items into one payload
std::string rlp_encode_list(const std::vector<std::string>& encoded_items) {
    std::string payload = ""; 
    for (const auto& item : encoded_items) {
        payload += item;
    }
    return encode_length(payload.length(), 0xC0) + payload;
}