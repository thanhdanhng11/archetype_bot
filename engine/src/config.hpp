#ifndef ARCHETYPE_CONFIG_HPP
#define ARCHETYPE_CONFIG_HPP

#include <string>
#include <fstream>

#include <unordered_map>
#include <iostream>

struct EngineConfig {
    std::string rpc_url;
    std::string wallet_address;
    std::string private_key;
    std::string archetype_contract;
};

inline EngineConfig load_config(const std::string& env_path) {
    EngineConfig config;
    std::ifstream file(env_path);
    if (!file.is_open()) {
        std::cerr << "[-] Critical Error: Could not open " << env_path << std::endl;
        std::cerr << "    Please ensure you have created engine/.env based on the template." << std::endl;
        exit(1);
    }

    std::unordered_map<std::string, std::string> env_vars;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        size_t equal_pos = line.find('=');
        if (equal_pos != std::string::npos) {
            std::string key = line.substr(0, equal_pos);
            std::string value = line.substr(equal_pos + 1);
            
            // Basic trim
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            
            env_vars[key] = value;
        }
    }

    config.rpc_url = env_vars["ARBITRUM_MAINNET_RPC_URL"];
    config.wallet_address = env_vars["WALLET_ADDRESS"];
    config.private_key = env_vars["PRIVATE_KEY"];
    config.archetype_contract = env_vars["ARCHETYPE_CONTRACT"];

    if (config.rpc_url.empty() || config.wallet_address.empty() || 
        config.private_key.empty() || config.archetype_contract.empty()) {
        std::cerr << "[-] Critical Error: Missing required variables in .env" << std::endl;
        exit(1);
    }

    return config;
}

#endif // ARCHETYPE_CONFIG_HPP
