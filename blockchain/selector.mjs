
import { keccak256, toUtf8Bytes } from "ethers";

function selector(sig) {
  const hash = keccak256(toUtf8Bytes(sig));
  const sel = hash.slice(0, 10); // 0x + first 4 bytes
  console.log(`${sig} ->`);
  console.log(`  keccak256: ${hash}`);
  console.log(`  selector:  ${sel}`);
  console.log("");
}

selector("getUSDCBalance(address)");
selector("balanceOf(address)");
