// hardhat.config.ts
import { defineConfig } from "hardhat/config";
import hardhatToolboxMochaEthers from "@nomicfoundation/hardhat-toolbox-mocha-ethers";
import * as dotenv from "dotenv";

dotenv.config();

const { ARC_RPC_URL, DEPLOYER_PRIVATE_KEY } = process.env;

export default defineConfig({
  plugins: [hardhatToolboxMochaEthers],

  solidity: "0.8.28",

  networks: {
    hardhat: {
      type: "edr-simulated",
    },
    arcTestnet: {
      type: "http",
      url: ARC_RPC_URL || "",
      accounts: DEPLOYER_PRIVATE_KEY ? [DEPLOYER_PRIVATE_KEY] : [],
    },
  },
});