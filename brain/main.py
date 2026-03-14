import asyncio
import os
from dotenv import load_dotenv
from web3 import AsyncWeb3, WebSocketProvider

# Import our custom modules
from db_client import DatabaseClient
from decoder import EventDecoder

# 1. Resolve the absolute path to the root .env file securely
base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
env_path = os.path.join(base_dir, '.env')
load_dotenv(dotenv_path=env_path)

WSS_URL = os.getenv("RPC_WSS_URL")
NETWORK_ID = "arbitrum_mainnet"

async def main_loop():
    print("[*] The Archetype Conductor is waking up...")

    # 1. Initialize Memory
    db = DatabaseClient()
    await db.connect()

    # 2. Initialize Brain
    decoder = EventDecoder()

    # 3. Open Eyes
    if not WSS_URL or WSS_URL == "wss://YOUR_ALCHEMY_OR_QUICKNODE_WSS_URL_HERE":
        print("[-] Critical Error: RPC_WSS_URL is missing or invalid in .env.")
        await db.close()
        return

    async with AsyncWeb3(WebSocketProvider(WSS_URL)) as w3:
        if not await w3.is_connected():
            print("[-] Critical Error: Failed to connect to the blockchain node.")
            await db.close()
            return

        print("[+] All systems nominal. Listening to the pulse of the network...\n")
        
        # Subscribe to new blocks
        await w3.eth.subscribe('newHeads')

        try:
            # The Infinite Core Loop
            async for event in w3.socket.process_subscriptions():
                block_data = event['result']
                block_number = int(block_data['number'], 16)
                
                print(f"\n[Core] Processing Block: {block_number}")
                
                # Step A: Save the system state (Checkpoint)
                await db.update_sync_state(NETWORK_ID, block_number)
                
                # Step B: Fetch ALL logs for this specific block
                block_logs = await w3.eth.get_logs({"fromBlock": block_number, "toBlock": block_number})
                
                # Step C: Decode and Inject
                borrow_count = 0
                repay_count = 0
                
                for log in block_logs:
                    # 1. Check if the log is an Aave Borrow event
                    borrow_data = decoder.decode_aave_borrow(log)
                    if borrow_data:
                        borrow_count += 1
                        # Inject into PostgreSQL (Positive Delta for new debt)
                        await db.update_debt_position(
                            user=borrow_data['user'],
                            protocol=borrow_data['protocol'],
                            asset=borrow_data['asset'],
                            amount_delta=borrow_data['amount']
                        )
                        continue # Skip checking other decoders if we already found a match

                    # 2. Check if the log is an Aave Repay event
                    repay_data = decoder.decode_aave_repay(log)
                    if repay_data:
                        repay_count += 1
                        # Inject into PostgreSQL (Negative Delta to reduce debt)
                        await db.update_debt_position(
                            user=repay_data['user'],
                            protocol=repay_data['protocol'],
                            asset=repay_data['asset'],
                            amount_delta=-repay_data['amount']
                        )
                        continue
                        
                print(f"       -> Extracted & Stored: {borrow_count} Borrows | {repay_count} Repays.")

        except Exception as e:
            print(f"[!] System Exception Caught: {e}")
        finally:
            # Cleanly severe the database connection on exit
            await db.close()

if __name__ == "__main__":
    try:
        # Ignite the engine
        asyncio.run(main_loop())
    except KeyboardInterrupt:
        print("\n[*] Graceful shutdown initiated. The Conductor is sleeping.")