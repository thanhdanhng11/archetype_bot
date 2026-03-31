# Detailed Code Explanation: Python Brain

The Python Brain is the intelligence center of the Archetype Bot. While C++ is optimized for speed, Python is optimized for robust network processing, API abstractions, and complex logic parsing. Its core job is to listen to the blockchain, track the debt of every user on Aave across millions of blocks, simulate prices via Oracles, and target liquidatable users.

## 1. `brain/db_client.py`
**Purpose:** Manages the heavy synchronization of network state via PostgreSQL.
- **Libraries:** heavily relies on `asyncpg`. Standard `psycopg2` requires thread locks and is strictly synchronous. `asyncpg` operates entirely in `asyncio` loops, allowing the bot to parse thousands of blockchain events seamlessly.
- **Function - `connect()`:** Instantiates an `asyncpg.create_pool` with `min_size=1`, `max_size=10`. Pooling is vital; generating a brand-new TCP connection to a database for every Ethereum block takes up to 50ms. A pool holds connections open indefinitely, making queries instantaneous.
- **Function - `update_sync_state()`:** Performs an `ON CONFLICT DO UPDATE` query to save `last_processed_block`. During sudden network drops or container restarts, the bot reads this to avoid scanning already-parsed blocks or skipping unparsed blocks. Fault zero tolerance.
- **Function - `update_debt_position()`:** Liquidations require strict mathematical accuracy mapping the user, protocol, and asset columns to a single debt amount. 
  - `amount_delta` dictates logic. Borrows are (+), Repays are (-).
  - Uses `ON CONFLICT DO UPDATE SET debt_amount = lending_positions.debt_amount + EXCLUDED.debt_amount`. The exact atomic addition is offloaded purely to the PostgreSQL engine, eliminating race conditions entirely if multiple events hit the same user at the same timestamp.

## 2. `brain/decoder.py`
**Purpose:** Ingests raw hexadecimal EVM Event Logs and converts them into structured dictionaries.
- **Reasoning:** A blockchain doesn't emit "User Alice borrowed 5 WETH". It emits Topic Hashes and raw hex strings padded to 32 bytes.
- **Initialization:** Loads standard ABI (Application Binary Interface) JSONs for `erc20` and `aave_pool`. It then initializes a `web3.py contract` purely for its event decoding abstraction engine.
- **Function - `decode_aave_borrow(raw_log)`:** Uses `aave_contract.events.Borrow().process_log(raw_log)` to perfectly extract `user`, `reserve` (asset), and `amount`. Returns structured JSON.
- **Function - `decode_aave_repay(raw_log)`:** Uses identical logic on `events.Repay()`. 

## 3. `brain/listener.py` & `main.py`
**Purpose:** The central nervous system loop that synchronizes the L2 node to the PostgreSQL database.
- **Libraries:** `web3` (AsyncWeb3) over `WebSocketProvider`. Standard HTTP polling is too slow and misses blocks. WebSockets keep an active TCP layer stream open with an Arbitrum RPC node.
- **Execution Flow (`main_loop`):**
  1. Bootloads `DatabaseClient` and connects pool.
  2. Bootloads `EventDecoder`.
  3. `w3.eth.subscribe('newHeads')`: Asks the Arbitrum node to push every newly mined block header directly into the bot's memory space the millisecond it's hashed.
  4. `async for event in w3.socket.process_subscriptions()`: Python's async execution loop spins indefinitely catching these pushed headers.
  5. `block_number = int(block_data['number'], 16)`: Extracts the absolute block integer.
  6. `w3.eth.get_logs({"fromBlock": block_number, "toBlock": block_number})`: Fires an RPC request querying specifically for ALL smart contract events inside the exact target block.
  7. Loops sequentially over the raw `logs` returned, throwing them against `decode_aave_borrow` and `decode_aave_repay`. 
  8. For every active decode, it pushes the `delta_amount` into `db.update_debt_position()`.
  9. Finally, it checkpoints the bot's state using `db.update_sync_state()`.

## 4. `brain/simulator.py`
**Purpose:** Prices assets using decentralized physical Oracles (Chainlink) to avoid local price manipulation attacks.
- **Initialization:** Mappings to specific proxy Chainlink feeds e.g., `ETH_USD (0x639F...)`.
- **Function - `get_live_price(feed_address)`:** Calls `latestRoundData()` across a standard `AsyncWeb3` HTTP connection. 
- **Optimization:** Chainlink historically uses 8 decimal accuracy for USD pairs (e.g. `2000000000` = `$2,000`). It calculates `raw_price / (10 ** 8)` to provide floating-point standardized calculations used later against Aave debt positions to perfectly calculate Health Ratios.

## 5. `brain/ipc_sender.py`
**Purpose:** Bridges the gap between Python (Analysis) and C++ (Execution).
- **Libraries:** `redis`. Redis operates entirely in RAM. Standard IPC (like Unix sockets/Pipelines) can break across Docker containers. Redis Lists bypass this safely.
- **Function - `fire_kill_signal()`:** Accepts isolated parameters (`target_user`, `debt_asset`, `collateral_asset`, `amount`).
- **Data Safety:** Python floats/integers lose precision in Javascript/JSON due to byte limits (safe max is `9007199254740991`). Ethereum deals in Wei (`10**18`). `ipc_sender` forcefully converts `str(amount_to_repay)` into a String before `json.dumps()` serialization to guarantee mathematically flawless data transit.
- **Execution:** Calls `self.redis_client.lpush("archetype_execute", json)`. This command perfectly awakens the sleeping `BLPOP` C++ Engine thread.

## 6. `brain/mock_injector.py`
**Purpose:** Phase 4 E2E Testnet Simulation trigger script. Bypasses live mempool scanning and injects a synthetic Arbitrum Sepolia target into the pipeline for validation.
- **Channel:** Publishes to `archetype_execute` — the same channel the C++ engine subscribes to.
- **Payload:** Uses Sepolia USDC (`0x75faf...`) as the debt asset and Sepolia WETH (`0x980B...`) as the collateral, with an amount of `1,000,000` (1 USDC, 6 decimals).
- **Run:** `venv/bin/python brain/mock_injector.py`

## 7. `brain/tests/test_decoder.py` & `test_ipc.py`
**Purpose:** PyTest automation suite that validates the Python Brain outputs are stable and accurate.
- `test_decoder.py` — mocks the Web3 event pipeline and asserts `decode_aave_borrow` and `decode_aave_repay` return correct typed dictionaries with lowercased addresses.
- `test_ipc.py` — mocks the Redis client and asserts that `fire_kill_signal` pushes to the correct `liquidation_orders` queue key, applies lowercase enforcement, and casts amounts as strings.
- **Run:** `venv/bin/python -m pytest brain/tests/ -v`
- **Status:** All 3 tests pass.
