#include "ArcBlockchainBlueprintLibrary.h"
#include "ArcBlockchainSettings.h"
#include "ArcHttpClient.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"

// ---------------------------------------------------------
// Helper: build ABI-encoded data for GameCore.getUSDCBalance(address)
// ---------------------------------------------------------

static FString BuildGetUSDCBalanceData(const FString& PlayerAddress)
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



void UArcBlockchainBlueprintLibrary::GetUSDCBalance(
    UObject* WorldContextObject,
    const FString& PlayerAddress,
    bool& bSuccess,
    int64& Balance)
{
    bSuccess = false;
    Balance = 0;

    const UArcBlockchainSettings* Settings = GetDefault<UArcBlockchainSettings>();
    if (!Settings || Settings->RpcUrl.IsEmpty() || Settings->GameCoreAddress.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ArcBlockchain: Settings invalid or missing RPC/GameCoreAddress"));
        return;
    }

    FString ToAddress = Settings->GameCoreAddress;
    ToAddress.RemoveFromStart(TEXT("0x"));

    const FString Data = BuildGetUSDCBalanceData(PlayerAddress);

    // Build eth_call params
    TSharedPtr<FJsonObject> CallObj = MakeShared<FJsonObject>();
    CallObj->SetStringField(TEXT("to"), FString::Printf(TEXT("0x%s"), *ToAddress));
    CallObj->SetStringField(TEXT("data"), Data);

    TArray<TSharedPtr<FJsonValue>> Params;
    Params.Add(MakeShared<FJsonValueObject>(CallObj));
    Params.Add(MakeShared<FJsonValueString>(TEXT("latest")));

    
    FArcHttpClient::JsonRpcCall(Settings->RpcUrl, TEXT("eth_call"), Params,
        [](const TSharedPtr<FJsonObject>& Result, const FString& Error)
        {
            if (!Error.IsEmpty() || !Result.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("ArcBlockchain: GetUSDCBalance error: %s"), *Error);
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
                UE_LOG(LogTemp, Warning, TEXT("ArcBlockchain: GetUSDCBalance result has no value field"));
                return;
            }

            HexValue.RemoveFromStart(TEXT("0x"));
            int64 LocalBalance = (int64)FCString::Strtoui64(*HexValue, nullptr, 16);

            UE_LOG(LogTemp, Log, TEXT("ArcBlockchain: GetUSDCBalance -> %lld"), LocalBalance);
        }
    );

    
    bSuccess = true;
}

// ---------------------------------------------------------
// Backend helper
// ---------------------------------------------------------

static void FireBackendPostRequest(
    const FString& Url,
    const TSharedPtr<FJsonObject>& JsonBody)
{
    if (Url.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ArcBlockchain: Backend URL is empty"));
        return;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    FString BodyStr;
    {
        TSharedRef<FJsonObject> JsonRef = JsonBody.ToSharedRef();
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
        FJsonSerializer::Serialize(JsonRef, Writer);
    }
    Request->SetContentAsString(BodyStr);

    Request->OnProcessRequestComplete().BindLambda(
        [](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            if (!bWasSuccessful || !Response.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("ArcBlockchain: Backend HTTP request failed"));
                return;
            }

            const int32 Code = Response->GetResponseCode();
            const FString Content = Response->GetContentAsString();

            if (Code != 200)
            {
                UE_LOG(LogTemp, Warning, TEXT("ArcBlockchain: Backend HTTP %d: %s"), Code, *Content);
                return;
            }

            TSharedPtr<FJsonObject> Json;
            const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
            if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("ArcBlockchain: Failed to parse backend JSON"));
                return;
            }

            const bool bOk = Json->GetBoolField(TEXT("ok"));
            FString TxHash;
            Json->TryGetStringField(TEXT("txHash"), TxHash);

            if (!bOk)
            {
                FString Err;
                Json->TryGetStringField(TEXT("error"), Err);
                UE_LOG(LogTemp, Warning, TEXT("ArcBlockchain: Backend error: %s"), *Err);
                return;
            }

            UE_LOG(LogTemp, Log, TEXT("ArcBlockchain: Backend tx ok: %s"), *TxHash);
        }
    );

    Request->ProcessRequest();
}

