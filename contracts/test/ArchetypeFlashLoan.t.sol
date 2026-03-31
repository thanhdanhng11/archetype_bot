// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

import {Test, console} from "forge-std/Test.sol";
import {ArchetypeFlashLoan, IERC20} from "../src/ArchetypeFlashLoan.sol";

// Minimal ERC20 mock that returns 0 balance so we properly test the "No profit" revert
contract MockERC20 is IERC20 {
    function balanceOf(address) external pure override returns (uint256) { return 0; }
    function transfer(address, uint256) external pure override returns (bool) { return true; }
    function approve(address, uint256) external pure override returns (bool) { return true; }
}

contract ArchetypeFlashLoanTest is Test {
    ArchetypeFlashLoan public archetype;
    MockERC20 public mockToken;

    // Test addresses
    address owner    = address(0x1);
    address attacker = address(0x2);
    address aavePool  = address(0x3);
    address uniRouter = address(0x4);

    function setUp() public {
        // Deploy mock token
        mockToken = new MockERC20();
        // Impersonate the owner when deploying the main contract
        vm.prank(owner);
        archetype = new ArchetypeFlashLoan(aavePool, uniRouter);
    }

    // =====================================================
    // TEST 1: Only the owner can trigger initiateStrike
    // =====================================================
    function test_OnlyOwnerCanInitiateStrike() public {
        vm.prank(attacker);
        vm.expectRevert("Archetype: Unauthorized execution");
        archetype.initiateStrike(address(0x5), address(0x6), address(0x7), 1000000);
    }

    // =====================================================
    // TEST 2: Only the owner can withdraw profits
    // =====================================================
    function test_OnlyOwnerCanWithdrawProfit() public {
        vm.prank(attacker);
        vm.expectRevert("Archetype: Unauthorized execution");
        archetype.withdrawProfit(address(mockToken));
    }

    // =====================================================
    // TEST 3: executeOperation rejects external initiators
    // =====================================================
    function test_ExecuteOperationRejectsUntrustedInitiator() public {
        vm.prank(aavePool);
        vm.expectRevert("Archetype: External initiator not allowed");
        archetype.executeOperation(address(mockToken), 1000000, 100, attacker, "");
    }

    // =====================================================
    // TEST 4: executeOperation rejects calls NOT from Aave
    // =====================================================
    function test_ExecuteOperationRejectsNonAaveCallers() public {
        vm.prank(attacker);
        vm.expectRevert("Archetype: Untrusted loan initiator");
        archetype.executeOperation(address(mockToken), 1000000, 100, address(archetype), "");
    }

    // =====================================================
    // TEST 5: withdrawProfit reverts when contract has no profit
    // =====================================================
    function test_WithdrawProfitRevertsOnEmptyBalance() public {
        vm.prank(owner);
        vm.expectRevert("Archetype: No profit to withdraw");
        archetype.withdrawProfit(address(mockToken));
    }
}
