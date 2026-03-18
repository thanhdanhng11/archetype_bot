import json
import os
import redis
from datetime import datetime

class SignalTransmitter:
    def __init__(self):
        """
        Initializes the high-speed connection to the local Redis server.
        Redis acts as the shared memory space between the Python Brain and the C++ Engine.
        """
        # Connect to the Redis container we spun up via Docker
        try:
            self.redis_client = redis.Redis(
                host='localhost', 
                port=6379, 
                db=0, 
                decode_responses=True
            )
            # Ping the server to ensure the connection is alive
            self.redis_client.ping()
            print("[+] Neural link to the C++ Engine (Redis) is active.")
        except redis.ConnectionError as e:
            print(f"[-] Critical Error: Failed to connect to Redis -> {e}")

    def fire_kill_signal(self, target_user: str, debt_asset: str, collateral_asset: str, amount_to_repay: int):
        """
        Constructs the execution payload and fires it into the Redis queue.
        The C++ Engine will instantly pop this payload and build the blockchain transaction.
        """
        payload = {
            "target": target_user.lower(),
            "asset": debt_asset.lower(),
            "collateral": collateral_asset.lower(),
            "amount": str(amount_to_repay), # Convert to string to prevent precision loss in JSON
            "timestamp": datetime.utcnow().isoformat()
        }
        
        try:
            # LPUSH inserts the payload at the head of the list "liquidation_orders"
            self.redis_client.lpush("liquidation_orders", json.dumps(payload))
            print(f"\n[!!!] EXECUTION SIGNAL FIRED [!!!]")
            print(f"      -> Target: {target_user}")
            print(f"      -> Asset:  {debt_asset}")
            print(f"      -> Collateral: {collateral_asset}")
            print(f"      -> Amount: {amount_to_repay}")
            print("--------------------------------------------------")
        except Exception as e:
            print(f"[-] Failed to transmit kill signal: {e}")

# Quick local test execution
if __name__ == "__main__":
    print("[*] Testing the IPC Signal Transmitter...")
    transmitter = SignalTransmitter()
    
    # Simulating a kill order for a dummy address
    transmitter.fire_kill_signal(
        target_user="0x1234567890abcdef1234567890abcdef12345678",
        debt_asset="0xaf88d065e77c8cc2239327c5edb3a432268e5831", # USDC on Arbitrum
        collateral_asset="0x82aF49447D8a07e3bd95BD0d56f35241523fBab1", # WETH on Arbitrum
        amount_to_repay=50000000000 # 50,000 USDC (6 decimals)
    )