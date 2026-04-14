#include "PoseGameMode.h"
#include "WebcamCapture.h"
#include "HTTPComponent2.h"
#include "Kismet/GameplayStatics.h"

APoseGameMode::APoseGameMode()
{
}

void APoseGameMode::BeginPlay()
{
    Super::BeginPlay();

    // 🔥 액터에서 컴포넌트 찾기 (간단 버전)
    AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(this, 0);

    if (PlayerActor)
    {
        Webcam = PlayerActor->FindComponentByClass<UWebcamCapture>();
        HTTP = PlayerActor->FindComponentByClass<UHTTPComponent2>();

        if (Webcam)
        {
            Webcam->OnFrameCaptured.AddDynamic(this, &APoseGameMode::OnCaptured);
        }

        if (HTTP)
        {
            HTTP->OnHttpResponse.AddDynamic(this, &APoseGameMode::OnAIResponse);
        }
    }
}

//////////////////////////////////////////////////////
// 🔥 WBP에서 호출
void APoseGameMode::StartPoseRound()
{
    // 1️⃣ 랜덤 포즈 선택
    CurrentPoseIndex = FMath::RandRange(0, TotalPoseCount - 1);

    UE_LOG(LogTemp, Warning, TEXT("Selected Pose Index: %d"), CurrentPoseIndex);

    // 👉 TODO: UI에 이미지 변경 (블루프린트에서 처리 추천)

    // 2️⃣ 5초 타이머 시작
    GetWorld()->GetTimerManager().SetTimer(
        PoseTimerHandle,
        this,
        &APoseGameMode::OnPoseTimeEnd,
        PoseDuration,
        false
    );
}

//////////////////////////////////////////////////////
// 5초 끝
void APoseGameMode::OnPoseTimeEnd()
{
    UE_LOG(LogTemp, Warning, TEXT("Pose Time End → Capture"));

    if (Webcam)
    {
        Webcam->CaptureNow();
    }
}

//////////////////////////////////////////////////////
// 캡쳐 완료
void APoseGameMode::OnCaptured(const TArray<uint8>& ImageBytes)
{
    UE_LOG(LogTemp, Warning, TEXT("Image Captured → Send to AI"));

    if (HTTP)
    {
        HTTP->UploadImageBytes(ImageBytes);
    }
}

//////////////////////////////////////////////////////
// AI 응답
void APoseGameMode::OnAIResponse(const FString& Response)
{
    UE_LOG(LogTemp, Warning, TEXT("AI Response: %s"), *Response);

    // 👉 여기서 점수 처리
    // 예: Contains("85") or JSON 파싱

    // TODO: 플레이어 점수 저장
}