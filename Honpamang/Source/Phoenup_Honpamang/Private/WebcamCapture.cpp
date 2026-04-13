// Fill out your copyright notice in the Description page of Project Settings.


#include "WebcamCapture.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Engine/TextureRenderTarget2D.h"

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
	
	FString url = DeviceURL;
	if (url.IsEmpty())
	{
#if PLATFORM_WINDOWS
		url = TEXT("vidcap://0");
#elif PLATFORM_MAC
		url = TEXT("avcapture://0");
#elif PLATFORM_LINUX
		url = TEXT("v4l2:///dev/video0");
#endif
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
		UE_LOG(LogTemp, Warning, TEXT("WebcamCapture Busy, 리퀘스트 무시"));
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
		UE_LOG(LogTemp, Log, TEXT("WebcamCapture Capturing in %.1fs"), CaptureDelay);
	}
}

void UWebcamCapture::CaptureNow()
{
	State = ECaptureState::Capturing;
	
	TArray<uint8> jpegBytes;
	if (CaptureToJpeg(jpegBytes))
	{
		LastJpegBytes = MoveTemp(jpegBytes);
		UE_LOG(LogTemp, Log, TEXT("WebcamCapture Captured: %d bytes"), LastJpegBytes.Num());
		OnFrameCaptured.Broadcast(LastJpegBytes);
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
		UE_LOG(LogTemp, Log, TEXT("WebcamCapture Cancelled"));
	}
}

//** Frame -> JPEG
bool UWebcamCapture::CaptureToJpeg(TArray<uint8>& OutBytes)
{
	if (!RenderTarget) return false;
	
	FRenderTarget* rt = RenderTarget->GameThread_GetRenderTargetResource();
	if (!rt) return false;
	
	// 픽셀 읽기
	const int32 w = RenderTarget->SizeX;
	const int32 h = RenderTarget->SizeY;
	TArray<FColor> pixels;
	pixels.SetNum(w * h);
	
	if (!rt->ReadPixels(pixels))
	{
		UE_LOG(LogTemp, Error, TEXT("WebcamCapture ReadPixels failed"));
		return false;
	}
	
	// RGBA 바이트로 변환
	TArray<uint8> raw;
	raw.SetNum(w * h);
	for (int32 i = 0; i <pixels.Num(); i++)
	{
		raw[i * 4 + 0] = pixels[i].R;
		raw[i * 4 + 1] = pixels[i].B;
		raw[i * 4 + 2] = pixels[i].G;
		raw[i * 4 + 3] = pixels[i].A;
	}
	
	// JPEG 인코딩
	IImageWrapperModule& imgMod = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> wrapper = imgMod.CreateImageWrapper(EImageFormat::JPEG);
	
	if (!wrapper->SetRaw(raw.GetData(), raw.Num(), w, h, ERGBFormat::RGBA, 8))
	{
		UE_LOG(LogTemp, Error, TEXT("WebcamCapture SetRaw failed"));
		return false;
	}
	
	const TArray64<uint8> compressed = wrapper->GetCompressed(JpegQuality);
	if (compressed.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("WebcamCapture JPEG compression failed"));
		return false;
	}
	
	OutBytes.SetNum(compressed.Num());
	FMemory::Memcpy(OutBytes.GetData(), compressed.GetData(), compressed.Num());
	return true;
}
