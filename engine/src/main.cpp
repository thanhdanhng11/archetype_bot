#include <hiredis/hiredis.h>
#include <iostream>
#include <string>

int main() {

  std::cout << "The Archetype C++ Engine is booting..." << std::endl;

  // 1. Establish a low-level socket connection to the Redis container
  redisContext *context = redisConnect("127.0.0.1", 6379);
  if (context == NULL || context->err) {
    if (context) {
      std::cerr << "Critical Error: Redis connection failed -> "
                << context->errstr << std::endl;
      redisFree(context);

    } else {
      std::cerr << "Critical Error: Failed to allocate Redis context."
                << std::endl;
    }
    return 1;
  }

  std::cout << "Engine neural link establised. Awaiting execution signals..." << std::endl;

  // 2. The Infinite loop
  while (true) {
    // BLPOP blocks the thread entirely until a payload arrives in the 'liquidation_orders' queue
    redisReply *reply = (redisReply *)redisCommand(context, "BLPOP liquidation_orders 0"); 
    if (reply == NULL) {
        std:: cerr << "Redis communication severed." << std::endl;
        break;
    }

    // BLPOP returns an array: element [0] is the queue name, element [1] is the payload string
    if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 2) {
        std::string payload = reply->element[1]->str;
        std::cout << "Raw Payload: " << payload << std::endl;
        std::cout << "Routing to Transaction Builder..." << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;

        // Future step: pass this raw JSON string to tx_builder.cpp to craft the blockchain transaction
    }
    
    freeReplyObject(reply);
  }

  // clean up memory on exit (though this loop never naturally exits)
  redisFree(context);
  return 0;
}