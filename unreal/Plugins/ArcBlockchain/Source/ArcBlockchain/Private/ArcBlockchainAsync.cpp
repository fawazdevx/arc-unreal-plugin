#include "ArcBlockchainAsync.h"
#include "ArcBlockchainSettings.h"
#include "ArcHttpClient.h"

#include "Engine/World.h"
#include "Engine/Engine.h"


static FString BuildGetUSDCBalanceData_Internal(const FString& PlayerAddress)
{
    
    const FString FunctionSelector = TEXT("0x68024717");

    FString CleanAddress = PlayerAddress;
    CleanAddress.RemoveFromStart(TEXT("0x"));
    CleanAddress = CleanAddress.ToLower();

    while (CleanAddress.Len() < 64)
    {
        CleanAddress = TEXT("0") + CleanAddress;
    }

    return FunctionSelector + CleanAddress;
}

UGetUSDCBalanceAsync* UGetUSDCBalanceAsync::GetUSDCBalanceAsync(
    UObject* WorldContextObject,
    const FString& PlayerAddress)
{
    UGetUSDCBalanceAsync* Node = NewObject<UGetUSDCBalanceAsync>();
    Node->PlayerAddressInternal = PlayerAddress;
    return Node;
}

void UGetUSDCBalanceAsync::Activate()
{
    const UArcBlockchainSettings* Settings = GetDefault<UArcBlockchainSettings>();
    if (!Settings || Settings->RpcUrl.IsEmpty() || Settings->GameCoreAddress.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ArcBlockchain: GetUSDCBalanceAsync settings invalid"));
        OnFailure.Broadcast();
        return;
    }

    FString ToAddress = Settings->GameCoreAddress;
    ToAddress.RemoveFromStart(TEXT("0x"));

    const FString Data = BuildGetUSDCBalanceData_Internal(PlayerAddressInternal);

    TSharedPtr<FJsonObject> CallObj = MakeShared<FJsonObject>();
    CallObj->SetStringField(TEXT("to"), FString::Printf(TEXT("0x%s"), *ToAddress));
    CallObj->SetStringField(TEXT("data"), Data);

    TArray<TSharedPtr<FJsonValue>> Params;
    Params.Add(MakeShared<FJsonValueObject>(CallObj));
    Params.Add(MakeShared<FJsonValueString>(TEXT("latest")));

    const FString RpcUrl = Settings->RpcUrl;

    // Fire request
    FArcHttpClient::JsonRpcCall(RpcUrl, TEXT("eth_call"), Params,
        [this](const TSharedPtr<FJsonObject>& Result, const FString& Error)
        {
            if (!Error.IsEmpty() || !Result.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("ArcBlockchain: GetUSDCBalanceAsync error: %s"), *Error);
                // Schedule failure broadcast back on game thread
                AsyncTask(ENamedThreads::GameThread, [this]()
                {
                    OnFailure.Broadcast();
                });
                return;
            }

            FString HexValue;
            if (!Result->TryGetStringField(TEXT("value"), HexValue))
            {
                if (const TSharedPtr<FJsonValue>* v = Result->Values.Find(TEXT("value")))
                {
                    if ((*v)->Type == EJson::String)
                    {
                        HexValue = (*v)->AsString();
                    }
                }
            }

            if (HexValue.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("ArcBlockchain: GetUSDCBalanceAsync result has no value"));
                AsyncTask(ENamedThreads::GameThread, [this]()
                {
                    OnFailure.Broadcast();
                });
                return;
            }

            HexValue.RemoveFromStart(TEXT("0x"));
            const int64 BalanceRaw = (int64)FCString::Strtoui64(*HexValue, nullptr, 16);

            // Convert using 6 decimals for USDC
            const float BalanceUSDC = (float)BalanceRaw / 1000000.0f;

            UE_LOG(LogTemp, Log, TEXT("ArcBlockchain: GetUSDCBalanceAsync -> %lld (%.6f USDC)"),
                   BalanceRaw, BalanceUSDC);

            // Broadcast result on game thread
            AsyncTask(ENamedThreads::GameThread, [this, BalanceRaw, BalanceUSDC]()
            {
                OnSuccess.Broadcast(BalanceRaw, BalanceUSDC);
            });
        }
    );
}