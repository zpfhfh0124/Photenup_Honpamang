#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Http.h"
#include "HTTPComponent2.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHttpResponse, const FString&, Response);

UCLASS(ClassGroup=(HTTP), meta=(BlueprintSpawnableComponent))
class PHOENUP_HONPAMANG_API UHTTPComponent2 : public UActorComponent
{
    GENERATED_BODY()

public:
    UHTTPComponent2();

protected:
    virtual void BeginPlay() override;

public:

    // 서버 상태 체크 (GET /)
    UFUNCTION(BlueprintCallable, Category="HTTP")
    void CheckServer();

    // 파일 경로로 업로드
    UFUNCTION(BlueprintCallable, Category="HTTP")
    void UploadImage(const FString& FilePath, int32 PoseIndex);

    // 🔥 바이트로 업로드 (핵심)
    UFUNCTION(BlueprintCallable, Category="HTTP")
    void UploadImageWithIndex(const TArray<uint8>& ImageBytes, int32 PoseIndex);
    // 마지막 응답
    UPROPERTY(BlueprintReadOnly, Category="HTTP")
    FString LastResponse;

    // 블루프린트 이벤트
    UPROPERTY(BlueprintAssignable, Category="HTTP")
    FOnHttpResponse OnHttpResponse;

private:

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    FString BaseURL = "http://172.16.30.124:8099";
};
