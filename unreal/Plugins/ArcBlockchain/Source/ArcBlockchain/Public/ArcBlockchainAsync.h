#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ArcBlockchainAsync.generated.h"

// Delegate for success: returns raw units and human-readable USDC balance
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnGetUSDCBalanceAsyncResult,
    int64, BalanceRaw,
    float, BalanceUSDC
);

// Delegate for failure: no params, just a signal
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGetUSDCBalanceAsyncFailure);

/**
 * Async Blueprint node to get USDC balance via GameCore.getUSDCBalance(address).
 */
UCLASS()
class ARCBLOCKCHAIN_API UGetUSDCBalanceAsync : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    // Called when balance is retrieved successfully
    UPROPERTY(BlueprintAssignable)
    FOnGetUSDCBalanceAsyncResult OnSuccess;

    // Called when request fails
    UPROPERTY(BlueprintAssignable)
    FOnGetUSDCBalanceAsyncFailure OnFailure;

    // Factory function for Blueprint
    UFUNCTION(BlueprintCallable,
              meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"),
              Category="ARC|Blockchain")
    static UGetUSDCBalanceAsync* GetUSDCBalanceAsync(
        UObject* WorldContextObject,
        const FString& PlayerAddress);

    // UBlueprintAsyncActionBase override
    virtual void Activate() override;

private:
    FString PlayerAddressInternal;
};