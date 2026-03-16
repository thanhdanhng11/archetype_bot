#include "../include/types.hpp"
#include <cryptopp/keccak.h> 
#include <cryptopp/hex.h>
#include <cryptopp/filters.h> 
#include <string> 


std::string generate_keccak256_hash(const std::string& rlp_hex_payload) {
    // 1. Clean the input (remove "0x" if it exists) 
    std::string clean_hex = (rlp_hex_payload.substr(0, 2) == "0x") ? rlp_hex_payload.substr(2) : rlp_hex_payload; 

    // 2. Convert the hex string back into raw bytes for hasing
    std::string raw_bytes; 
    CryptoPP::StringSource ss(clean_hex, true,
        new CryptoPP::HexDecoder(
            new CryptoPP::StringSink(raw_bytes)
        )
    ); 

    // 3. Initialize  the ethereum-standard keccak-256 hasher 
    CryptoPP::Keccak_256 hasher; 
    std::string hash_output; 

    // 4. Crush the raw bytes into a 32-byte hash, then convert back to hex
    CryptoPP::StringSource ss2(raw_bytes, true,
        new CryptoPP::HashFilter(hasher,
            new CryptoPP::HexEncoder(
                new CryptoPP::StringSink(hash_output), 
                false // false = lowercase hex letters 
            )
        )
    );

    return "0x" + hash_output; 
}