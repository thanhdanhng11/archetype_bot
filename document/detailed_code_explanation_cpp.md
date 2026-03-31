# Detailed Code Explanation: C++ Engine

The C++ engine is the "muscle" of the Archetype bot. It values zero-latency execution. To achieve microseconds latency, it communicates with the Python "brain" via Redis (IPC), custom assembles EVM payloads mathematically, builds raw Ethereum transactions, signs them, and fires them via RPC.

## 1. `engine/include/types.hpp`
**Purpose:** Serves as the global memory contract and function declaration header for the engine.
- `#pragma once`: Prevents duplicate inclusions that would increase compilation time.
- `struct ExecutionPayload`: The strictest representation of an attack. It maps perfectly to what Python sends over Redis. It stores `target_user`, `debt_asset`, `collateral_asset`, `amount`, and `timestamp`. Using C++ `std::string` ensures memory is dynamically handled safely.
- Function Headers: Exposes methods like `parse_redis_payload`, `pad_to_32bytes`, `build_liquidation_tx`, `rlp_encode_item`, `generate_keccak256_hash`, and `sign_transaction` preventing circular dependency issues.

## 2. `engine/src/ipc_receiver.cpp`
**Purpose:** Handles parsing the raw JSON string fired by Python over Redis into the strict C++ `ExecutionPayload` struct.
- **Libraries:** `<nlohmann/json.hpp>`. A lightweight, single-header C++ JSON parsing library. We use this because writing native JSON parsers in C++ is error-prone.
- **Function - `parse_redis_payload(raw_json)`:** 
  - `json::parse(raw_json)` converts the string to an object. 
  - Using `j.at("target").get<std::string>()` explicitly maps JSON keys to the struct. `at()` is safer than `[]` because it throws a `json::out_of_range` error if the key is missing, allowing us to `catch` and immediately abort a flawed liquidation rather than accessing a null pointer and segfaulting the core engine.

## 3. `engine/src/tx_builder.cpp`
**Purpose:** Translates human-readable parameters into strict, EVM-compliant 32-byte hexadecimal calldata blocks.
- **Function - `pad_to_32bytes(hex_value)`:** Standard Ethereum addresses are 20 bytes (40 hex chars). To pack them into the transaction payload correctly, they must be 32 bytes (64 chars). This utility strips the `0x` prefix off incoming variables, compares its length to 64, and dynamically prepends leading zeros `std::string(64 - hex_value.length(), '0')` until it exactly matches the EVM slot size.
- **Function - `build_liquidation_tx(payload)`:** 
  - `method_id = "fa83bfae"`: The first 4 bytes of the Keccak256 hash of `initiateStrike(address,address,address,uint256)`. This is the exact function signature the EVM reads to know which Solidity function to execute.
  - Concatenates the ABI: `0x` + `method_id` + `pad_to_32bytes(payload.target_user)` + ...
  - **Optimization:** We perform simple string concatenation instead of invoking heavy EVM serialization libraries (like `ethabi`). This keeps payload generation under 1 millisecond.

## 4. `engine/src/gas_manager.cpp` & `rpc_client.cpp`
**Purpose:** Retrieves real-time EIP-1559 network states before firing the bullet.
- **Libraries:** `<curl/curl.h>` to perform HTTP POST requests directly to the Arbitrum RPC Node.
- `rpc_client.cpp` contains `fire_rpc_request`. It handles spinning up cURL, setting JSON-RPC headers, capturing the network's JSON response, and tearing down the cURL context safely to avoid memory leaks.
- `gas_manager.cpp` implements a **two-step EIP-1559 gas calculation:**
  1. `eth_getBlockByNumber(latest)` — extracts `baseFeePerGas` from the most recent mined block.
  2. `eth_maxPriorityFeePerGas` — fetches the network's current recommended tip.
  3. **Dynamic Formula:** `optimal_gas_price = (baseFee * 120 / 100) + priorityTip` — applies a 20% multiplier to ensure next-block inclusion. Falls back to `1 gwei` if both queries return zero.
