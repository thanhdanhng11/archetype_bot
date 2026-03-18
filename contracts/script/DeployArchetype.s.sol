// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

import "forge-std/Script.sol";
import "../src/ArchetypeFlashLoan.sol";

contract DeployArchetype is Script {
    function run() external {
        // 1. Securely load your Private Key from a hidden environment file
        uint256 deployerPrivateKey = vm.envUint("PRIVATE_KEY");

        // 2. Arbitrum Sepolia Testnet Protocol Addresses
        address aaveV3PoolSepolia = 0x6Ae43d3271ff6888e7Fc43Fd7321a503ff738951; 
        address uniswapV3RouterSepolia = 0x101f443b4d1B059569d6439175deD12E0F228f5f; 

        // 3. Instruct the EVM to start signing transactions with your key
        vm.startBroadcast(deployerPrivateKey);

        // 4. DEPLOY THE WEAPON
        // This takes your compiled bytecode and physically permanently writes it to Arbitrum
        ArchetypeFlashLoan archetype = new ArchetypeFlashLoan(
            aaveV3PoolSepolia, 
            uniswapV3RouterSepolia
        );

        // 5. Stop signing
        vm.stopBroadcast();

        // 6. Print the target address to the console so we can feed it to our C++ Engine
        console.log("[+] Archetype Assassin Deployed To:", address(archetype));
    }
}