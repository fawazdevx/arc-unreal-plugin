// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

interface IERC20 {
    function balanceOf(address account) external view returns (uint256);
    function transferFrom(address from, address to, uint256 value) external returns (bool);
}

contract GameCore {
    IERC20 public immutable usdcToken;
    address public owner;

    struct PlayerProfile {
        uint256 totalSpentUSDC;
        uint256 totalEarnedUSDC;
        uint256 lastUpdated;
        bool exists;
    }

    mapping(address => PlayerProfile) private _profiles;

    event PlayerRegistered(address indexed player);
    event PaymentRecorded(address indexed player, uint256 amount, string reason);
    event RewardRecorded(address indexed player, uint256 amount, string reason);

    modifier onlyOwner() {
        require(msg.sender == owner, "Not contract owner");
        _;
    }

    constructor(address _usdcToken) {
        require(_usdcToken != address(0), "USDC token required");
        usdcToken = IERC20(_usdcToken);
        owner = msg.sender;
    }

    function registerPlayer(address player) public {
        require(player != address(0), "Invalid player");
        PlayerProfile storage p = _profiles[player];
        if (!p.exists) {
            p.exists = true;
            p.lastUpdated = block.timestamp;
            emit PlayerRegistered(player);
        }
    }

    function getPlayerProfile(address player)
        external
        view
        returns (
            uint256 totalSpentUSDC,
            uint256 totalEarnedUSDC,
            uint256 lastUpdated,
            bool exists
        )
    {
        PlayerProfile memory p = _profiles[player];
        return (p.totalSpentUSDC, p.totalEarnedUSDC, p.lastUpdated, p.exists);
    }

    function getUSDCBalance(address player) external view returns (uint256) {
        return usdcToken.balanceOf(player);
    }

    /**
     * @dev Record a payment (for example, purchasing an in-game item).
     * The player must have approved this contract to spend `amount` USDC beforehand.
     */
    function payWithUSDC(uint256 amount, string calldata reason) external {
        require(amount > 0, "Amount must be > 0");

        // Pull USDC from player into this contract
        bool ok = usdcToken.transferFrom(msg.sender, address(this), amount);
        require(ok, "USDC transfer failed");

        // Update profile
        PlayerProfile storage p = _profiles[msg.sender];
        if (!p.exists) {
            p.exists = true;
            emit PlayerRegistered(msg.sender);
        }
        p.totalSpentUSDC += amount;
        p.lastUpdated = block.timestamp;

        emit PaymentRecorded(msg.sender, amount, reason);
    }

    /**
     * @dev Record a reward sent from this contract to the player.
     * For example, game rewards denominated in USDC.
     */
    function rewardUSDC(address player, uint256 amount, string calldata reason) external onlyOwner {
        require(player != address(0), "Invalid player");
        require(amount > 0, "Amount must be > 0");

        bool ok = usdcToken.transferFrom(msg.sender, player, amount);
        require(ok, "USDC reward transfer failed");

        PlayerProfile storage p = _profiles[player];
        if (!p.exists) {
            p.exists = true;
            emit PlayerRegistered(player);
        }
        p.totalEarnedUSDC += amount;
        p.lastUpdated = block.timestamp;

        emit RewardRecorded(player, amount, reason);
    }
}