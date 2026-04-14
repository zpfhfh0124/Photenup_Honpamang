#include "PoseGameMode.h"
#include "WebcamCapture.h"
#include "HTTPComponent2.h"
#include "Kismet/GameplayStatics.h"

APoseGameMode::APoseGameMode() { }

void APoseGameMode::BeginPlay()
{
    Super::BeginPlay();

    // 캐릭터에서 컴포넌트 참조 가져오기
    AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(this, 0);
    if (PlayerActor)
    {
        Webcam = PlayerActor->FindComponentByClass<UWebcamCapture>();
        HTTP = PlayerActor->FindComponentByClass<UHTTPComponent2>();

        if (Webcam) Webcam->OnFrameCaptured.AddDynamic(this, &APoseGameMode::OnCaptured);
        if (HTTP) HTTP->OnHttpResponse.AddDynamic(this, &APoseGameMode::OnAIResponse);
    }
}

// 1. 인원 설정 (WBP에서 버튼 클릭 시 호출) [cite: 30]
void APoseGameMode::SetTotalPlayers(int32 Count)
{
    TotalPlayers = FMath::Clamp(Count, 1, 4);
    CurrentTurnIndex = 0;
    PlayerScores.Init(0.0f, TotalPlayers); // 배열 초기화 [cite: 44]
}

// 2. 라운드 시작 (카운트다운 3초 뒤 실행) 
void APoseGameMode::StartPoseRound()
{
    CurrentPoseIndex = FMath::RandRange(0, TotalPoseCount - 1);
    
    // UI에 현재 누가 어떤 포즈를 해야 하는지 알림 
    OnUpdateUI(CurrentTurnIndex, CurrentPoseIndex);

    // 5초 타이머 작동 
    GetWorld()->GetTimerManager().SetTimer(PoseTimerHandle, this, &APoseGameMode::OnPoseTimeEnd, PoseDuration, false);
}

// 5초 종료 -> 캡처 
void APoseGameMode::OnPoseTimeEnd()
{
    if (Webcam) Webcam->CaptureNow();
}

// 캡처 완료 -> AI 전송
void APoseGameMode::OnCaptured(const TArray<uint8>& ImageBytes)
{
    // 🔥 수정됨: 이미지 바이트와 저장해둔 인덱스를 함께 보냅니다.
    HTTP->UploadImageWithIndex(ImageBytes, CurrentPoseIndex);
        
    UE_LOG(LogTemp, Warning, TEXT("AI 서버로 포즈 %d번 전송 시도"), CurrentPoseIndex);
}

// AI 응답 처리 및 턴 교체 로직 [cite: 77, 83, 84, 85]
void APoseGameMode::OnAIResponse(const FString& Response)
{
    // 점수 저장 (예시: Response 문자열을 float로 변환)
    float Score = FCString::Atof(*Response);
    PlayerScores[CurrentTurnIndex] = Score;

    // 모든 플레이어가 끝났는지 확인 [cite: 83]
    if (CurrentTurnIndex + 1 < TotalPlayers)
    {
        // 다음 플레이어로 인덱스 증가 [cite: 84]
        CurrentTurnIndex++;
        OnRoundFinished(false); // UI에 "다음 사람 나오세요" 출력 유도 [cite: 81]
    }
    else
    {
        // 모든 인원 종료 -> 결과 화면으로 [cite: 85]
        OnRoundFinished(true);
    }
}