- **Reasoning:** Hardcoded gas limits are prohibited in Phase 4+. Real-time gas ensures the transaction is competitive and isn't rejected by the sequencer.

## 5. `engine/src/rlp.cpp`
**Purpose:** Implements Ethereum's Recursive Length Prefix (RLP) serialization protocol from scratch.
- Standard JSON payloads cannot be signed. Ethereum requires variables (nonce, gas, data, etc.) to be heavily serialized into a very specific byte structure called RLP before Keccak hashing.
- **Function - `rlp_encode_item`:** Converts a single hex value (e.g., a nonce or gas price) into its shortest byte representation and appends an RLP length prefix. E.g., prefixes `0x80` to `0xb7` for specific length rules.
- **Function - `rlp_encode_list`:** Takes multiple `rlp_encode_item` strings, concatenates them, counts the cumulative length, and prepends the overarching Ethereum payload list header (e.g., `0xc0` to `0xf7` length bytes).

## 6. `engine/src/keccak.cpp` & `signer.cpp`
**Purpose:** Applies the cryptographic seal to the RLP-encoded payload wrapper.
- **Libraries:** `<openssl/evp.h>` (for Keccak / SHA3) and `<secp256k1.h>` (for Elliptic Curve Digital Signature Algorithm).
- **Function - `generate_keccak256_hash`:** Feeds the `unsigned_RLP_hex` into the OpenSSL digest. The EVM strictly uses the `SHA3_256` variant (Keccak-256).
- **Function - `sign_transaction`:** Takes the generated Keccak Hash and the `.env` `PRIVATE_KEY`. 
  - Creates a `secp256k1_context`.
  - Computes the Recoverable Signature (`secp256k1_ecdsa_sign_recoverable`). 
  - Extracts the exact `v`, `r`, and `s` elliptic curve components. 
  - By managing `secp256k1` internally, we completely bypass the standard Python web3.py signing delay (~40-80ms), reducing signature generation to less than 2 milliseconds.

## 7. `engine/src/main.cpp`
**Purpose:** The central nervous system loop that ties all previous modules into executing sequentially.
- **Initialization:** Connects to Redis via `hiredis` on port 6379 natively.
- **Execution Loop:**
  1. `BLPOP archetype_execute 0`: The core optimization. Instead of polling the DB every second, `BLPOP` sleeps the C++ thread indefinitely until Python `LPUSH`es a payload to the `archetype_execute` channel. The very nanosecond a payload is written, C++ awakes and executes. Zero CPU waste, Zero latency.
  2. `parse_redis_payload(raw_payload)` — dynamically rips the `target`, `asset`, `collateral`, and `amount` fields from the live JSON payload.
  3. Uses `tx_builder` to pad variables exactly 32 bytes and prepend `fa83bfae` (the `initiateStrike` function selector).
  4. Fetches Nonce and dynamic EIP-1559 Gas via `gas_manager`.
  5. Wraps the parameters via `rlp_encode_list` into an `unsigned_RLP_payload`.
  6. Hashes via `generate_keccak256_hash`.
  7. Generates the `(v, r, s)` cryptographic signature using the `PRIVATE_KEY` via `sign_transaction`.
  8. Applies EIP-155 Chain ID replay protection mathematically: `V = recovery_id + (CHAIN_ID * 2) + 35`.
  9. Wraps parameters AGAIN including the `(v, r, s)` variables into the `final_signed_RLP_payload`.
  10. Fires the exact hex string to the Arbitrum Sepolia RPC via `eth_sendRawTransaction`.

## 8. `engine/tests/crypto_tests.cpp`
**Purpose:** Native C++ unit test suite validating the core cryptographic utilities.
- **Compile & Run:** `g++ -std=c++17 -o run_tests engine/tests/crypto_tests.cpp && ./run_tests`
- **Tests included:** 7 assertions covering `pad_to_32bytes` stripping and padding precision, `hex_to_bytes` round-trip fidelity, and `rlp_encode_item` boundary encoding.
- **Status:** All 7 tests pass.
