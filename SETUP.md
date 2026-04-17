# SETUP – @Arc Unreal Plugin (Quick Start Using My Contracts)

This guide is for **getting the project running quickly** using **my deployed contracts and config** on @Arc testnet.

If you want a deeper architectural explanation or to use your own contracts from scratch, see [`DOC.md`](./DOC.md).

## Video walkthrough

I posted a short setup/demo video here (X):
- Video: https://x.com/<your_handle>/status/<tweet_id>

This video shows:
- Running the backend
- Setting `BackendBaseUrl` (WSL IP)
- Reading USDC balance in Unreal
- Sending pay/reward transactions + checking the explorer

---

## 1. Prerequisites

- **Unreal Engine 4.26+** (tested with UE 4.26 on Windows).
- **Node.js 18+** on your WSL/Linux environment.
- **Git** (optional, but recommended).
- An @Arc testnet account with:
  - Some gas for transactions.
  - Some **USDC** (on @Arc) for testing payments/rewards.

---

## 2. Contract Addresses (@Arc testnet)

Using the deployed instance used in development:

- **USDC (@Arc testnet)**  
  `0x3600000000000000000000000000000000000000`  
  Decimals: `6`

- **GameCore**  
  `0x263D7a26eCbF80f09eD1D3efc745FE9D9802b47c`

- **GameNFT**  
  `0xce784fF7C418bAB2B92ed76C27ac5AE3B2687216`

If you deployed your own contracts via `blockchain/` (Hardhat), you’ll have these in your deployment logs or JSON artifacts. Copy those addresses here.

---

## 3. Backend Setup (WSL / Linux)

The backend lives in `backend/` and exposes:

- `POST /registerPlayer`
- `POST /payWithUSDC`
- `POST /rewardUSDC`
- `POST /mintNFT`
- `GET /health`

### 3.1. Install dependencies

```bash
cd ~/arc-unreal/backend
npm install
```

### 3.2. Configure `.env`

Create `.env` in `backend/`:

```env
ARC_RPC_URL=https://rpc.testnet.arc.network

# Server wallet that will own GameCore and call rewardUSDC, mintNFT, etc.
PRIVATE_KEY=0xYOUR_SERVER_WALLET_PRIVATE_KEY

# Deployed contract addresses
GAME_CORE_ADDRESS=0x263D7a26eCbF80f09eD1D3efc745FE9D9802b47c
GAME_NFT_ADDRESS=0xce784fF7C418bAB2B92ed76C27ac5AE3B2687216

PORT=4000
```

Notes:

- `PRIVATE_KEY` must be the owner of `GameCore` (or at least an authorized caller for `rewardUSDC`).
- This key must hold gas for transactions and enough USDC if `rewardUSDC` pulls from the owner.

### 3.3. Start backend on WSL

In WSL:

```bash
cd ~/arc-unreal/backend
node server.js
# Expect: ARC backend listening on port 4000
```

Ensure the server is listening on all interfaces. In `server.js` the last lines should be:

```js
const PORT = process.env.PORT || 4000;
app.listen(PORT, "0.0.0.0", () => {
  console.log(`ARC backend listening on port ${PORT}`);
});
```

---

## 4. Make Backend Reachable From Windows

Unreal runs on Windows; backend runs in WSL. Unreal must hit the WSL IP, not Windows localhost.

### 4.1. Get WSL IP

In WSL:

```bash
hostname -I
```

Example output:

```text
172.25.160.1
```

### 4.2. Test from Windows

In **PowerShell**:

```powershell
curl http://172.25.160.1:4000/health
```

Expected:

```json
{"ok":true}
```

If this doesn’t work, double‑check:

- Backend is running.
- `app.listen` binds to `"0.0.0.0"`.
- No firewall is blocking the connection.

---

## 5. Plugin Setup in Unreal

### 5.1. Add plugin to your Unreal project

Copy the plugin folder:

```text
<YourUnrealProject>/
└─ Plugins/
   └─ ArcBlockchain/
      ├─ Source/
      ├─ ArcBlockchain.uplugin
      └─ ...
```

If `Plugins/` doesn’t exist, create it in the project root (the same folder as `.uproject`).

Right‑click your `.uproject` → **Generate Visual Studio project files**, then open the solution and build (or let Unreal prompt you to rebuild the plugin on launch).

### 5.2. Enable the plugin

- Start Unreal.
- Go to **Edit → Plugins**.
- Find `ArcBlockchain`.
- Enable it and restart the editor if prompted.

