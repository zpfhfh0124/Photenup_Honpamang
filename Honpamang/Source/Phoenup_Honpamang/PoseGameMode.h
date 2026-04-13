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

	// 🔥 카운트 끝나면 호출 (WBP에서)
	UFUNCTION(BlueprintCallable)
	void StartPoseRound();

protected:
	virtual void BeginPlay() override;

private:

	// 컴포넌트 참조
	UPROPERTY()
	UWebcamCapture* Webcam;

	UPROPERTY()
	UHTTPComponent2* HTTP;

	// 현재 포즈 인덱스
	int32 CurrentPoseIndex;

	// 타이머
	FTimerHandle PoseTimerHandle;

	// 내부 흐름 함수
	void OnPoseTimeEnd();

	UFUNCTION()
	void OnCaptured(const TArray<uint8>& ImageBytes);

	UFUNCTION()
	void OnAIResponse(const FString& Response);

	// 설정
	int32 TotalPoseCount = 11;
	float PoseDuration = 5.0f;
};
