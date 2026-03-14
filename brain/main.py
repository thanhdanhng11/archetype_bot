
import asyncio 
import os
from dotenv import load_dotenv 
from web3 import AsyncWeb3, WebSocketProvider 

from db_client import DatabaseClient 
from decoder import EventDecoder 

base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
env_path = os.path.join(base_dir, '.env')

# 2. Force load the exact .env file
load_dotenv(dotenv_path=env_path)

# 3. Fetch the variables
WSS_URL = os.getenv("RPC_WSS_URL")
NETWORK_ID = "arbitrum_mainnet"

async def main_loop(): 
    print("The Archetype Bot is running...")

    # 1. Initialize Menory (Database Connection Pool)
    db = DatabaseClient()
    await db.connect()

    # 2. Initialize Brain (Event Decoder)
    decoder = EventDecoder()

    # 3. Initialize Eyes (WebSocket Connection)
    if not WSS_URL: 
        print("Error: WSS_URL is not set in the environment variables.")
        return 
    
    async with AsyncWeb3(WebSocketProvider(WSS_URL)) as w3: 
        if not await w3.is_connected(): 
            print("Critical Error: Failed to connect to the WebSocket provider.")
            await db.close()
            return 

        print("All systems nominal. Listening to the pulse of the network...\n")

        # Subscribe to new blocks
        subscription_id = await w3.eth.subscribe('newHeads')

        try: 
            # The infinite core loop
            async for event in w3.socket.process_subscriptions(): 
                block_data = event['result']
                block_number = int(block_data['number'], 16)

                print(f"[Core] Procesing block: {block_number}")

                # The Pipeline
                # Step A: Save the system state
                await db.update_sync_state(NETWORK_ID, block_number)

                # Step B: Fetch the actual logs of this block (the raw data). note: we use an empty dict {} to fetch all logs in the block for demonstration. in production, we will filter by specific contract addreses here to save bandwidth.
                block_logs = await w3.eth.get_logs({"fromBlock": block_number, "toBlock": block_number})
                
                # Step C: Pass the logs to the brain for decoding 
                transfer_count = 0
                for log in block_logs: 
                    decoded_data = decoder.decode_transfer(log) 

                    if decoded_data: 
                        transfer_count += 1

                        # Step D: (Future) Push 'decoded_data' to Postgres here 

                print(f"Decoded {transfer_count} ERC20 Transfer events in this block.")
        
        except Exception as e:
            print(f"System Exception Caught: {e}")
        finally:
            # Always ensure the database connection closes cleanly if the loop breaks
            await db.close()

if __name__ == "__main__":
    try:
        # Ignite the engine
        asyncio.run(main_loop())
    except KeyboardInterrupt:
        print("\n[*] Keyboard Interrupt detected. Shutting down gracefully...")