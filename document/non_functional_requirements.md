# Archetype Flash Loan System: Non-Functional Requirements

This document outlines the strict technical constraints and non-functional requirements that govern the Archetype Flash Loan project. Because this system operates in a highly adversarial, hyper-competitive Maximum Extractable Value (MEV) environment, standard software engineering paradigms are insufficient.

## 1. Zero-Latency Execution (Microsecond Precision)
- **Requirement:** The system must process an opportunity and dispatch a signed transaction in sub-millisecond time.
- **Justification:** Flash loan arbitrage and liquidations operate on a "first-to-mine" basis. If another searcher's bot identifies the same target user dropping below the health factor threshold, the transaction that reaches the mempool first (and with the optimal gas/tip) wins the block space.
- **Implementation:** 
  - The off-chain execution tier is written in pure C++ rather than Python or Node.js. 
  - Dynamic memory allocations in the critical path are minimized.
  - The use of Redis (`BLPOP`) provides instantly blocking, zero-polling inter-process communication (IPC) between Python (the brain) and C++ (the muscle).

## 2. Memory Safety & Deterministic Assembly
- **Requirement:** EVM payload assembly must be exact down to the byte.
- **Justification:** Any malformed transaction data will immediately cause an EVM revert, wasting gas and missing the opportunity.
- **Implementation:**
  - Strict 32-byte zero-padding for EVM function arguments implemented within C++.
  - RLP (Recursive Length Prefix) encoding built from scratch accurately to ensure network acceptance.
  - Cryptographic operations (Keccak256, secp256k1 signing) must execute deterministically without memory leaks or race conditions.

## 3. High-Throughput Data Ingestion Layer
- **Requirement:** The system must be capable of processing thousands of raw blockchain events (Borrows, Repays) per second.
- **Justification:** Aave V3 on Arbitrum Mainnet produces an immense amount of activity. The "Brain" must maintain an absolutely synchronized state of the lending pool to accurately simulate health factors.
- **Implementation:**
  - Asynchronous Python (`asyncio`, `web3.py` AsyncWeb3) over WebSocket (WSS) allows non-blocking ingestion of `newHeads` and batch fetching of logs.
  - Database updates are routed through an asynchronous connection pool (`asyncpg`) to a PostgreSQL instance.

## 4. Concurrent Database Math Updates
- **Requirement:** The system must perfectly update the exact debt balance of thousands of users simultaneously without race conditions.
- **Justification:** If two events for the same user occur within milliseconds, standard read-modify-write patterns will result in lost updates. An inaccurate state leads to the bot executing a revertable liquidation, losing gas money.
- **Implementation:**
  - PostgreSQL `ON CONFLICT DO UPDATE` UPSERT queries are used. The math `debt_amount = lending_positions.debt_amount + EXCLUDED.debt_amount` is handled atomically by the database engine itself, completely eliminating the race condition.

## 5. Security & Isolation
- **Requirement:** The private key with access to the gas funds and the contract owner privileges must be fiercely protected.
- **Justification:** If compromised, the system's wallet will be drained immediately by MEV sweepers.
- **Implementation:**
  - The private key is never hardcoded. It is injected into the C++ Engine via an `.env` file and immediately loaded into memory.
  - No RPC nodes, logging systems, or output terminals should echo the raw private key.

## 6. Fault Tolerance & State Resumption
- **Requirement:** If the Python Brain crashes or the WebSocket drops, the system must recover exactly where it left off.
- **Justification:** Missing even one block of Borrow/Repay events corrupts the entire database's calculated Health Factors, ruining the bot's accuracy.
- **Implementation:**
  - The system saves the `last_processed_block` into the PostgreSQL `sync_state` table. On boot, the bot queries this table and resumes parsing logs from that exact block.

## 7. Mathematical On-Chain Safeguards (The Floor)
- **Requirement:** The Smart Contract must guarantee absolute safety of principal. It must mathematically guarantee it never loses money on a trade.
- **Justification:** MEV sandwiching, slippage, or failed liquidations can result in the flash loan failing to be repaid, destroying the contract's operation.
- **Implementation:**
  - The Solidity contract uses `amountOutMinimum` precisely set to the `FlashLoan Principal + Premium fee`. If the Uniswap swap results in anything less than this amount, the entire transaction reverts, costing only minimal gas rather than the entire principal.
