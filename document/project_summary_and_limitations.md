# Archetype Flash Loan System: Execution Flow, Limitations & Summary

This final document traces the high-level execution order of the system from boot to profit token generation, outlines what the project successfully accomplished, and honestly discusses limitations, difficulties, and future improvements.

## 1. Absolute Order of Execution

1. **`contracts/script/DeployArchetype.s.sol` (Initialization)**
   - **Runs First.** The Solidity contract must physically exist on the blockchain before any off-chain logic can target it. The script deploys `ArchetypeFlashLoan.sol` to Arbitrum using Foundry.

2. **`database/schema.sql` (Initialization)**
   - The PostgreSQL tables (`sync_state` and `lending_positions`) are created to house the off-chain system memory.

3. **`engine/src/main.cpp` (The Muscle - Background Daemon)**
   - The C++ engine boots and connects to the active Redis server on port `6379`.
   - It issues `BLPOP archetype_execute 0`, intentionally hanging the execution thread entirely. It now sleeps, silently waiting for the kill signal from the `archetype_execute` channel.

4. **`brain/main.py` (The Brain - Continuous Loop)**
   - The Python brain boots.
   - Connects to PostgreSQL (`db_client.py`).
   - Hooks into the Web3 Arbitrum node via WebSocket (`listener.py`).
   - Starts streaming Ethereum blocks sequentially, sending raw logs to (`decoder.py`) to maintain an active map of who owes what to Aave V3.

5. **`brain/simulator.py` & `brain/ipc_sender.py` (The Trigger)**
   - While Python updates the DB, `simulator.py` constantly asks Chainlink for the prices of the debt and collateral assets.
   - The exact millisecond Python determines a target's Health Factor is `< 1.0`, it invokes `ipc_sender.fire_kill_signal()`.
   - Python pushes the JSON payload to Redis.

6. **`engine/src/main.cpp` (The Strike)**
   - Redis instantly wakes up the C++ `BLPOP` thread.
   - C++ parses the JSON (`ipc_receiver.cpp`).
   - C++ mathematically formats the 32-byte arguments (`tx_builder.cpp`).
   - C++ RLP encodes, Keccak hashes, and signs the transaction with the deployer's private key (`rlp.cpp`, `keccak.cpp`, `signer.cpp`).
   - C++ broadcasts the raw transaction back to the Arbitrum RPC (`rpc_client.cpp`).

7. **`contracts/src/ArchetypeFlashLoan.sol` (The Final Output)**
   - **Runs Last.** The RPC node mines the C++ transaction.
   - The EVM executes `initiateStrike`.
   - The Flash Loan triggers. Aave liquidation executes. Uniswap V3 swap secures the repayment. Aave reclaims the principal.
   - **Final Output:** The Archetype contract permanently retains the physical net-positive WETH/USDC profit seized from the liquidated target.

---

## 2. Accomplishments

- **Zero-Latency Architecture Established:** By completely bypassing Web3 Python/Javascript libraries for the crucial transaction signing phase, the Archetype bot achieved massive execution speed advantages by handling RLP encoding and ECDSA signing natively in C++.
- **Asynchronous Data Ingestion:** Successfully implemented a non-blocking `asyncio` and `asyncpg` PostgreSQL pipeline capable of indexing thousands of Aave V3 lending events concurrently without race conditions.
- **IPC Safety Protocol:** Successfully bridged isolated Python environments to raw C++ executables using Redis as a highly fault-tolerant intermediary memory broker (`archetype_execute` channel), allowing language-specific optimizations.
- **Dynamic EIP-1559 Gas Engine:** Replaced hardcoded gas prices with a live `baseFeePerGas + priorityTip` calculation, ensuring competitive transaction inclusion without wasting ETH on overpaying.
- **Automated Test Suite (15/15 Passing):** Implemented a complete cross-language test suite — PyTest (Python), Native Assertions (C++), and Foundry Fork Tests (Solidity) — validating all critical security guards, payload math, and IPC serialization.
- **Phase 4 E2E Simulation Completed:** Successfully ran the full Python → Redis → C++ → RPC pipeline on Arbitrum Sepolia. The transaction was correctly signed, broadcast, and received by the Sepolia node, confirming complete architectural integrity.

## 3. Difficulties Encountered

- **EVM Payload Verification (C++):** Building robust Ethereum RLP encoders and Keccak hashers from absolute scratch in C++ was incredibly complex. If even one byte of padding was off, or if the `0x` string format wasn't explicitly managed across libraries, the entire cryptographic signature would be considered mathematically invalid by Arbitrum, throwing vague "invalid transaction" errors.
- **Type Casting & Precision Math:** Python dynamic typing vs. C++ static typing vs. Solidity Unsigned Integers. Ensuring that massive numbers (like `50000000000` Wei) didn't lose precision during JSON packaging or `std::string` formatting was a relentless vulnerability. We had to enforce strict string mappings across the network boundary to avoid floating-point corruption.
- **Dependencies & Dockerization:** Unifying heavy cryptographic headers (`secp256k1`, `hiredis`, `openssl`) alongside Python neural logic required extremely rigid Make/CMake files.

## 4. Limitations & Areas for Improvement

- **Gas Wars & Mempool Competition (Limitation):** Currently, the C++ engine fires the transaction blindly when a target goes insolvent. It does not look at the mempool to see if other MEV bots are attempting to liquidate the exact same target. If an adversary pays a higher "tip" to the sequencer, our transaction will arrive second and revert, losing our baseline gas fees.
- **Dynamic Profit Simulation (Area for Improvement):** The Solidity contract executes the Uniswap Arbitrage blindly based on the current pool reserves. A sophisticated upgrade would be to have the Python Brain actively simulate the Uniswap V3 slippage *before* firing the IPC signal, ensuring that network gas fees don't eclipse the arbitrage profit margin.
- **Decentralized Execution (Future Integration):** Currently, one single central C++ engine signs transactions with one private key. Expanding to a distributed set of engine nodes acting globally alongside multiple smart contracts across sidechains (Optimism, Base, Polygon) would exponentially scale system revenue.
