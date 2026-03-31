import redis
import json
from datetime import datetime

# ============================================================
# Phase 4: Step 3.4 - Mock Scenario Injection
# ============================================================
# Purpose: Bypasses standard mempool scanning and immediately
# injects a mathematically valid testnet opportunity into the
# Python → Redis → C++ pipeline to trigger the E2E simulation.
# 
# Run this script to start the Phase 4 E2E simulation.
# ============================================================

# Arbitrum Sepolia Testnet Token Addresses
SEPOLIA_USDC  = "0x75faf114eafb1BDbe2F0316DF893fd58CE46AA4d"  # Testnet USDC
SEPOLIA_WETH  = "0x980B62Da83eFf3A457E0d4fCEEA2A4F687D03b0b"  # Testnet WETH
MOCK_TARGET   = "0x1111111111111111111111111111111111111111"  # Mock undercollateralized user

# Per the spec: C++ subscribes to the "archetype_execute" channel
REDIS_CHANNEL = "archetype_execute"

def inject_mock_opportunity():
    """
    Step 3.4 of the Phase 4 E2E simulation.
    Publishes a formatted JSON payload to the archetype_execute Redis channel.
    The C++ engine will instantly wake up and construct the signed transaction.
    """
    r = redis.Redis(host='localhost', port=6379, db=0)

    # Verify Redis is alive before firing
    try:
        r.ping()
        print("[✓] Redis connection verified on localhost:6379")
    except redis.ConnectionError:
        print("[✗] FATAL: Redis is not running. Boot docker compose first.")
        return

    # Step 3.3: Strict JSON payload format per Phase 4 neural link spec
    payload = {
        "target":     MOCK_TARGET,
        "asset":      SEPOLIA_USDC,
        "collateral": SEPOLIA_WETH,
        "amount":     "1000000",     # 1 USDC (6 decimals), as string to avoid float precision loss
        "timestamp":  datetime.utcnow().isoformat()
    }

    serialized = json.dumps(payload)
    r.lpush(REDIS_CHANNEL, serialized)

    print(f"\n[!!!] PHASE 4: MOCK SIGNAL FIRED [!!!]")
    print(f"      Channel  : {REDIS_CHANNEL}")
    print(f"      Target   : {MOCK_TARGET}")
    print(f"      Debt     : {SEPOLIA_USDC} (USDC)")
    print(f"      Collateral: {SEPOLIA_WETH} (WETH)")
    print(f"      Amount   : 1,000,000 (1 USDC)")
    print(f"\n[*] Awaiting C++ engine response on Arbiscan...")
    print("--------------------------------------------------")

if __name__ == "__main__":
    inject_mock_opportunity()
