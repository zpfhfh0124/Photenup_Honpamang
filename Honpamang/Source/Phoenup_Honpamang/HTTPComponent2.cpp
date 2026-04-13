#include "HTTPComponent2.h"
#include "Misc/FileHelper.h"

UHTTPComponent2::UHTTPComponent2()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHTTPComponent2::BeginPlay()
{
    Super::BeginPlay();
}

//////////////////////////////////////////////////////
// 서버 체크 (GET /)
void UHTTPComponent2::CheckServer()
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    Request->SetURL("http://172.16.30.124:8099/");
    Request->SetVerb("GET");

    Request->OnProcessRequestComplete().BindUObject(this, &UHTTPComponent2::OnResponseReceived);
    Request->ProcessRequest();
}

//////////////////////////////////////////////////////
// 이미지 업로드 (POST /upload_image)
void UHTTPComponent2::UploadImage(const FString& FilePath)
{
    TArray<uint8> FileData;

    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("파일 로드 실패: %s"), *FilePath);
        return;
    }

    FString Boundary = "----UE4Boundary7MA4YWxkTrZu0gW";

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    Request->SetURL("http://172.16.30.124:8099/upload_image");
    Request->SetVerb("POST");

    FString ContentType = "multipart/form-data; boundary=" + Boundary;
    Request->SetHeader(TEXT("Content-Type"), ContentType);

    TArray<uint8> Body;

    FString HeaderPart = "--" + Boundary + "\r\n";
    HeaderPart += "Content-Disposition: form-data; name=\"file\"; filename=\"image.png\"\r\n";
    HeaderPart += "Content-Type: application/octet-stream\r\n\r\n";

    FString FooterPart = "\r\n--" + Boundary + "--\r\n";

    // Header 추가
    Body.Append((uint8*)TCHAR_TO_UTF8(*HeaderPart), HeaderPart.Len());

    // 파일 데이터 추가
    Body.Append(FileData);

    // Footer 추가
    Body.Append((uint8*)TCHAR_TO_UTF8(*FooterPart), FooterPart.Len());

    Request->SetContent(Body);

    Request->OnProcessRequestComplete().BindUObject(this, &UHTTPComponent2::OnResponseReceived);
    Request->ProcessRequest();
}

//////////////////////////////////////////////////////
// 공통 응답 처리
void UHTTPComponent2::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("HTTP 요청 실패"));
        return;
    }

    int32 StatusCode = Response->GetResponseCode();
    FString ResponseString = Response->GetContentAsString();

    UE_LOG(LogTemp, Warning, TEXT("Status Code: %d"), StatusCode);
    UE_LOG(LogTemp, Warning, TEXT("Response: %s"), *ResponseString);
}

