#include "../include/types.hpp"  
#include <secp256k1.h>  
#include <secp256k1_recovery.h>    
#include <iostream>   
#include <iomanip>
#include <sstream>
#include <vector> 

// Helper: convert hex string to byte vector
std::vector<unsigned char> hex_to_bytes_signer(const std::string& hex) {
    std::string clean_hex = (hex.substr(0,2) == "0x") ? hex.substr(2) : hex; 
    std::vector<unsigned char> bytes; 
    for (size_t i = 0; i < clean_hex.length(); i += 2) {
        bytes.push_back(static_cast<unsigned char>(std::stoi(clean_hex.substr(i, 2), nullptr, 16))); 

    }
    return bytes;
}

// Helper: convert bytes to hex string 
std::string bytes_to_hex_signer(const unsigned char* data, size_t length) {
    std::stringstream ss; 
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str(); 
}

// The core ethereum signer 
// It returns a vector containing [V, R, S] 
std::vector<std::string> sign_transaction(const std::string& private_key_hex, const std::string& keccak_hash_hex) {
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);

    std::vector<unsigned char> priv_key = hex_to_bytes_signer(private_key_hex);
    std::vector<unsigned char> msg_hash = hex_to_bytes_signer(keccak_hash_hex); 

    // We must use the recoverable signature structure for ethereum
    secp256k1_ecdsa_recoverable_signature signature; 
    std::vector<std::string> vrs_output;

    // Perform the mathematical signing
    int return_val = secp256k1_ecdsa_sign_recoverable(ctx, &signature, msg_hash.data(), priv_key.data(), NULL, NULL);

    if (return_val == 1) {
        unsigned char sig_bytes[64];
        int recovery_id; // This will become our 'V'

        // Extract the raw R and S bytes, and the recovery ID 
        secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, sig_bytes, &recovery_id, &signature); 

        std::string r_hex = "0x" + bytes_to_hex_signer(sig_bytes, 32);
        std::string s_hex = "0x" + bytes_to_hex_signer(sig_bytes + 32, 32); 

        // Pass the raw recovery ID back as a string so main.cpp can apply the chain ID math
        std::string v_raw = std::to_string(recovery_id); 

        vrs_output.push_back(v_raw); 
        vrs_output.push_back(r_hex); 
        vrs_output.push_back(s_hex);

    } else {
        std::cerr << "Critical Error: True cryptographic signing failed." << std::endl;
    }

    secp256k1_context_destroy(ctx);
    return vrs_output;
}