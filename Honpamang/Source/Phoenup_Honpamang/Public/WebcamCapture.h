// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MediaTexture.h"
#include "Components/ActorComponent.h"
#include "WebcamCapture.generated.h"

UENUM(BlueprintType)
enum class ECaptureState : uint8
{
	Idle,
	WaitingDelay,
	Capturing,
};

// 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFrameCaptured, const TArray<uint8>&, JpegBytes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCountdown, float, RemainingSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCaptureError, const FString&, Error);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), DisplayName="Webcam Capture" )
class PHOENUP_HONPAMANG_API UWebcamCapture : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWebcamCapture();

	//** 설정
	// 웹캠 디바이스 URL (비워두면 기본 카메라)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Webcam")
	FString DeviceURL;
	
	// 캡쳐 해상도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Webcam")
	FIntPoint Resolution = FIntPoint(1280, 720);
	
	//JPEG 압축 품질 (1~100)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Webcam", meta = (ClampMin = "1", ClampMax="100"))
	int32 JpegQuality = 85;
	
	// 트리거 후 캡쳐까지 딜레이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Webcam", meta = (ClampMin = "0.0", ClampMax="10.0"))
	float CaptureDelay = 2.0f;
	
	// 캡쳐 시 로컬 자동 저장
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Webcam")
	bool bAutoSave = true;
	
	// 저장 폴더 경로 (비워두면 Saved/Captures/)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Webcam")
	FString SaveDirectoryPath;
	
	//** 이벤트
	// 프레임 캡쳐 완료 (JPEG 바이트 전달)
	UPROPERTY(BlueprintAssignable, Category = "Webcam|Events")
	FOnFrameCaptured OnFrameCaptured;
	
	// 딜레이 카운트다운 (매 프레임) 
	UPROPERTY(BlueprintAssignable, Category = "Webcam|Events")
	FOnCountdown OnCountdown;
	
	// 에러
	UPROPERTY(BlueprintAssignable, Category = "Webcam|Events")
	FOnCaptureError OnCaptureError;
	
	//** 웹캠 제어
	// 웹캠 열기
	UFUNCTION(BlueprintCallable, Category = "Webcam")
	bool OpenCamera();
	
	// 웹캠 닫기
	UFUNCTION(BlueprintCallable, Category = "Webcam")
	void CloseCamera();
	
	// 웹캠 오픈 체크
	UFUNCTION(BlueprintPure, Category = "Webcam")
	bool IsCameraOpen() const;
	
	//**캡쳐 제어
	// 딜레이 후 캡쳐 (메인 트리거) 
	UFUNCTION(BlueprintCallable, Category = "Webcam")
	void RequestCapture();
	
	// 즉시 캡쳐 (No Delay)
	UFUNCTION(BlueprintCallable, Category = "Webcam")
	void CaptureNow();
	
	// 딜레이 대기 취소
	UFUNCTION(BlueprintCallable, Category = "Webcam")
	void CancelCapture();
	
	// 현재 상태
	UFUNCTION(BlueprintPure, Category = "Webcam")
	ECaptureState GetState() const { return State; }
	
	//**결과 접근
	// 마지막 캡쳐된 JPEG 바이트 (HTTP 전송 등에 사용)
	UFUNCTION(BlueprintPure, Category = "Webcam")
	const TArray<uint8>& GetLastCapturedImage() const { return LastJpegBytes; }
	
	// 마지막 캡쳐 이미지 크기 (바이트)
	UFUNCTION(BlueprintPure, Category = "Webcam")
	int32 GetCaptureDelay() const { return LastJpegBytes.Num(); }
	
	// 웹캠 미리보기 텍스쳐 (UMG Image 위젯에 연결 가능)
	UFUNCTION(BlueprintPure, Category = "Webcam")
	UMediaTexture* GetPreviewTexture() const { return MediaTexture; }
	
	// JPEG 바이트를 로컬 파일로 저장, 저장된 경로 변환
	UFUNCTION(BlueprintCallable, Category = "Webcam")
	FString SaveCaptureToFile();
	
	// 최종 저장 파일 경로
	UFUNCTION(BlueprintPure, Category = "Webcam")
	FString GetLastSavedPath() const { return LastSavedFilePath; }
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	ECaptureState State = ECaptureState::Idle;
	float DelayRemaining = 0.0f;
	
	UPROPERTY()
	UMediaPlayer* MediaPlayer = nullptr;
	
	UPROPERTY()
	UMediaTexture* MediaTexture = nullptr;
		
	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget = nullptr;
	
	TArray<uint8> LastJpegBytes;
	
	FString LastSavedFilePath;
	
	void InitMedia();
	bool CaptureToJpeg(TArray<uint8>& OutBytes);
};
