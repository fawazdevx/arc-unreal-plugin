// scripts/deploy.js
import { ethers } from "ethers";
import * as dotenv from "dotenv";
import hre from "hardhat";

dotenv.config();

async function main() {
  const { ARC_RPC_URL, ARC_USDC_ADDRESS, DEPLOYER_PRIVATE_KEY } = process.env;

  if (!ARC_RPC_URL) {
    throw new Error("ARC_RPC_URL not set in .env");
  }
  if (!ARC_USDC_ADDRESS) {
    throw new Error("ARC_USDC_ADDRESS not set in .env");
  }
  if (!DEPLOYER_PRIVATE_KEY) {
    throw new Error("DEPLOYER_PRIVATE_KEY not set in .env");
  }

  console.log("Deploying GameCore with USDC:", ARC_USDC_ADDRESS);
  console.log("Using RPC URL:", ARC_RPC_URL);

  // Provider & signer from env
  const provider = new ethers.JsonRpcProvider(ARC_RPC_URL);
  const deployer = new ethers.Wallet(DEPLOYER_PRIVATE_KEY, provider);
  const deployerAddress = await deployer.getAddress();
  console.log("Deployer address:", deployerAddress);

  // Use Hardhat artifacts
  const gameCoreArtifact = await hre.artifacts.readArtifact("GameCore");
  const GameCoreFactory = new ethers.ContractFactory(
    gameCoreArtifact.abi,
    gameCoreArtifact.bytecode,
    deployer
  );
  const gameCore = await GameCoreFactory.deploy(ARC_USDC_ADDRESS);
  await gameCore.waitForDeployment();
  console.log("GameCore deployed to:", await gameCore.getAddress());

  const gameNFTArtifact = await hre.artifacts.readArtifact("GameNFT");
  const GameNFTFactory = new ethers.ContractFactory(
    gameNFTArtifact.abi,
    gameNFTArtifact.bytecode,
    deployer
  );
  const gameNFT = await GameNFTFactory.deploy(deployerAddress);
  await gameNFT.waitForDeployment();
  console.log("GameNFT deployed to:", await gameNFT.getAddress());

  console.log(
    "Remember to set GameNFT owner if needed (current owner is deployer)."
  );
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});