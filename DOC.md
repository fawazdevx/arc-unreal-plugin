# DOC - @Arc Unreal Engine Plugin & Backend (Detailed Guide)

This document is the full guide to the @Arc Unreal plugin + backend:

- Architecture and design choices
- How the contracts, backend, and Unreal plugin fit together
- How to customize for your own game
- Best practices for @Arc game devs

If you just want to get running quickly with the sample setup, use [`SETUP.md`](./SETUP.md).

---

## 1. High-Level Architecture

### 1.1. Components

1. **Smart Contracts (blockchain/)**

   - `GameCore.sol`
     - Manages player profiles and USDC financials.
     - Uses an `IERC20` interface to the @Arc USDC token.
     - Functions:
       - `registerPlayer(address player)`
       - `payWithUSDC(uint256 amount, string reason)`
       - `rewardUSDC(address player, uint256 amount, string reason)`
       - `getUSDCBalance(address player)` (reads via `usdcToken.balanceOf(player)`)
       - `getPlayerProfile(address player)` (total spent/earned, etc)

   - `GameNFT.sol`
     - Simple ERC‑721 contract for minting NFTs (e.g., achievements, items).

2. **Backend (backend/server.js)**

   - Node.js + Express + ethers v6.
   - Holds a **server wallet** (`PRIVATE_KEY`) used to:
     - Call `GameCore` as owner.
     - Call `GameNFT` for minting.
   - Provides HTTP API for the game client:
     - `POST /registerPlayer`
     - `POST /payWithUSDC`
     - `POST /rewardUSDC`
     - `POST /mintNFT`
     - `GET /health`

3. **Unreal Plugin (Plugins/ArcBlockchain)**

   - C++ plugin exposing Blueprint nodes.
   - Uses **direct JSON‑RPC** to read from @Arc (`GetUSDCBalanceAsync`).
   - Uses **HTTP** to talk to backend for writes.

### 1.2. Why this design?

- **Security**: Write operations (spend/reward USDC, mint NFTs) require signing and gas. You don’t want private keys in the client. So:
  - Client → Backend → Chain
- **Simplicity for UE devs**: Blueprint nodes hide all ethers/HTTP/JSON details.
- **Flexibility**: Backend can implement arbitrary game logic, rate‑limiting, anti‑cheat, etc, without changing the plugin.

---

## 2. Smart Contract Details

### 2.1. GameCore.sol

Simplified view:

```solidity
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

    constructor(address _usdcToken) {
        require(_usdcToken != address(0), "USDC token required");
        usdcToken = IERC20(_usdcToken);
        owner = msg.sender;
    }

    function registerPlayer(address player) public { ... }

    function getPlayerProfile(address player) external view returns (...) { ... }

    function getUSDCBalance(address player) external view returns (uint256) {
        return usdcToken.balanceOf(player);
    }

    function payWithUSDC(uint256 amount, string calldata reason) external {
        require(amount > 0, "Amount must be > 0");
        bool ok = usdcToken.transferFrom(msg.sender, address(this), amount);
        require(ok, "USDC transfer failed");
        // update profile, emit events...
    }

    function rewardUSDC(address player, uint256 amount, string calldata reason) external onlyOwner {
        require(player != address(0), "Invalid player");
        require(amount > 0, "Amount must be > 0");
        bool ok = usdcToken.transferFrom(msg.sender, player, amount);
        require(ok, "USDC reward transfer failed");
        // update profile, emit events...
    }
}
```

Key points:

- **USDC decimals**: typically **6** (1 USDC = `1_000_000` units).
- `payWithUSDC`:
  - `msg.sender` is the onchain caller.
  - Requires **allowance**: `allowance(msg.sender, GameCore) >= amount`.
  - Pulls USDC from `msg.sender` → GameCore contract.
- `rewardUSDC`:
  - Runs as **owner** (backend signer) and transfers from owner to player (using `transferFrom`).
  - Requires **allowance**: `allowance(owner, GameCore) >= amount`.

This pattern gives:

- Explicit consent via `approve`.
- Owner‑controlled rewards.

### 2.2. Approvals Required

For everything to work, both wallets typically approve GameCore to spend USDC:

1. Player approves GameCore:

   ```text
   USDC.approve(GameCore, LARGE_ALLOWANCE) from player address
   ```

2. Owner / backend signer approves GameCore:

   ```text
   USDC.approve(GameCore, LARGE_ALLOWANCE) from owner address
   ```

---

## 3. Backend Design (server.js)

### 3.1. Provider & wallet

```js
const provider = new ethers.JsonRpcProvider(ARC_RPC_URL);
const wallet   = new ethers.Wallet(PRIVATE_KEY, provider);

const gameCore = new ethers.Contract(GAME_CORE_ADDRESS, gameCoreJson.abi, wallet);
const gameNft  = new ethers.Contract(GAME_NFT_ADDRESS, gameNftJson.abi, wallet);
```

