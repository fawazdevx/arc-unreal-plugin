require("dotenv").config();
const express = require("express");
const cors = require("cors");
const { ethers } = require("ethers");
const fs = require("fs");
const path = require("path");

const app = express();
app.use(cors());
app.use(express.json());

const {
  ARC_RPC_URL,
  PRIVATE_KEY,
  GAME_CORE_ADDRESS,
  GAME_NFT_ADDRESS,
} = process.env;

if (!ARC_RPC_URL || !PRIVATE_KEY || !GAME_CORE_ADDRESS || !GAME_NFT_ADDRESS) {
  console.error("Missing env vars, check .env");
  process.exit(1);
}

const provider = new ethers.JsonRpcProvider(ARC_RPC_URL);
const wallet = new ethers.Wallet(PRIVATE_KEY, provider);

// Load ABIs from your Hardhat artifacts
const gameCoreJson = JSON.parse(
  fs.readFileSync(
    path.join(__dirname, "../blockchain/artifacts/contracts/GameCore.sol/GameCore.json"),
    "utf8"
  )
);
const gameNftJson = JSON.parse(
  fs.readFileSync(
    path.join(__dirname, "../blockchain/artifacts/contracts/GameNFT.sol/GameNFT.json"),
    "utf8"
  )
);

const gameCore = new ethers.Contract(GAME_CORE_ADDRESS, gameCoreJson.abi, wallet);
const gameNft = new ethers.Contract(GAME_NFT_ADDRESS, gameNftJson.abi, wallet);

// ---------- Helper ----------
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

// ---------- Routes ----------

// Register player on-chain (just marks struct + event)
app.post("/registerPlayer", async (req, res) => {
  const { player } = req.body;
  if (!player) return res.status(400).json({ ok: false, error: "Missing player" });

  return handleTx(res, gameCore.registerPlayer(player));
});

// Player paying with USDC (requires player approved GameCore)
app.post("/payWithUSDC", async (req, res) => {
  const { from, amount, reason } = req.body;
  if (!from || !amount) {
    return res.status(400).json({ ok: false, error: "Missing from or amount" });
  }

  // NOTE: This function assumes the caller (wallet) is the one paying.
  // For a real game you'd probably pay from server wallet or do something else.
  return handleTx(res, gameCore.payWithUSDC(amount, reason || ""));
});

// Reward USDC to player (owner-only)
app.post("/rewardUSDC", async (req, res) => {
  const { player, amount, reason } = req.body;
  if (!player || !amount) {
    return res.status(400).json({ ok: false, error: "Missing player or amount" });
  }

  return handleTx(res, gameCore.rewardUSDC(player, amount, reason || ""));
});

// Mint NFT to player
app.post("/mintNFT", async (req, res) => {
  const { to, tokenURI } = req.body;
  if (!to || !tokenURI) {
    return res.status(400).json({ ok: false, error: "Missing to or tokenURI" });
  }

  return handleTx(res, gameNft.mintTo(to, tokenURI));
});

// Simple health check
app.get("/health", (req, res) => res.json({ ok: true }));

const PORT = process.env.PORT || 4000;
app.listen(PORT, () => {
  console.log(`ARC backend listening on port ${PORT}`);
});