#pragma once

#include "CoreMinimal.h"
#include "Http.h"

class FArcHttpClient
{
public:
    static void JsonRpcCall(
        const FString& RpcUrl,
        const FString& Method,
        const TArray<TSharedPtr<FJsonValue>>& Params,
        TFunction<void(const TSharedPtr<FJsonObject>& Result, const FString& Error)> Callback);
};