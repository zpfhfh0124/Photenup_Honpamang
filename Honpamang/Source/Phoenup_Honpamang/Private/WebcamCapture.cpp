// Fill out your copyright notice in the Description page of Project Settings.


#include "WebcamCapture.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "IMediaCaptureSupport.h"
#include "MediaCaptureSupport.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Sets default values for this component's properties
UWebcamCapture::UWebcamCapture()
{
	PrimaryComponentTick.bCanEverTick = true;
}

//**Life Cycle
void UWebcamCapture::BeginPlay()
{
	Super::BeginPlay();
	InitMedia();
}

void UWebcamCapture::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseCamera();
	Super::EndPlay(EndPlayReason);
}

void UWebcamCapture::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (State == ECaptureState::WaitingDelay)
	{
		DelayRemaining -= DeltaTime;
		OnCountdown.Broadcast(FMath::Max(0.0f, DelayRemaining));
		
		if (DelayRemaining <= 0.0f)
		{
			CaptureNow();
		}
	}
}

//** 미디어 초기화
void UWebcamCapture::InitMedia()
{
	MediaPlayer = NewObject<UMediaPlayer>(this, TEXT("CamPlayer"));
	MediaPlayer->SetLooping(true);
	MediaPlayer->PlayOnOpen = true;
	
	MediaTexture = NewObject<UMediaTexture>(this, TEXT("CamTexture"));
	MediaTexture->SetMediaPlayer(MediaPlayer);
	MediaTexture->UpdateResource();
	
	RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("CamRT"));
	RenderTarget->InitAutoFormat(Resolution.X, Resolution.Y);
	RenderTarget->UpdateResource();
}

bool UWebcamCapture::IsCameraOpen() const
{
	return MediaPlayer && MediaPlayer->IsPlaying();
}

//** 웹캠 제어
bool UWebcamCapture::OpenCamera()
{
	if (!MediaPlayer)
	{
		OnCaptureError.Broadcast(TEXT("MediaPlayer가 초기화되지 않았다!"));
		return false;
	}
	
	// 연결된 웹캠 리스트 로드
	TArray<FMediaCaptureDeviceInfo> devices;
	MediaCaptureSupport::EnumerateVideoCaptureDevices(devices);
	
	if (devices.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[WebcamCapture] 웹캠을 찾지 못했다."));
		OnCaptureError.Broadcast(TEXT("웹캠을 찾지 못함."));
		return false;
	}
	
	// 찾은 디바이스 목록 로그 출력
	for (int32 i = 0; i < devices.Num(); i++)
	{
		UE_LOG(LogTemp, Log, TEXT("[WebcamCapture] Device %d: %s -> %s"), i, *devices[i].DisplayName.ToString(), *devices[i].Url);
	}
	
	// 디바이스 URL이 설정되었으면 사용, 아니면 첫 번째 디바이스 사용.
	FString url = DeviceURL;
	if (url.IsEmpty())
	{
		url = devices[0].Url;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[WebcamCapture] Opening: %s"), *url);
	return MediaPlayer->OpenUrl(url);
}

void UWebcamCapture::CloseCamera()
{
	if (MediaPlayer && MediaPlayer->IsPlaying())
	{
		MediaPlayer->Close();
	}
	State = ECaptureState::Idle;
}

//** 캡쳐 트리거
void UWebcamCapture::RequestCapture()
{
	if (!IsCameraOpen())
	{
		OnCaptureError.Broadcast(TEXT("카메라가 오픈되어 있지 않았다."));
		return;
	}
	
	if (State != ECaptureState::Idle)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WebcamCapture] Busy, 리퀘스트 무시"));
		return;
	}
	
	if (CaptureDelay <= 0.0f)
	{
		CaptureNow();
	}
	else
	{
		State = ECaptureState::WaitingDelay;
		DelayRemaining = CaptureDelay;
		UE_LOG(LogTemp, Log, TEXT("[WebcamCapture] Capturing in %.1fs"), CaptureDelay);
	}
}