### 5.3. Configure @Arc settings in project

In Unreal:

1. Open **Edit → Project Settings**.
2. Look for **ARC Blockchain** (under Plugins or similar).
3. Configure:
   - **RPC URL**: `https://rpc.testnet.arc.network`
   - **GameCore Address**: `0x263D7a26eCbF80f09eD1D3efc745FE9D9802b47c`
   - **USDC Decimals**: `6` (if exposed; otherwise coded as 6 in the plugin).

Save settings.

---

## 6. Blueprint Wiring (Minimal Test)

Use a simple test map / Level Blueprint.

### 6.1. Read USDC balance

In Level Blueprint:

1. From `Event BeginPlay`, add node:
   - `Get USDC Balance Async` (Category: `ARC|Blockchain`).
2. Set `PlayerAddress` to your test @Arc address.
3. From `OnSuccess`:
   - Add `Print String`.
   - Wire the `BalanceUSDC` output → `ToString` → `Print String.InString`.
4. Compile, Play.

You should see your real USDC balance printed (e.g. `46.321`).

### 6.2. Set backend base URL

For all backend nodes, set:

- `BackendBaseUrl = "http://<YOUR_WSL_IP>:4000"`

Example:

- `BackendBaseUrl = "http://172.25.160.1:4000"`

### 6.3. Register player

Bind an input key (e.g. `R`):

1. Add `Keyboard → R` (Event R).
2. From `Event R`, add `Backend Register Player`.
3. Inputs:
   - `BackendBaseUrl = "http://<YOUR_WSL_IP>:4000"`
   - `PlayerAddress = <Your @Arc address>`
4. From the node:
   - Use `Success` + `TxHashOrError` to `Print String`.

Play → press `R`:

- WSL console: `POST /registerPlayer`.
- Unreal Output Log: either `Backend tx ok: 0x...` or a 500 with error.

---

## NOTE: Required USDC approvals (run once)

For `BackendPayWithUSDC` and `BackendRewardUSDC` to work, USDC approvals must be set for `GameCore` using two scripts:

- `approveFromPlayer`  
  Approves `GameCore` to spend USDC from the **player wallet** (required for `payWithUSDC`).

- `approveFromOwner`  
  Approves `GameCore` to spend USDC from the **backend/owner wallet** (required for `rewardUSDC`).

### Run the scripts

From the `blockchain/` folder (or wherever your scripts are located), run:

```bash
node approveFromPlayer.mjs
node approveFromOwner.mjs
```

> These approvals are typically needed only once per wallet (unless you change allowance or reset it).

### Important notes

- Make sure the scripts use the correct addresses:
  - USDC token address (@Arc testnet)
  - `GAME_CORE_ADDRESS`
- Make sure your `.env` contains the correct private keys:
  - `PLAYER_PRIVATE_KEY` for `approveFromPlayer`
  - `PRIVATE_KEY` (backend/owner) for `approveFromOwner`
- USDC uses **6 decimals**:
  - `5 USDC = 5,000,000` (raw units)

---

### 6.4. Pay with USDC & reward

Bind more keys:

- `P` → `Backend Pay With USDC`
  - `FromAddress = <Your player address>`
  - `Amount = 5000000` (for 5 USDC, with 6 decimals)
  - `Reason = "Test purchase"`

- `T` → `Backend Reward USDC`
  - `PlayerAddress = <Your player address>`
  - `Amount = 5000000` (reward 5 USDC, raw units)
  - `Reason = "Test reward"`

> Amount is in raw USDC units (6 decimals). Example: `5 USDC = 5,000,000`.

After calling these, re‑run `Get USDC Balance Async` and watch the balance change.

### 6.5. Mint NFT

Bind `M`:

1. `M` → `Backend Mint NFT`.
2. Inputs:
   - `BackendBaseUrl = "http://<YOUR_WSL_IP>:4000"`
   - `ToAddress = <Your player address>`
   - `TokenURI = "ipfs://your-nft-metadata-uri"`
3. Observe the tx hash in logs / explorer.

---

## Troubleshooting

### Unreal can’t reach the backend (timeouts)
- Confirm backend is running in WSL:
  - `curl http://localhost:4000/health`
- Confirm Windows can reach WSL:
  - `curl http://<YOUR_WSL_IP>:4000/health`
- Confirm backend listens on all interfaces:
  - `app.listen(PORT, "0.0.0.0", ...)`

For deeper details (approvals, contract design, extending the plugin), see [`DOC.md`](./DOC.md).