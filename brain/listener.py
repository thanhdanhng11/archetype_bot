import asyncio
import os
from dotenv import load_dotenv
from web3 import AsyncWeb3, WebSocketProvider
from websockets.exceptions import ConnectionClosedError

load_dotenv()
WSS_URL = os.getenv("RPC_WSS_URL")

async def open_the_eyes():
    if not WSS_URL:
        print("Error: RPC_WSS_URL is not configured in the .env file.")
        return
    print("Connecting the neural link to the Layer 2 network...")

    # Initilize asynchoronous Web3 with WebSocket Provider
    async with AsyncWeb3(WebSocketProvider(WSS_URL)) as w3:
        if await w3.is_connected():
            print("Connection established.")
        else:
            print("Connection failed. Please verify the WSS_URL.")
            return
        
        # Subscribe to new block headers
        subscription_id = await w3.eth.subscribe('newHeads')

        try:
            # Infinite loop: listen continuously
            async for event in w3.socket.process_subscriptions():
                block_data = event['result']

                # Parse hex data to human-readable integers and strings
                block_number = int(block_data['number'], 16)
                block_hash = block_data['hash']

                print(f"[New Block] Number: {block_number} | Hash: {block_hash[:8]}...{block_hash[-6]}")
        
        except ConnectionClosedError:
            print("Connection abruptly closed. System requires an Auto-Reconnect routine.")
        except Exception as e:
            print(f"Operational Exception: {e}")


if __name__ == "__main__":
    try:
        # Ignite the asynchoronous event loop
        asyncio.run(open_the_eyes())
    except KeyboardInterrupt:
        print("System terminated safety.")