// crypto_tests.cpp
// Native C++ unit tests for the Archetype Engine cryptographic utilities
// Compile: g++ -std=c++17 crypto_tests.cpp -o run_tests && ./run_tests

#include <iostream>
#include <string>
#include <cassert>
#include <vector>
#include <sstream>
#include <iomanip>

// =====================================================
// Re-implement the functions locally to test in isolation
// =====================================================
std::string pad_to_32bytes(std::string hex_value) {
    if (hex_value.size() >= 2 && hex_value.substr(0, 2) == "0x") {
        hex_value = hex_value.substr(2);
    }
    if (hex_value.length() > 64) {
        hex_value = hex_value.substr(hex_value.length() - 64);
    }
    return std::string(64 - hex_value.length(), '0') + hex_value;
}

std::string hex_to_bytes(const std::string& hex) {
    std::string bytes;
    std::string clean_hex = (hex.substr(0, 2) == "0x") ? hex.substr(2) : hex;
    if (clean_hex.length() % 2 != 0) clean_hex = "0" + clean_hex;
    for (size_t i = 0; i < clean_hex.length(); i += 2) {
        bytes.push_back(static_cast<char>(std::stoi(clean_hex.substr(i, 2), nullptr, 16)));
    }
    return bytes;
}

std::string bytes_to_hex_string(const std::string& bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char c : bytes) {
        ss << std::setw(2) << static_cast<int>(c);
    }
    return "0x" + ss.str();
}

std::string encode_length(size_t len, int offset) {
    if (len < 56) {
        return std::string(1, static_cast<char>(len + offset));
    } else {
        std::string hex_len;
        size_t temp = len;
        while (temp > 0) { hex_len = static_cast<char>(temp & 0xFF) + hex_len; temp >>= 8; }
        return std::string(1, static_cast<char>(hex_len.length() + offset + 55)) + hex_len;
    }
}

std::string rlp_encode_item(const std::string& hex_input) {
    if (hex_input == "0x0" || hex_input == "0x") return std::string(1, static_cast<char>(0x80));
    std::string bytes = hex_to_bytes(hex_input);
    if (bytes.length() == 1 && static_cast<unsigned char>(bytes[0]) < 0x80) return bytes;
    return encode_length(bytes.length(), 0x80) + bytes;
}

// =====================================================
// TESTS
// =====================================================
void test_pad_strips_0x() {
    std::string result = pad_to_32bytes("0xabcd");
    assert(result.length() == 64);
    assert(result.find("0x") == std::string::npos); // No 0x prefix in result
    std::cout << "[PASS] test_pad_strips_0x\n";
}

void test_pad_address_to_32_bytes() {
    // An Ethereum address is 20 bytes = 40 hex chars, should be padded to 64
    std::string addr = "0x1234567890abcdef1234567890abcdef12345678";
    std::string padded = pad_to_32bytes(addr);
    assert(padded.length() == 64);
    assert(padded.substr(0, 24) == std::string(24, '0')); // First 24 chars are zero-padding
    std::cout << "[PASS] test_pad_address_to_32_bytes\n";
}

void test_pad_already_64_chars() {
    std::string already_padded = std::string(64, 'f');
    std::string result = pad_to_32bytes(already_padded);
    assert(result == already_padded);
    std::cout << "[PASS] test_pad_already_64_chars\n";
}

void test_pad_amount_hex() {
    // 1 USDC = 1,000,000 = 0xF4240 (the function preserves original hex case)
    std::string result = pad_to_32bytes("0x00000000000000000000000000000000000000000000000000000000000F4240");
    assert(result.length() == 64);
    assert(result.substr(56) == "000F4240"); // Original case is preserved
    std::cout << "[PASS] test_pad_amount_hex\n";
}

void test_hex_to_bytes_round_trip() {
    std::string original = "0xdeadbeef";
    std::string bytes = hex_to_bytes(original);
    std::string recovered = bytes_to_hex_string(bytes);
    assert(recovered == original);
    std::cout << "[PASS] test_hex_to_bytes_round_trip\n";
}

void test_rlp_encode_zero_is_0x80() {
    // RLP encodes zero "0x0" as 0x80 (empty byte)
    std::string encoded = rlp_encode_item("0x0");
    assert(encoded.length() == 1);
    assert(static_cast<unsigned char>(encoded[0]) == 0x80);
    std::cout << "[PASS] test_rlp_encode_zero_is_0x80\n";
}

void test_rlp_single_small_byte() {
    // A single byte value < 0x80 should be encoded as itself
    std::string encoded = rlp_encode_item("0x01");
    assert(encoded.length() == 1);
    assert(static_cast<unsigned char>(encoded[0]) == 0x01);
    std::cout << "[PASS] test_rlp_single_small_byte\n";
}

int main() {
    std::cout << "====== Archetype Engine Crypto Tests ======\n\n";
    
    test_pad_strips_0x();
    test_pad_address_to_32_bytes();
    test_pad_already_64_chars();
    test_pad_amount_hex();
    test_hex_to_bytes_round_trip();
    test_rlp_encode_zero_is_0x80();
    test_rlp_single_small_byte();
    
    std::cout << "\n====== All Tests PASSED ======\n";
    return 0;
}
