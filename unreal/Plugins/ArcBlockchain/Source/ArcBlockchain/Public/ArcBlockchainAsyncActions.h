#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ArcBlockchainAsyncActions.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FArcTxCompleted, bool, bSuccess, FString, TxHashOrError);


UCLASS()
class ARCBLOCKCHAIN_API UArcSendRawTransactionAsync : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FArcTxCompleted OnCompleted;

    UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"))
    static UArcSendRawTransactionAsync* SendRawTransaction(UObject* WorldContextObject, const FString& SignedRawTransactionHex);

    virtual void Activate() override;

private:
    FString RawTxHex;
};