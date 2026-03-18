# Archetype Flash Loan Bot: Project Handover & Onboarding Guide

Welcome to the Archetype project! This document is designed to get new developers, quants, and engineers up to speed on the system architecture as quickly as possible.

## 1. What is Archetype?
Archetype is a zero-latency MEV (Maximum Extractable Value) liquidation and arbitrage bot. It operates primarily on Arbitrum (an Ethereum Layer 2) interacting with the Aave V3 lending protocol and Uniswap V3 decentralized exchange.

**The Goal:** Find users on Aave whose collateral has dropped below the liquidation threshold (Health Factor < 1.0), take out a massive uncollateralized Flash Loan to repay their debt, seize their collateral at a discount, swap the collateral back to the original debt asset on Uniswap, repay the Flash Loan, and keep the difference as pure mathematical profit. All of this happens in a single, atomic blockchain transaction.

## 2. The Core Architecture (The Three Pillars)
The project is built on three distinct languages for optimal performance and safety. Do not attempt to rewrite the C++ engine in Python, or the Python brain in C++. They are separated for a reason.

### A. The Brain (Python 3.11+)
- **Location:** `brain/`
- **Role:** Intelligence, Ingestion, and Targeting.
- **How it works:** It connects to an Arbitrum RPC node via WebSockets (`listener.py`), downloads every new block, parses the Smart Contract logs (`decoder.py`), and maintains an active database of every user's Aave debt/collateral balance. It constantly checks live asset prices (`simulator.py`) using Chainlink oracles.
- **The Trigger:** When a target is found, it formats a JSON payload and fires it into a Redis queue (`ipc_sender.py`).

### B. The Muscle (C++17)
- **Location:** `engine/`
- **Role:** Zero-Latency Execution and Transaction Cryptography.
- **How it works:** It runs a continuous daemon (`main.cpp`) that blocks on the Redis queue. The millisecond Python pushes a target payload, C++ wakes up. It calculates the exact EVM payload bytes (`tx_builder.cpp`), fetches the current network gas price (`gas_manager.cpp`), serializes the transaction (`rlp.cpp`), physically hashes it (`keccak.cpp`), signs it with the private key (`signer.cpp`), and broadcasts it to the network.
- **Why C++?:** Generating RLP encodings and ECDSA signatures in Python takes ~40-80ms. C++ does it in <2ms. In MEV, 40ms is the difference between winning a $5,000 liquidation and losing your gas fee.

### C. The Assassin (Solidity 0.8.19)
- **Location:** `contracts/`
- **Role:** On-chain atomic execution.
- **How it works:** The C++ engine sends the transaction to this deployed contract (`ArchetypeFlashLoan.sol`). The contract requests the Aave Flash Loan, invokes the liquidation, approves the Uniswap V3 router, performs the swap (with strict slippage protection), and finally repays Aave.

---

## 3. The Tech Stack & Dependencies
You will need to be comfortable with the following tools to work on this codebase:
- **Redis:** Used exclusively as the Inter-Process Communication (IPC) bridge between Python and C++.
- **PostgreSQL (`asyncpg`):** Stores the actively synchronized Aave debt state. We use `ON CONFLICT DO UPDATE` heavily to prevent race conditions when thousands of debt events arrive per second.
- **Foundry (Forge/Cast):** The Solidity development framework used for compiling, testing, and deploying the smart contracts.
- **CMake & Make:** Used to compile the C++ Engine (requires `libcurl`, `hiredis`, `openssl`, and `secp256k1`).

---

## 4. Getting Started (Local Development)
Follow these steps to spin up the environment locally:

1. **Environment Variables:** Copy `.env.example` to `.env` in the root directory and fill in your WebSocket URL (Alchemy/QuickNode), Database details, and your development wallet Private Key.
2. **Infrastructure:** Run `docker compose up -d` to spin up the local PostgreSQL database and Redis server.
3. **Database Schema:** Run `psql -U postgres -d archetype_db -f database/schema.sql` to initialize the tables.
4. **Compile C++:** 
   ```bash
   cd engine
   mkdir build && cd build
   cmake .. && make
   ```
5. **Install Python Libs:** `cd brain && pip install -r requirements.txt`
6. **Compile Solidity:** `cd contracts && forge build`

---

## 5. Where to look next?
If you are diving into the codebase for the first time, read the following detailed documents in the `document/` folder in this exact order:
1. `functional_requirements_and_workflow.md` (To understand the exact step-by-step Mermaid sequence)
2. `detailed_code_explanation_python.md`
3. `detailed_code_explanation_cpp.md`
4. `detailed_code_explanation_solidity.md`
5. `project_summary_and_limitations.md` (To understand where the system currently struggles, e.g., Gas Wars)

Good luck, and welcome to the MEV trenches!
