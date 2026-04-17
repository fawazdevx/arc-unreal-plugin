#include "ArcHttpClient.h"
#include "Json.h"
#include "JsonUtilities.h"

void FArcHttpClient::JsonRpcCall(
    const FString& RpcUrl,
    const FString& Method,
    const TArray<TSharedPtr<FJsonValue>>& Params,
    TFunction<void(const TSharedPtr<FJsonObject>& Result, const FString& Error)> Callback)
{
    if (RpcUrl.IsEmpty())
    {
        Callback(nullptr, TEXT("RPC URL is empty"));
        return;
    }

    // Thread-safe shared ref – must match CreateRequest's return type
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(RpcUrl);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // Build JSON-RPC payload
    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
    Payload->SetStringField(TEXT("method"), Method);
    Payload->SetNumberField(TEXT("id"), 1);
    Payload->SetArrayField(TEXT("params"), Params);

    FString PayloadStr;
    {
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadStr);
        FJsonSerializer::Serialize(Payload, Writer);
    }

    Request->SetContentAsString(PayloadStr);

    Request->OnProcessRequestComplete().BindLambda(
        [Callback](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            if (!bWasSuccessful || !Response.IsValid())
            {
                Callback(nullptr, TEXT("HTTP request failed"));
                return;
            }

            const FString ResponseStr = Response->GetContentAsString();

            TSharedPtr<FJsonObject> JsonResponse;
            {
                const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
                if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse.IsValid())
                {
                    Callback(nullptr, TEXT("Failed to parse JSON response"));
                    return;
                }
            }

            // JSON-RPC error?
            if (JsonResponse->HasField(TEXT("error")))
            {
                const TSharedPtr<FJsonObject> ErrorObj = JsonResponse->GetObjectField(TEXT("error"));
                const FString Message = ErrorObj->GetStringField(TEXT("message"));
                Callback(nullptr, Message);
                return;
            }

            if (!JsonResponse->HasField(TEXT("result")))
            {
                Callback(nullptr, TEXT("No result field in JSON-RPC response"));
                return;
            }

            // Wrap the result value under "value" for convenience
            const TSharedPtr<FJsonValue> ResultValue = JsonResponse->TryGetField(TEXT("result"));

            TSharedRef<FJsonObject> Wrapper = MakeShared<FJsonObject>();
            Wrapper->SetField(TEXT("value"), ResultValue);

            Callback(Wrapper, TEXT(""));
        }
    );

    Request->ProcessRequest();
}