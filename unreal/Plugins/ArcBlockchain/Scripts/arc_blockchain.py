import os
import json
from dataclasses import dataclass
from typing import Optional, Tuple

from web3 import Web3
from web3.middleware import geth_poa_middleware  # if ARC is POA-like


@dataclass
class ArcConfig:
    rpc_url: str
    game_core_address: str
    game_nft_address: str
    chain_id: int = 0  # set to actual ARC testnet chain ID if needed

class ArcBlockchainClient:
    def __init__(self, config: ArcConfig, private_key: Optional[str] = None):
        self.config = config
        self.web3 = Web3(Web3.HTTPProvider(config.rpc_url))

        # If ARC is POA, like many testnets:
        self.web3.middleware_onion.inject(geth_poa_middleware, layer=0)

        if not self.web3.is_connected():
            raise RuntimeError("Cannot connect to ARC RPC at {}".format(config.rpc_url))

        self.account = None
        self.private_key = None
        if private_key:
            self.private_key = private_key
            self.account = self.web3.eth.account.from_key(private_key)

        # Load ABI files compiled by Hardhat (you must copy them into plugin or point to them)
        # Here we assume you copied the JSONs from blockchain/artifacts/contracts into this folder.
        base_dir = os.path.dirname(os.path.abspath(__file__))
        with open(os.path.join(base_dir, "GameCore.json"), "r") as f:
            game_core_abi = json.load(f)["abi"]
        with open(os.path.join(base_dir, "GameNFT.json"), "r") as f:
            game_nft_abi = json.load(f)["abi"]

        self.game_core = self.web3.eth.contract(
            address=self.web3.to_checksum_address(config.game_core_address),
            abi=game_core_abi,
        )
        self.game_nft = self.web3.eth.contract(
            address=self.web3.to_checksum_address(config.game_nft_address),
            abi=game_nft_abi,
        )

    # ---------- READ METHODS (NO SIGNING) ----------

    def get_usdc_balance(self, player_address: str) -> int:
        # Delegates to the GameCore view method
        checksum = self.web3.to_checksum_address(player_address)
        return self.game_core.functions.getUSDCBalance(checksum).call()

    def get_player_profile(self, player_address: str) -> Tuple[int, int, int, bool]:
        checksum = self.web3.to_checksum_address(player_address)
        return self.game_core.functions.getPlayerProfile(checksum).call()

    # ---------- WRITE METHODS (SIGNED TX) ----------

    def _build_and_send_tx(self, tx) -> str:
        if not self.account or not self.private_key:
            raise RuntimeError("No private key/account set for signing transactions")

        nonce = self.web3.eth.get_transaction_count(self.account.address)
        tx_dict = tx.build_transaction({
            "from": self.account.address,
            "nonce": nonce,
            # Gas params: might require tuning for ARC
            "gas": 800000,
            "gasPrice": self.web3.eth.gas_price,
            # "chainId": self.config.chain_id or self.web3.eth.chain_id,
        })

        signed = self.web3.eth.account.sign_transaction(tx_dict, private_key=self.private_key)
        tx_hash = self.web3.eth.send_raw_transaction(signed.rawTransaction)
        return self.web3.to_hex(tx_hash)

    def register_player(self, player_address: str) -> str:
        checksum = self.web3.to_checksum_address(player_address)
        tx = self.game_core.functions.registerPlayer(checksum)
        return self._build_and_send_tx(tx)

    def pay_with_usdc(self, amount: int, reason: str) -> str:
        tx = self.game_core.functions.payWithUSDC(amount, reason)
        return self._build_and_send_tx(tx)

    def reward_usdc(self, player_address: str, amount: int, reason: str) -> str:
        checksum = self.web3.to_checksum_address(player_address)
        tx = self.game_core.functions.rewardUSDC(checksum, amount, reason)
        return self._build_and_send_tx(tx)

    def mint_nft(self, to_address: str, token_uri: str) -> str:
        checksum = self.web3.to_checksum_address(to_address)
        tx = self.game_nft.functions.mintTo(checksum, token_uri)
        return self._build_and_send_tx(tx)

# Simple singleton for UE Python usage
_arc_client: Optional[ArcBlockchainClient] = None

def init_arc_client(
    rpc_url: str,
    game_core_address: str,
    game_nft_address: str,
    private_key: Optional[str] = None,
    chain_id: int = 0,
):
    global _arc_client
    cfg = ArcConfig(
        rpc_url=rpc_url,
        game_core_address=game_core_address,
        game_nft_address=game_nft_address,
        chain_id=chain_id,
    )
    _arc_client = ArcBlockchainClient(cfg, private_key=private_key)
    return True

def get_client() -> ArcBlockchainClient:
    if _arc_client is None:
        raise RuntimeError("ArcBlockchainClient not initialized. Call init_arc_client first.")
    return _arc_client

# Convenience wrapper functions for UE (easier to bind to buttons)

def ue_get_usdc_balance(player_address: str) -> int:
    return get_client().get_usdc_balance(player_address)

def ue_get_player_profile(player_address: str):
    spent, earned, last_updated, exists = get_client().get_player_profile(player_address)
    return {
        "totalSpentUSDC": spent,
        "totalEarnedUSDC": earned,
        "lastUpdated": last_updated,
        "exists": exists,
    }

def ue_register_player(player_address: str) -> str:
    return get_client().register_player(player_address)

def ue_pay_with_usdc(amount: int, reason: str) -> str:
    return get_client().pay_with_usdc(amount, reason)

def ue_reward_usdc(player_address: str, amount: int, reason: str) -> str:
    return get_client().reward_usdc(player_address, amount, reason)

def ue_mint_nft(to_address: str, token_uri: str) -> str:
    return get_client().mint_nft(to_address, token_uri)