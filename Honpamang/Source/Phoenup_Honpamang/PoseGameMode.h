#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PoseGameMode.generated.h"

class UWebcamCapture;
class UHTTPComponent2;

UCLASS()
class PHOENUP_HONPAMANG_API APoseGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APoseGameMode();

	// 1. 게임 시작 전 인원 설정 (WBP의 SCENE 02에서 호출)
	UFUNCTION(BlueprintCallable, Category = "PoseGame")
	void SetTotalPlayers(int32 Count);

	// 2. 카운트다운 완료 후 라운드 시작 (WBP의 SCENE 06에서 호출)
	UFUNCTION(BlueprintCallable, Category = "PoseGame")
	void StartPoseRound();

	// 블루프린트에서 UI 갱신을 위해 사용할 이벤트들SD
	UFUNCTION(BlueprintImplementableEvent, Category = "PoseGame")
	void OnUpdateUI(int32 PlayerIndex, int32 PoseIndex); // 현재 플레이어와 포즈 안내 

	UFUNCTION(BlueprintImplementableEvent, Category = "PoseGame")
	void OnRoundFinished(bool bIsAllFinished); // 라운드 종료 및 다음 사람 대기 [cite: 81, 85]

protected:
	virtual void BeginPlay() override;

private:
	// 컴포넌트 참조 (Private이지만 블루프린트 접근 허용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PoseGame", meta = (AllowPrivateAccess = "true"))
	UWebcamCapture* Webcam;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PoseGame", meta = (AllowPrivateAccess = "true"))
	UHTTPComponent2* HTTP;

	// 게임 상태 변수 [cite: 30, 45]
	int32 TotalPlayers = 1;
	int32 CurrentTurnIndex = 0;
	int32 TotalPoseCount = 11;
	float PoseDuration = 5.0f;

	UPROPERTY()
	TArray<float> PlayerScores; // 플레이어별 점수 저장 [cite: 44]

	FTimerHandle PoseTimerHandle;

	void OnPoseTimeEnd();

	UFUNCTION()
	void OnCaptured(const TArray<uint8>& ImageBytes);

	UFUNCTION()
	void OnAIResponse(const FString& Response);
};