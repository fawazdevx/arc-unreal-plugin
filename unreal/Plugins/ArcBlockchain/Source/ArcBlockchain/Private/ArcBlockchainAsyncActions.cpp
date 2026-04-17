#include "ArcBlockchainAsyncActions.h"
#include "ArcBlockchainSettings.h"
#include "ArcHttpClient.h"

UArcSendRawTransactionAsync* UArcSendRawTransactionAsync::SendRawTransaction(UObject* WorldContextObject, const FString& SignedRawTransactionHex)
{
    UArcSendRawTransactionAsync* Action = NewObject<UArcSendRawTransactionAsync>();
    Action->RawTxHex = SignedRawTransactionHex;
    return Action;
}

void UArcSendRawTransactionAsync::Activate()
{
    const UArcBlockchainSettings* Settings = GetDefault<UArcBlockchainSettings>();
    if (!Settings || Settings->RpcUrl.IsEmpty())
    {
        OnCompleted.Broadcast(false, TEXT("Invalid settings or RPC URL"));
        return;
    }

    if (RawTxHex.IsEmpty())
    {
        OnCompleted.Broadcast(false, TEXT("Raw transaction hex is empty"));
        return;
    }

    TArray<TSharedPtr<FJsonValue>> Params;
    Params.Add(MakeShared<FJsonValueString>(RawTxHex));

    FArcHttpClient::JsonRpcCall(Settings->RpcUrl, TEXT("eth_sendRawTransaction"), Params,
        [this](const TSharedPtr<FJsonObject>& Result, const FString& Error)
        {
            if (!Error.IsEmpty() || !Result.IsValid())
            {
                OnCompleted.Broadcast(false, Error.IsEmpty() ? TEXT("Unknown error") : Error);
                return;
            }

            FString TxHash;
            if (!Result->TryGetStringField(TEXT("value"), TxHash))
            {
                if (const TSharedPtr<FJsonValue>* v = Result->Values.Find(TEXT("value")))
                {
                    if ((*v)->Type == EJson::String)
                    {
                        TxHash = (*v)->AsString();
                    }
                }
            }

            if (TxHash.IsEmpty())
            {
                OnCompleted.Broadcast(false, TEXT("No tx hash in result"));
            }
            else
            {
                OnCompleted.Broadcast(true, TxHash);
            }
        }
    );
}
