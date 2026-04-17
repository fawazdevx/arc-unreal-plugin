#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ArcBlockchainSettings.generated.h"

/**
 * Configurable settings for ARC blockchain integration.
 */
UCLASS(Config=Game, DefaultConfig)
class ARCBLOCKCHAIN_API UArcBlockchainSettings : public UObject
{
    GENERATED_BODY()

public:

    // ARC testnet RPC URL
    UPROPERTY(EditAnywhere, Config, Category="ARC")
    FString RpcUrl = TEXT("https://rpc.testnet.arc.network");

    // Deployed GameCore contract address
    UPROPERTY(EditAnywhere, Config, Category="ARC")
    FString GameCoreAddress;

    // Deployed GameNFT contract address
    UPROPERTY(EditAnywhere, Config, Category="ARC")
    FString GameNftAddress;

    // Optional chain ID
    UPROPERTY(EditAnywhere, Config, Category="ARC")
    int32 ChainId = 0;
};