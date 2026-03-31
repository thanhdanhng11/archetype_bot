# Phase 4: End-to-End (E2E) Testnet Simulation Protocol

## 1. Objective
To successfully validate the absolute latency, dynamic payload generation, and atomic profitability logic of the complete Archetype MEV architecture on the Arbitrum Sepolia Testnet. This phase ensures the system can autonomously detect an opportunity, route the data, calculate dynamic gas, and execute a mathematically profitable flash loan with zero human intervention.

---

## 2. Architecture Overview & Data Flow
The simulation tests the active neural link between all four core components:

| Component | Role |
|---|---|
| **Python Scanner** (`brain/mock_injector.py`) | The Brain — formats the target payload and publishes to Redis |
| **Redis Server** (`archetype_execute` channel) | The Nervous System — sub-millisecond message broker |
| **C++ Engine** (`engine/src/main.cpp`) | The Trigger — constructs the EVM payload and signs the transaction |
| **Solidity Contract** (`ArchetypeFlashLoan.sol`) | The Assassin — executes the flash loan and asset swaps on-chain |

---

## 3. Step-by-Step Execution Protocol

### Step 3.1: Infrastructure & Environment Verification
Before initializing the simulation, all local and remote environments must be synchronized.

- **Redis Initialization:** `docker compose up -d` — ensure Redis is active on `localhost:6379`.
- **Contract State:** Verify the `ArchetypeFlashLoan.sol` contract is deployed on Arbitrum Sepolia and holds a **zero-balance state**.
- **Execution Wallet:** Ensure the C++ engine's `.env` file contains the testnet burner wallet private key, funded with **≥ 0.05 Sepolia ETH** for gas.

### Step 3.2: Dynamic Gas (EIP-1559) — Already Implemented
> [!NOTE]
> Hardcoded gas is strictly prohibited. The `gas_manager.cpp` already fetches `baseFeePerGas` dynamically via `eth_getBlockByNumber` and adds the network's `eth_maxPriorityFeePerGas` tip with a 20% multiplier.

- `baseFee` → fetched live from each block.
- `maxFeePerGas` = `baseFee * 1.20 + priorityTip`
- Ensures transaction is included in the immediate **next block**.

### Step 3.3: Neural Link — Redis Channel (`archetype_execute`)
- **Python Publisher:** `brain/mock_injector.py` publishes the JSON payload via `LPUSH archetype_execute`.
- **C++ Subscriber:** `engine/src/main.cpp` continuously blocks on `BLPOP archetype_execute 0`.
- On wake, C++ parses the JSON, applies 32-byte ABI padding, and prepends the `fa83bfae` function selector.

**Payload JSON Format:**
```json
{
  "target":     "<undercollateralized wallet address>",
  "asset":      "<Sepolia USDC address>",
  "collateral": "<Sepolia WETH address>",
  "amount":     "<debt amount as string>",
  "timestamp":  "<ISO 8601 UTC timestamp>"
}
```

### Step 3.4: Mock Scenario Injection
```bash
# 1. Boot infrastructure
docker compose up -d

# 2. Arm the C++ Engine (in a separate terminal)
./run_main.sh

# 3. Fire the mock opportunity (in another terminal)
venv/bin/python brain/mock_injector.py
```

The mock scenario uses Arbitrum Sepolia testnet tokens:
- **Debt Asset:** `0x75faf114eafb1BDbe2F0316DF893fd58CE46AA4d` (Sepolia USDC)
- **Collateral:** `0x980B62Da83eFf3A457E0d4fCEEA2A4F687D03b0b` (Sepolia WETH)
- **Amount:** `1,000,000` (1 USDC, 6 decimals)

### Step 3.5: Atomic Execution & Verification

| Checkpoint | Expected Result |
|---|---|
| C++ Terminal Output | Must print the raw transaction hash |
| RPC Response | No `gas too low` or `nonce mismatch` error |
| Arbiscan Status | Transaction **Reverted** due to internal profitability check |

> [!IMPORTANT]
> A **clean contract revert** (not an RPC-level rejection) is the **success metric** for Phase 4. It proves the transaction was perfectly formatted, signed, and delivered to the network. The revert occurs because the mock target is mathematically unsatisfying to Aave — not because the system is broken.

---

## 4. Rollback & Failsafe Criteria

Do **NOT** proceed to Phase 5 (Mainnet Migration) if the simulation fails to produce a transaction hash. Failure must be isolated to one of:

| Failure Type | Cause | Fix Location |
|---|---|---|
| **Data Malformation** | ABI padding errors | `engine/src/tx_builder.cpp` |
| **Latency Delay** | RPC timeout or Redis bottleneck | `engine/src/rpc_client.cpp` |
| **Gas Estimation** | EIP-1559 integer conversion bug | `engine/src/gas_manager.cpp` |
| **Nonce Mismatch** | Wallet nonce out of sync | `engine/src/gas_manager.cpp` |
