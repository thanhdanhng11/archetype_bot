#include "../include/types.hpp" 
#include <nlohmann/json.hpp> 
#include <iostream> 

// Alias for cleaner code 
using json = nlohmann::json; 

ExecutionPayload parse_redis_payload(const std::string& raw_json) {
    ExecutionPayload payload;
    
    try {
        // Parse the raw string into a json object
        auto j = json::parse(raw_json); 

        // Map the json fields directly to our C++ struct
        payload.target_user = j.at("target").get<std::string>();
        payload.debt_asset = j.at("asset").get<std::string>(); 
        payload.collateral_asset = j.at("collateral").get<std::string>();
        payload.amount = j.at("amount").get<std::string>();
        payload.timestamp = j.at("timestamp").get<std::string>(); 

    } catch (const json::parse_error& e) {
        std::cerr << "Critical Error: Invalid json structure -> " << e.what() << std::endl;

    } catch (const json::out_of_range& e) {
        std::cerr << "Critical Error: Missing required json field -> " << e.what() << std::endl;
    }

    return payload;

}