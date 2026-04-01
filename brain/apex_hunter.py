import asyncio
import os
import json
from dotenv import load_dotenv
from web3 import AsyncWeb3
from db_client import DatabaseClient
from ipc_sender import SignalTransmitter

# ------------------------------------------------------------------------------
# THE APEX HUNTER: Autonomous MEV Execution Scanner
# ------------------------------------------------------------------------------

# 1. Resolve Environment
base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
env_path = os.path.join(base_dir, '.env')
load_dotenv(dotenv_path=env_path)

RPC_URL = os.getenv("ARBITRUM_MAINNET_RPC_URL")

# Arbitrum Mainnet Production Constants
AAVE_V3_POOL_ADDRESS = "0x794a61358D6845594F94dc1DB02A252b5b4814aD"
USDC_ADDRESS = "0xaf88d065e77c8cc2239327c5edb3a432268e5831"
WETH_ADDRESS = "0x82aF49447D8a07e3bd95BD0d56f35241523fBab1"

# The threshold of death for any Aave position (1.0 in WAD)
HEALTH_FACTOR_LIQUIDATION_THRESHOLD = 10**18 

async def apex_scan_loop():
    print("\n[⚔️] The Apex Hunter has been unleashed on Arbitrum Mainnet [⚔️]")
    
    if not RPC_URL:
        print("[-] Critical Error: ARBITRUM_MAINNET_RPC_URL is missing in .env")
        return

    # 2. Establish connections
    w3 = AsyncWeb3(AsyncWeb3.AsyncHTTPProvider(RPC_URL))
    if not await w3.is_connected():
        print("[-] Critical Error: Failed to connect to the blockchain.")
        return
        
    db = DatabaseClient()
    await db.connect()
    
    ipc = SignalTransmitter()
    
    # Load Aave V3 ABI for getUserAccountData
    abis_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'abis')
    with open(os.path.join(abis_dir, 'aave_pool.json'), 'r') as f:
        aave_abi = json.load(f)
        
    aave_pool = w3.eth.contract(address=w3.to_checksum_address(AAVE_V3_POOL_ADDRESS), abi=aave_abi)

    try:
        while True:
            # Step 1: Query all tracked wallets from our PostgreSQL Brain
            debtors = await db.get_active_debtors()
            print(f"[*] Scanning {len(debtors)} active targets from the database...")
            
            for user in debtors:
                try:
                    checksum_user = w3.to_checksum_address(user)
                    
                    # Step 2: Query the true, non-counterfeitable on-chain state
                    account_data = await aave_pool.functions.getUserAccountData(checksum_user).call()
                    health_factor = account_data[5]
                    
                    # Step 3: Assess Vulnerability
                    if health_factor < HEALTH_FACTOR_LIQUIDATION_THRESHOLD:
                        total_debt_base = account_data[1]
                        
                        # In Aave V3, we can liquidate up to 50% of the active debt at a time
                        liquidatable_debt = total_debt_base // 2 
                        
                        print(f"\n[!!!] BLOOD IN THE WATER: Target {user} has cracked!")
                        print(f"      -> Health Factor: {health_factor / 10**18:.4f}")
                        print(f"      -> Seizing opportunity...")

                        # MVP Calculation: We statically assume WETH Collateral / USDC Debt.
                        # We execute our maximum possible kill size (using arbitrary large flash loan, contract resolves exactly).
                        # For now, we pass 10,000 USDC as a safe static payload to test execution.
                        target_kill_size = 10000000000 # 10,000 USDC (6 decimals)
                        
                        if liquidatable_debt > 0:
                            # Step 4: Pull the trigger. Fire JSON to the Execution Engine.
                            ipc.fire_kill_signal(
                                target_user=checksum_user,
                                debt_asset=USDC_ADDRESS,
                                collateral_asset=WETH_ADDRESS,
                                amount_to_repay=target_kill_size
                            )
                            
                except Exception as e:
                    print(f"[-] Evaluation Error for {user}: {e}")
                
                # Throttle execution slightly to avoid 429 RPC Rate Limits
                await asyncio.sleep(0.5)
                    
            # Cool down before the next hunting sequence (e.g. 10 seconds)
            await asyncio.sleep(10)
            
    except asyncio.CancelledError:
         print("\n[*] Apex Hunter deactivated.")
    finally:
        await db.close()

if __name__ == "__main__":
    try:
        asyncio.run(apex_scan_loop())
    except KeyboardInterrupt:
        print("\n[*] Terminating Apex Hunter gracefully.")
