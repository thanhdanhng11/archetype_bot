# Archetype Flash Loan System: Functional Requirements & Workflow

This document covers the functional requirements, detailing exactly what the system does, followed by a step-by-step execution flowchart visualizing the architecture.

## Functional Requirements

### 1. Blockchain State Ingestion (The Brain)
- The system must connect to the Arbitrum Layer 2 network via WebSockets.
- It must listen continuously for new blocks (`newHeads`).
- For every new block, it must fetch all logs and filter them for Aave V3 `Borrow` and `Repay` events.
- It must decode the raw hexadecimal logs into standard JSON/Dictionary objects representing the User, Asset, and Amount.

### 2. State Maintenance (The Database)
- The system must insert new users into a PostgreSQL `lending_positions` database.
- It must increase debt amounts when `Borrow` events occur.
- It must decrease debt amounts when `Repay` events occur.
- It must save its current sync state to resume sequentially without missing blocks.

### 3. Oracle Price Simulation
- The system must connect to Chainlink Price Feeds.
- It must fetch the real-time USD prices of Debt and Collateral assets to calculate a user's Health Factor.
- If a target's Health Factor drops below `1.0`, it must trigger the execution pipeline.

### 4. Inter-Process Communication (IPC)
- The Python Brain must format the attack parameters (Target User, Debt Asset, Collateral Asset, Debt Amount).
- It must push this JSON payload instantly to a Redis List (`liquidation_orders`) via `LPUSH`.

### 5. Transaction Crafting (The Engine)
- The C++ Engine must block on the Redis List via `BLPOP` to catch the target payload in zero time.
- It must dynamically pad variable-length addresses and amounts to standard 32-byte EVM words.
- It must append these padded arguments directly behind the execution function selector `fa83bfae`.
- It must RLP-encode the transaction data, hash it via Keccak256, and sign it with ECDSA (secp256k1).
- It must broadcast the raw transaction to the RPC node.

### 6. On-Chain Execution (The Smart Contract)
- The Smart Contract must initiate a Flash Loan from Aave V3 for the specified debt amount.
- In the callback, it must approve Aave to use the flash loaned funds.
- It must trigger `liquidationCall` on Aave, receiving the seized collateral token (e.g., WETH) at a discount.
- It must approve the Uniswap V3 Router to swap the seized collateral.
- It must swap the collateral back to the original debt asset (e.g., USDC), enforcing an `amountOutMinimum` to cover the flash loan debt plus the premium fee.
- It must retain the remaining arbitrage profit securely within the contract.

---

## Step-by-Step System Workflow

```mermaid
sequenceDiagram
    participant B as Python Brain (Ingestion)
    participant O as Chainlink (Oracle)
    participant DB as PostgreSQL (State)
    participant R as Redis (IPC Queue)
    participant E as C++ Engine (Muscle)
    participant Node as RPC Node
    participant SC as Smart Contract (Archetype)
    participant Aave as Aave V3 Pool
    participant Uni as Uniswap V3 Router

    Note over B: 1. Listen for newBlocks
    B->>Node: Subscribe to newHeads (WSS)
    Node-->>B: Block 123456 Logs
    B->>B: Decode Borrows & Repays
    
    Note over B, DB: 2. Update System State
    B->>DB: UPSERT Debt Positions (+ or -)
    B->>DB: UPDATE sync_state (Block 123456)
    
    Note over B, O: 3. Health Factor Simulation
    B->>O: Get Live Asset Prices (USD)
    O-->>B: Price Data
    B->>B: Calculate HF < 1.0 (Target Found!)
    
    Note over B, R: 4. Fire the Kill Signal
    B->>R: LPUSH liquidation_orders (Target, Debt, Collateral, Amount)
    
    Note over E, R: 5. Payload Assembly & Signing
    R-->>E: BLPOP triggers C++ Engine
    E->>E: Zero-pad to 32 bytes & append to fa83bfae selector
    E->>Node: Get Nonce & Gas Price
    Node-->>E: nonce: 4, gas: 0.1 gwei
    E->>E: RLP Encode -> Keccak256 Hash -> ECDSA Sign
    
    Note over E, Node: 6. Execution Broadcast
    E->>Node: eth_sendRawTransaction(signedTx)
    
    Note over Node, SC: 7. On-Chain Liquidation
    Node->>SC: call initiateStrike(user, debt, col, amt)
    SC->>Aave: flashLoanSimple(amount)
    Aave-->>SC: executeOperation() callback (Funds transferred)
    SC->>Aave: liquidationCall(user) (Repay debt, Receive Collateral at discount)
    Aave-->>SC: Sends seized Collateral (e.g., WETH)
    SC->>Uni: exactInputSingle() (Swap Collateral back to Debt token)
    Uni-->>SC: Sends swapped Debt token (e.g., USDC)
    SC->>Aave: Repay Flash Loan Principal + Premium
    Note over SC: 8. Profit Retained in Contract
```
