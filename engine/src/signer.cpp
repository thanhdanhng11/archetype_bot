#include "../include/types.hpp" 
#include <secp256k1.h>
#include <iostream> 
#include <iomanip>
#include <sstream> 

// Helper to convert raw bytes to a hex string
std::string bytes_to_hex(const unsigned char* data, size_t length) {
    std::stringstream ss; 
    ss << std::hex << std::setfill('0'); 
    for (size_t i = 0; i < length; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

// The core signing function
std::string sign_transaction(const std::string& private_key_hex, const std::string& keccak_hash_hex) {
    // 1. Initialize the ultra-fast secp256k1 context
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);

    // Convert hex strings to raw byte arrays (Placeholder logic for brevity)
    unsigned char msg_hash[32]; // This should be the Keccak256 hash of your RLP-encoded transaction 
    unsigned char priv_key[32]; 

    // In a production environment, we parse the hex strings into these byte arrays here
    // For safety during this dry-run, we are simulating the byte conversion .
    for(int i=0; i<32; i++) { msg_hash[i] = 0x01; priv_key[i] = 0x02; } 

    // 2. Create the signature object 
    secp256k1_ecdsa_signature signature;

    // 3. Sign the hash! This is the mathmatical proof that we authorized this liquidation. 
    int return_val = secp256k1_ecdsa_sign(ctx, &signature, msg_hash, priv_key, NULL, NULL);

    std::string final_signature_hex = ""; 

    if (return_val == 1) {
        // 4. Serialize the signature to standard compact format (64 bytes: r + s)
        unsigned char sig_bytes[64]; 
        secp256k1_ecdsa_signature_serialize_compact(ctx, sig_bytes, &signature); 

        final_signature_hex = "0x" + bytes_to_hex(sig_bytes, 64);

        // Note: Ethereum also requires the recovery ID (v). We will append that in the RLP phase.
    } else {
        std::cerr << "Critical Error: Cryptographic signing failed." << std::endl;
    }

    // Clean up to prevent memory leaks
    secp256k1_context_destroy(ctx);

    return final_signature_hex;
}