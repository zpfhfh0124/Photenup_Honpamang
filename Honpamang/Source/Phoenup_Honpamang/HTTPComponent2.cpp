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
// 서버 체크
void UHTTPComponent2::CheckServer()
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    Request->SetURL(BaseURL + "/");
    Request->SetVerb("GET");

    Request->OnProcessRequestComplete().BindUObject(this, &UHTTPComponent2::OnResponseReceived);
    Request->ProcessRequest();
}

//////////////////////////////////////////////////////
// 파일 경로 업로드
void UHTTPComponent2::UploadImage(const FString& FilePath)
{
    TArray<uint8> FileData;

    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("파일 로드 실패: %s"), *FilePath);
        return;
    }

    UploadImageBytes(FileData);
}

//////////////////////////////////////////////////////
// 🔥 바이트 업로드 (핵심)
void UHTTPComponent2::UploadImageBytes(const TArray<uint8>& ImageBytes)
{
    FString Boundary = "----UEBoundary123456789";

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    Request->SetURL(BaseURL + "/upload_image");
    Request->SetVerb("POST");

    FString ContentType = "multipart/form-data; boundary=" + Boundary;
    Request->SetHeader(TEXT("Content-Type"), ContentType);

    TArray<uint8> Body;

    FString HeaderPart = "--" + Boundary + "\r\n";
    HeaderPart += "Content-Disposition: form-data; name=\"file\"; filename=\"capture.jpg\"\r\n";
    HeaderPart += "Content-Type: image/jpeg\r\n\r\n";

    FString FooterPart = "\r\n--" + Boundary + "--\r\n";

    // Header
    Body.Append((uint8*)TCHAR_TO_UTF8(*HeaderPart), HeaderPart.Len());

    // 🔥 이미지 바이트
    Body.Append(ImageBytes);

    // Footer
    Body.Append((uint8*)TCHAR_TO_UTF8(*FooterPart), FooterPart.Len());

    Request->SetContent(Body);

    Request->OnProcessRequestComplete().BindUObject(this, &UHTTPComponent2::OnResponseReceived);
    Request->ProcessRequest();
}

//////////////////////////////////////////////////////
// 응답 처리
void UHTTPComponent2::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("HTTP 요청 실패"));
        return;
    }

    int32 StatusCode = Response->GetResponseCode();

    FString ResponseString;
    FFileHelper::BufferToString(
        ResponseString,
        Response->GetContent().GetData(),
        Response->GetContentLength()
    );

    LastResponse = ResponseString;

    UE_LOG(LogTemp, Warning, TEXT("Status Code: %d"), StatusCode);
    UE_LOG(LogTemp, Warning, TEXT("Response: %s"), *ResponseString);

    // 블루프린트로 전달
    OnHttpResponse.Broadcast(ResponseString);
}

