// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

// ==========================================
// PILLAR 1: THE INTERFACES
// ==========================================

interface IERC20 {
    function balanceOf(address account) external view returns (uint256);
    function transfer(address recipient, uint256 amount) external returns (bool);
    function approve(address spender, uint256 amount) external returns (bool);
}

interface IPool {
    function liquidationCall(
        address collateralAsset,
        address debtAsset,
        address user,
        uint256 debtToCover,
        bool receiveAToken
    ) external;
    
    function flashLoanSimple(
        address receiverAddress,
        address asset,
        uint256 amount,
        bytes calldata params,
        uint16 referralCode
    ) external;
}

interface IFlashLoanSimpleReceiver {
    function executeOperation(
        address asset,
        uint256 amount,
        uint256 premium,
        address initiator,
        bytes calldata params
    ) external returns (bool);
}

// NEW: Uniswap V3 Router Interface
interface ISwapRouter {
    struct ExactInputSingleParams {
        address tokenIn;
        address tokenOut;
        uint24 fee;
        address recipient;
        uint256 deadline;
        uint256 amountIn;
        uint256 amountOutMinimum;
        uint160 sqrtPriceLimitX96;
    }

    function exactInputSingle(ExactInputSingleParams calldata params) external payable returns (uint256 amountOut);
}

// ==========================================
// PILLAR 2: THE CONTRACT CORE
// ==========================================

contract ArchetypeFlashLoan is IFlashLoanSimpleReceiver {
    address public owner;
    IPool public aavePool;
    ISwapRouter public swapRouter; // NEW: The Uniswap execution variable

    modifier onlyOwner() {
        _checkOwner();
        _;
    }

    function _checkOwner() internal view {
        require(msg.sender == owner, "Archetype: Unauthorized execution");
    }

    // NEW: We now pass the Uniswap Router address when we deploy the contract
    constructor(address _aavePoolAddress, address _swapRouterAddress) {
        owner = msg.sender;
        aavePool = IPool(_aavePoolAddress);
        swapRouter = ISwapRouter(_swapRouterAddress);
    }

    // ==========================================
    // THE ENTRY POINT (Triggered by C++ Engine)
    // ==========================================
    function initiateStrike(
        address targetUser,
        address debtAsset,
        address collateralAsset,
        uint256 debtToCover
    ) external onlyOwner {
        
        bytes memory params = abi.encode(targetUser, debtAsset, collateralAsset);

        aavePool.flashLoanSimple(
            address(this), 
            debtAsset,     
            debtToCover,   
            params,        
            0              
        );
    }

    // ==========================================
    // PILLARS 3 & 4: THE CALLBACK & EXECUTION
    // ==========================================
    
    function executeOperation(
        address asset,
        uint256 amount,
        uint256 premium,
        address initiator,
        bytes calldata params
    ) external returns (bool) {
        
        require(msg.sender == address(aavePool), "Archetype: Untrusted loan initiator");
        require(initiator == address(this), "Archetype: External initiator not allowed");

        (address targetUser, address debtAsset, address collateralAsset) = abi.decode(params, (address, address, address));

        IERC20(debtAsset).approve(address(aavePool), amount);

        aavePool.liquidationCall(
            collateralAsset,
            debtAsset,
            targetUser,
            amount,
            false
        );

        uint256 amountToOwe = amount + premium;

        // ==========================================
        // THE MISSING LINK: UNISWAP V3 DEX SWAP
        // ==========================================
        
        // 1. Check exactly how much collateral (e.g., WETH) we successfully seized
        uint256 collateralBalance = IERC20(collateralAsset).balanceOf(address(this));
        
        // 2. Approve Uniswap to take that seized collateral
        IERC20(collateralAsset).approve(address(swapRouter), collateralBalance);

        // 3. Set the strict mathematical parameters for the trade
        ISwapRouter.ExactInputSingleParams memory swapParams = ISwapRouter.ExactInputSingleParams({
            tokenIn: collateralAsset,
            tokenOut: asset,           // We need the original debt asset back (e.g., USDC)
            fee: 3000,                 // The standard 0.3% Uniswap pool fee
            recipient: address(this),  // Send the swapped USDC back to this contract
            deadline: block.timestamp,
            amountIn: collateralBalance,
            amountOutMinimum: amountToOwe, // THE FLOOR: We must get AT LEAST what we owe Aave, or the transaction violently reverts
            sqrtPriceLimitX96: 0
        });

        // 4. Pull the trigger on the swap
        swapRouter.exactInputSingle(swapParams);

        // ==========================================
        // END OF SWAP
        // ==========================================

        // Repay the Flash Loan: Approve Aave to take back the principal + fee.
        IERC20(asset).approve(address(aavePool), amountToOwe);

        return true;
    }

    // ==========================================
    // PILLAR 5: THE VAULT
    // ==========================================
    
    function withdrawProfit(address tokenAddress) external onlyOwner {
        uint256 balance = IERC20(tokenAddress).balanceOf(address(this));
        require(balance > 0, "Archetype: No profit to withdraw");
        
        bool  success = IERC20(tokenAddress).transfer(owner, balance);
        require(success, "Archetype: Profit withdrawal failed.");
    }
}