void UWebcamCapture::CaptureNow()
{
	State = ECaptureState::Capturing;
	
	TArray<uint8> jpegBytes;
	if (CaptureToJpeg(jpegBytes))
	{
		LastJpegBytes = MoveTemp(jpegBytes);
		UE_LOG(LogTemp, Log, TEXT("[WebcamCapture] Captured: %d bytes"), LastJpegBytes.Num());
		OnFrameCaptured.Broadcast(LastJpegBytes);
		
		// 자동 저장
		if (bAutoSave) SaveCaptureToFile();
	}
	else
	{
		OnCaptureError.Broadcast(TEXT("Frame capture failed"));
	}
	
	State = ECaptureState::Idle;
}

void UWebcamCapture::CancelCapture()
{
	if (State == ECaptureState::WaitingDelay)
	{
		State = ECaptureState::Idle;
		DelayRemaining = 0.0f;
		UE_LOG(LogTemp, Log, TEXT("[WebcamCapture] Cancelled"));
	}
}

//** Frame -> JPEG
bool UWebcamCapture::CaptureToJpeg(TArray<uint8>& OutBytes)
{
	if (!MediaPlayer || !RenderTarget) return false;
	
	// 웹캠 화면을 RT에 그리기
	UCanvas* canvas;
	FVector2D canvasSize;
	FDrawToRenderTargetContext context;
	
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, canvas, canvasSize, context);
	
	canvas->K2_DrawTexture(
		MediaTexture,
		FVector2D::ZeroVector,
		canvasSize,
		FVector2D::ZeroVector,
		FVector2D::UnitVector);
	
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, context);
	
	// ReadPixels
	FRenderTarget* rt = RenderTarget->GameThread_GetRenderTargetResource();
	if (!rt) return false;
	
	TArray<FColor> pixels;
	if (!rt->ReadPixels(pixels))
	{
		UE_LOG(LogTemp, Error, TEXT("[WebcamCapture] ReadPixels failed"));
		return false;
	}
	
	const int32 w = RenderTarget->SizeX;
	const int32 h = RenderTarget->SizeY;
	
	if (pixels.Num() != w * h)
	{
		UE_LOG(LogTemp, Error, TEXT("[WebcamCapture] Pixel count mismatch: %d vs %d"), pixels.Num(), w * h);
		return false;
	}
	
	// JPEG 인코딩 - FColor 배열을 직접 전달
	IImageWrapperModule& imgMod = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
	TSharedPtr<IImageWrapper> wrapper = imgMod.CreateImageWrapper(EImageFormat::JPEG);
	
	if (!wrapper->SetRaw(
		pixels.GetData(),
		pixels.Num() * sizeof(FColor),
		w, h,
		ERGBFormat::BGRA,
		8))
	{
		return false;
	}
	
	const TArray64<uint8>& compressed = wrapper->GetCompressed(JpegQuality);
	if (compressed.Num() == 0) return false;
	
	OutBytes.SetNum(compressed.Num());
	FMemory::Memcpy(OutBytes.GetData(), compressed.GetData(), compressed.Num());
	return true;
}

FString UWebcamCapture::SaveCaptureToFile()
{
	if (LastJpegBytes.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WebcamCapture] No image to save"));
		return FString();
	}
	
	//  저장 폴더 결정
	FString dir = SaveDirectoryPath;
	if (dir.IsEmpty()) dir = FPaths::ProjectSavedDir() / TEXT("Captures");
	
	// 폴더 생성
	IFileManager::Get().MakeDirectory(*dir, true);
	
	// 파일명: 날짜_시간.jpg
	const FString timeStemp = FDateTime::UtcNow().ToString(TEXT("%Y-%m-%d_%H-%M-%S"));
	const FString filePath = dir / (timeStemp + TEXT(".jpg"));
	
	// 저장 
	if (FFileHelper::SaveArrayToFile(LastJpegBytes, *filePath))
	{
		LastSavedFilePath = filePath;
		UE_LOG(LogTemp, Log, TEXT("[WebcamCapture] Saved: %s"), *filePath);
		return filePath;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[WebcamCapture] Could not save file: %s"), *filePath);
		return FString();
	}
}