- `wallet` is the server wallet.
- `gameCore` / `gameNft` are pre‑connected with this signer.

### 3.2. Unified tx handler

```js
async function handleTx(res, txPromise) {
  try {
    const tx = await txPromise;
    const receipt = await tx.wait();
    return res.json({ ok: true, txHash: receipt.transactionHash });
  } catch (err) {
    console.error(err);
    return res.status(500).json({ ok: false, error: err.message });
  }
}
```

All routes call `handleTx(res, gameCore.someFunction(...))`.

### 3.3. Routes

#### `/registerPlayer`

```js
app.post("/registerPlayer", async (req, res) => {
  const { player } = req.body;
  if (!player) return res.status(400).json({ ok: false, error: "Missing player" });
  return handleTx(res, gameCore.registerPlayer(player));
});
```

- Registers a profile onchain.
- Emits `PlayerRegistered` event.

#### `/payWithUSDC`

```js
app.post("/payWithUSDC", async (req, res) => {
  const { from, amount, reason } = req.body;
  if (!from || !amount)
    return res.status(400).json({ ok: false, error: "Missing from or amount" });

  return handleTx(res, gameCore.payWithUSDC(amount, reason || ""));
});
```

**Important clarification:** In this reference backend, transactions are signed by the backend wallet. That means the onchain caller (`msg.sender`) is the backend signer. If you want the player to be the onchain sender, you need a player-signed transaction flow or a meta-transaction/permit pattern (not included by default).

#### `/rewardUSDC`

```js
app.post("/rewardUSDC", async (req, res) => {
  const { player, amount, reason } = req.body;
  if (!player || !amount)
    return res.status(400).json({ ok: false, error: "Missing player or amount" });

  return handleTx(res, gameCore.rewardUSDC(player, amount, reason || ""));
});
```

Runs as owner wallet; see approvals above.

#### `/mintNFT`

```js
app.post("/mintNFT", async (req, res) => {
  const { to, tokenURI } = req.body;
  if (!to || !tokenURI)
    return res.status(400).json({ ok: false, error: "Missing to or tokenURI" });

  return handleTx(res, gameNft.mintTo(to, tokenURI));
});
```

Mints an ERC‑721 to the player.

---

## 4. Unreal Plugin Design

### 4.1. GetUSDCBalanceAsync

- Implements `UBlueprintAsyncActionBase`.
- Calls @Arc RPC `eth_call` with `data = selector + padded address` against `GameCore`.
- Parses `uint256` and converts raw → float using 6 decimals.

Blueprint usage:

- Node: `Get USDC Balance Async`
- Inputs: `PlayerAddress`
- Outputs:
  - `OnSuccess(BalanceRaw:int64, BalanceUSDC:float)`
  - `OnFailure()`

### 4.2. Backend Action Nodes

Currently, you have synchronous BlueprintCallable functions (like `BackendRegisterPlayer`) that:

- Take `BackendBaseUrl`, addresses, amounts.
- Fire an HTTP POST.
- Immediately return `Success = true` + `TxHashOrError = "Request sent (check logs)"`.
- Log actual result in the Output Log when the HTTP completes.

This is fine for internal testing. For other devs, you may want to add async Blueprint nodes that:

- Wait for HTTP response.
- Parse `{ ok, txHash, error }`.
- Expose:
  - `OnSuccess(TxHash)`
  - `OnFailure(Error)`

---

## 5. End-to-End Flow Examples

### 5.1. Player buys an item with USDC

1. One‑time setup:
   - Player approves GameCore to spend USDC.
2. In game:
   - Client calls backend endpoint for a purchase request (or uses a player-signed transaction flow).
   - Backend submits a transaction.
   - Contract records the purchase.
   - Client confirms using tx hash and updates the UI.

### 5.2. Server rewards player with USDC

1. One‑time setup:
   - Owner approves GameCore to spend owner’s USDC.
2. In game:
   - Player completes quest.
   - Backend calls `rewardUSDC(player, amount, reason)`.
   - Player balance increases.

### 5.3. Minting an NFT achievement

- Backend mints `GameNFT` to a player with a specific `tokenURI`.
- Client shows a “New NFT unlocked” UI and links to explorer.

---

## 6. Adapting This to Your Own Game

Things you’ll likely change:

1. Contracts:
   - Add fields to `PlayerProfile`
   - Add new functions for game actions and items
2. Backend:
   - Add authentication and game rules
   - Add new endpoints for your contract methods
3. Plugin:
   - Add more Blueprint nodes (read + backend calls)
4. UX:
   - Add transaction status UI (pending/confirmed)
   - Add inventory/NFT gallery widgets

---

## 7. Tips & Best Practices

- Treat the backend as authoritative.
- Be explicit with token units:
  - USDC uses 6 decimals (`5 USDC = 5,000,000` raw units).
- Log and display tx hashes for debugging and transparency.
- Separate testnet vs mainnet config.

---


If you use this setup, consider contributing back via issues/PRs.