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
void UHTTPComponent2::UploadImage(const FString& FilePath, int32 PoseIndex)
{
    TArray<uint8> FileData;

    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("파일 로드 실패: %s"), *FilePath);
        return;
    }

    UploadImageWithIndex(FileData, PoseIndex);
}

//////////////////////////////////////////////////////
// 🔥 바이트 업로드 (핵심)
void UHTTPComponent2::UploadImageWithIndex(const TArray<uint8>& ImageBytes, int32 PoseIndex)
{
    FString Boundary = "----UEBoundary123456789";

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    // AI 예측 경로로 설정
    Request->SetURL(BaseURL + "/predict"); 
    Request->SetVerb("POST");

    FString ContentType = "multipart/form-data; boundary=" + Boundary;
    Request->SetHeader(TEXT("Content-Type"), ContentType);

    TArray<uint8> Body;

    // --- Part 1: Pose Index (텍스트 데이터) ---
    FString IndexPart = "--" + Boundary + "\r\n";
    IndexPart += "Content-Disposition: form-data; name=\"pose_index\"\r\n\r\n";
    IndexPart += FString::FromInt(PoseIndex) + "\r\n";
    Body.Append((uint8*)TCHAR_TO_UTF8(*IndexPart), IndexPart.Len());

    // --- Part 2: Image File (바이너리 데이터) ---
    FString HeaderPart = "--" + Boundary + "\r\n";
    HeaderPart += "Content-Disposition: form-data; name=\"file\"; filename=\"capture.jpg\"\r\n";
    HeaderPart += "Content-Type: image/jpeg\r\n\r\n";
    Body.Append((uint8*)TCHAR_TO_UTF8(*HeaderPart), HeaderPart.Len());

    // 실제 이미지 바이트 추가
    Body.Append(ImageBytes);

    // --- Part 3: Footer (마무리) ---
    FString FooterPart = "\r\n--" + Boundary + "--\r\n";
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

    FString ResponseString = Response->GetContentAsString();
    
    LastResponse = ResponseString;

    UE_LOG(LogTemp, Warning, TEXT("Status Code: %d"), StatusCode);
    UE_LOG(LogTemp, Warning, TEXT("Response: %s"), *ResponseString);

    // 블루프린트로 전달
    OnHttpResponse.Broadcast(ResponseString);
}

