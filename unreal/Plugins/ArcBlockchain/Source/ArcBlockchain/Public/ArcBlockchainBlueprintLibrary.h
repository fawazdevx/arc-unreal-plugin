#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArcBlockchainBlueprintLibrary.generated.h"

/**
 * Blueprint function library for ARC blockchain helpers.
 *
 * - GetUSDCBalance: direct JSON-RPC eth_call to GameCore contract.
 * - Backend* methods: HTTP calls to your Node.js signer backend.
 */
UCLASS()
class ARCBLOCKCHAIN_API UArcBlockchainBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /**
     * Get USDC balance of a player by address (hex string).
     * Calls GameCore.getUSDCBalance(address) via JSON-RPC eth_call.
     *
     * @param PlayerAddress  Hex address string, e.g. "0x1234..."
     * @param bSuccess       True if the call succeeded.
     * @param Balance        Raw USDC token units (e.g. 1 USDC = 1_000_000 if 6 decimals).
     */
    UFUNCTION(BlueprintCallable, Category="ARC|Blockchain", meta=(WorldContext="WorldContextObject"))
    static void GetUSDCBalance(
        UObject* WorldContextObject,
        const FString& PlayerAddress,
        bool& bSuccess,
        int64& Balance);

    /**
     * Backend: register a player on-chain by calling your Node backend,
     * which signs and sends the GameCore.registerPlayer transaction.
     *
     * POST {BackendBaseUrl}/registerPlayer { "player": "0x..." }
     */
    UFUNCTION(BlueprintCallable, Category="ARC|Backend", meta=(WorldContext="WorldContextObject"))
    static void BackendRegisterPlayer(
        UObject* WorldContextObject,
        const FString& BackendBaseUrl,
        const FString& PlayerAddress,
        bool& bSuccess,
        FString& TxHashOrError);

    /**
     * Backend: call GameCore.payWithUSDC from the backend signer.
     *
     * POST {BackendBaseUrl}/payWithUSDC { "from": "0x...", "amount": <uint>, "reason": "..." }
     */
    UFUNCTION(BlueprintCallable, Category="ARC|Backend", meta=(WorldContext="WorldContextObject"))
    static void BackendPayWithUSDC(
        UObject* WorldContextObject,
        const FString& BackendBaseUrl,
        const FString& FromAddress,
        int64 Amount,
        const FString& Reason,
        bool& bSuccess,
        FString& TxHashOrError);

    /**
     * Backend: call GameCore.rewardUSDC from the backend signer (owner-only).
     *
     * POST {BackendBaseUrl}/rewardUSDC { "player": "0x...", "amount": <uint>, "reason": "..." }
     */
    UFUNCTION(BlueprintCallable, Category="ARC|Backend", meta=(WorldContext="WorldContextObject"))
    static void BackendRewardUSDC(
        UObject* WorldContextObject,
        const FString& BackendBaseUrl,
        const FString& PlayerAddress,
        int64 Amount,
        const FString& Reason,
        bool& bSuccess,
        FString& TxHashOrError);

    /**
     * Backend: mint an NFT via GameNFT.mintTo from the backend signer (owner-only).
     *
     * POST {BackendBaseUrl}/mintNFT { "to": "0x...", "tokenURI": "ipfs://..." }
     */
    UFUNCTION(BlueprintCallable, Category="ARC|Backend", meta=(WorldContext="WorldContextObject"))
    static void BackendMintNFT(
        UObject* WorldContextObject,
        const FString& BackendBaseUrl,
        const FString& ToAddress,
        const FString& TokenURI,
        bool& bSuccess,
        FString& TxHashOrError);
};