// ---------------------------------------------------------
// Backend* wrappers 
// ---------------------------------------------------------

void UArcBlockchainBlueprintLibrary::BackendRegisterPlayer(
    UObject* WorldContextObject,
    const FString& BackendBaseUrl,
    const FString& PlayerAddress,
    bool& bSuccess,
    FString& TxHashOrError)
{
    bSuccess = false;
    TxHashOrError.Empty();

    if (BackendBaseUrl.IsEmpty())
    {
        TxHashOrError = TEXT("Backend URL empty");
        return;
    }

    const FString Url = BackendBaseUrl.EndsWith(TEXT("/"))
        ? BackendBaseUrl + TEXT("registerPlayer")
        : BackendBaseUrl + TEXT("/registerPlayer");

    TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("player"), PlayerAddress);

    FireBackendPostRequest(Url, Body);

    bSuccess = true;
    TxHashOrError = TEXT("Request sent (check Output Log / backend logs)");
}

void UArcBlockchainBlueprintLibrary::BackendPayWithUSDC(
    UObject* WorldContextObject,
    const FString& BackendBaseUrl,
    const FString& FromAddress,
    int64 Amount,
    const FString& Reason,
    bool& bSuccess,
    FString& TxHashOrError)
{
    bSuccess = false;
    TxHashOrError.Empty();

    if (BackendBaseUrl.IsEmpty())
    {
        TxHashOrError = TEXT("Backend URL empty");
        return;
    }

    const FString Url = BackendBaseUrl.EndsWith(TEXT("/"))
        ? BackendBaseUrl + TEXT("payWithUSDC")
        : BackendBaseUrl + TEXT("/payWithUSDC");

    TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("from"), FromAddress);
    Body->SetNumberField(TEXT("amount"), static_cast<double>(Amount));
    Body->SetStringField(TEXT("reason"), Reason);

    FireBackendPostRequest(Url, Body);

    bSuccess = true;
    TxHashOrError = TEXT("Request sent (check Output Log / backend logs)");
}

void UArcBlockchainBlueprintLibrary::BackendRewardUSDC(
    UObject* WorldContextObject,
    const FString& BackendBaseUrl,
    const FString& PlayerAddress,
    int64 Amount,
    const FString& Reason,
    bool& bSuccess,
    FString& TxHashOrError)
{
    bSuccess = false;
    TxHashOrError.Empty();

    if (BackendBaseUrl.IsEmpty())
    {
        TxHashOrError = TEXT("Backend URL empty");
        return;
    }

    const FString Url = BackendBaseUrl.EndsWith(TEXT("/"))
        ? BackendBaseUrl + TEXT("rewardUSDC")
        : BackendBaseUrl + TEXT("/rewardUSDC");

    TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("player"), PlayerAddress);
    Body->SetNumberField(TEXT("amount"), static_cast<double>(Amount));
    Body->SetStringField(TEXT("reason"), Reason);

    FireBackendPostRequest(Url, Body);

    bSuccess = true;
    TxHashOrError = TEXT("Request sent (check Output Log / backend logs)");
}

void UArcBlockchainBlueprintLibrary::BackendMintNFT(
    UObject* WorldContextObject,
    const FString& BackendBaseUrl,
    const FString& ToAddress,
    const FString& TokenURI,
    bool& bSuccess,
    FString& TxHashOrError)
{
    bSuccess = false;
    TxHashOrError.Empty();

    if (BackendBaseUrl.IsEmpty())
    {
        TxHashOrError = TEXT("Backend URL empty");
        return;
    }

    const FString Url = BackendBaseUrl.EndsWith(TEXT("/"))
        ? BackendBaseUrl + TEXT("mintNFT")
        : BackendBaseUrl + TEXT("/mintNFT");

    TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("to"), ToAddress);
    Body->SetStringField(TEXT("tokenURI"), TokenURI);

    FireBackendPostRequest(Url, Body);

    bSuccess = true;
    TxHashOrError = TEXT("Request sent (check Output Log / backend logs)");
}