#include <iostream>
#include <hiredis/hiredis.h>
#include <string>
#include "../include/types.hpp"

int main() {
    std::cout << "[*] The Archetype C++ Engine is booting..." << std::endl;

    redisContext *context = redisConnect("127.0.0.1", 6379);
    if (context == NULL || context->err) {
        std::cerr << "[-] Critical Error: Redis connection failed." << std::endl;
        if (context) redisFree(context);
        return 1;
    }

    std::cout << "[+] Engine neural link established. Awaiting execution signals..." << std::endl;

    while (true) {
        redisReply *reply = (redisReply *)redisCommand(context, "BLPOP liquidation_orders 0");

        if (reply == NULL) {
            std::cerr << "[-] Redis communication severed." << std::endl;
            break;
        }

        if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 2) {
            std::string raw_payload = reply->element[1]->str;
            
            // Pass the raw string to our new parser
            ExecutionPayload target_data = parse_redis_payload(raw_payload);
            
            std::cout << "\n[⚡] SIGNAL PARSED SUCCESSFULLY [⚡]" << std::endl;
            std::cout << "      -> Target: " << target_data.target_user << std::endl;
            std::cout << "      -> Asset:  " << target_data.debt_asset << std::endl;
            std::cout << "      -> Amount: " << target_data.amount << std::endl;
            std::cout << "--------------------------------------------------" << std::endl;
            
            // Next up: Send 'target_data' to tx_builder.cpp
            std::string hex_tx_data = build_liquidation_tx(target_data);
            std::cout << "\n[⚡] TRANSACTION DATA ENCODED [⚡]" << std::endl;
            std::cout << "      -> Target: " << target_data.target_user << std::endl;
            std::cout << "      -> Payload: " << hex_tx_data << std::endl;
            std::cout << "--------------------------------------------------" << std::endl;
        }

        freeReplyObject(reply);
    }

    redisFree(context);
    return 0;
}