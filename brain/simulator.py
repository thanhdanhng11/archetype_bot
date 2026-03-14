import os
import json
import asyncio
from dotenv import load_dotenv
from web3 import AsyncWeb3, WebSocketProvider

# 1. Resolve absolute paths
base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
env_path = os.path.join(base_dir, '.env')
load_dotenv(dotenv_path=env_path)

WSS_URL = os.getenv("RPC_WSS_URL")

# Chainlink Price Feed Addresses on Arbitrum Mainnet
# If you are on Base, these addresses will be different.
CHAINLINK_FEEDS = {
    "ETH_USD": "0x639Fe6ab55C921f74e7fac1ee960C0B6293ba612",
    "WBTC_USD": "0x6ce185860a4963106506C203335A2910413708E9",
    "USDC_USD": "0x50834F3163758fcC1Df9973b6e91f0F0F0434aD3"
}

class Simulator:
    def __init__(self, w3: AsyncWeb3):
        """
        Initializes the pricing engine to calculate Health Factors.
        """
        self.w3 = w3
        
        # Load the minimal Chainlink ABI
        abi_path = os.path.join(base_dir, 'brain', 'abis', 'chainlink.json')
        with open(abi_path, 'r') as file:
            self.chainlink_abi = json.load(file)

    async def get_live_price(self, feed_name: str, feed_address: str) -> float:
        """
        Queries the Chainlink Oracle for the precise, real-time USD value of an asset.
        """
        try:
            # Bind the contract
            contract = self.w3.eth.contract(
                address=self.w3.to_checksum_address(feed_address), 
                abi=self.chainlink_abi
            )
            
            # Fetch the data. index [1] contains the actual price.
            # Chainlink USD feeds standardly use 8 decimals.
            data = await contract.functions.latestRoundData().call()
            raw_price = data[1]
            
            # Normalize the price
            actual_price = raw_price / (10 ** 8)
            print(f"[*] Oracle Pulse | {feed_name}: ${actual_price:,.2f}")
            return actual_price
            
        except Exception as e:
            print(f"[-] Oracle Error ({feed_name}): {e}")
            return 0.0

# Standalone execution for testing the Oracle connection
async def test_oracles():
    print("[*] Waking up the Pricing Simulator...\n")
    
    async with AsyncWeb3(WebSocketProvider(WSS_URL)) as w3:
        # Check connection
        if not await w3.is_connected():
            print("[-] Critical Error: Cannot connect to the blockchain RPC.")
            return

        sim = Simulator(w3)

        # Fetch live prices concurrently
        tasks = [
            sim.get_live_price("Ethereum", CHAINLINK_FEEDS["ETH_USD"]),
            sim.get_live_price("Wrapped BTC", CHAINLINK_FEEDS["WBTC_USD"]),
            sim.get_live_price("USDC", CHAINLINK_FEEDS["USDC_USD"])
        ]
        
        await asyncio.gather(*tasks)

if __name__ == "__main__":
    asyncio.run(test_oracles())