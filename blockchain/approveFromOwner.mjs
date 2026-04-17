import { JsonRpcProvider, Wallet, Contract, parseUnits } from "ethers";
import * as dotenv from "dotenv";

dotenv.config();

async function main() {
  const usdcAddress = "0x3600000000000000000000000000000000000000";
  const gameCoreAddress = "0x263D7a26eCbF80f09eD1D3efc745FE9D9802b47c";
  const rpcUrl = "https://rpc.testnet.arc.network";

  const ownerPk = process.env.DEPLOYER_PRIVATE_KEY;
  if (!ownerPk) {
    throw new Error("DEPLOYER_PRIVATE_KEY not set in .env (backend/owner PK)");
  }

  const provider = new JsonRpcProvider(rpcUrl);
  const owner = new Wallet(ownerPk, provider);
  console.log("Owner signer:", owner.address);

  // Minimal ERC20 ABI
  const erc20Abi = [
    "function approve(address spender, uint256 amount) external returns (bool)"
  ];
  const usdc = new Contract(usdcAddress, erc20Abi, owner);

  const allowance = parseUnits("1000000", 6); // 1,000,000 USDC
  console.log(`Approving GameCore (${gameCoreAddress}) to spend ${allowance.toString()} units from ${owner.address}`);

  const tx = await usdc.approve(gameCoreAddress, allowance);
  console.log("tx hash:", tx.hash);
  await tx.wait();
  console.log("Approved.");
}

main().catch((e) => {
  console.error(e);
  process.exitCode = 1;
});
