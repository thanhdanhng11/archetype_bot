// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

import {Script, console} from "forge-std/Script.sol";
import {ArchetypeFlashLoan} from "../src/ArchetypeFlashLoan.sol";

contract DeployArchetype is Script {
    function run() external {
        // 1. Securely load your Private Key from a hidden environment file
        uint256 deployerPrivateKey = vm.envUint("PRIVATE_KEY");

        // 2. Arbitrum Mainnet Protocol Addresses
        address aaveV3PoolMainnet = 0x794a61358D6845594F94dc1DB02A252b5b4814aD; 
        address uniswapV3RouterMainnet = 0xE592427A0AEce92De3Edee1F18E0157C05861564; 

        // 3. Instruct the EVM to start signing transactions with your key
        vm.startBroadcast(deployerPrivateKey);

        // 4. DEPLOY THE WEAPON
        // This takes your compiled bytecode and physically permanently writes it to Arbitrum
        ArchetypeFlashLoan archetype = new ArchetypeFlashLoan(
            aaveV3PoolMainnet, 
            uniswapV3RouterMainnet
        );

        // 5. Stop signing
        vm.stopBroadcast();

        // 6. Print the target address to the console so we can feed it to our C++ Engine
        console.log("[+] Archetype Assassin Deployed To:", address(archetype));
    }
}