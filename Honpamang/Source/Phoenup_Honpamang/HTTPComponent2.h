#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Http.h"
#include "HTTPComponent2.generated.h"

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
    UFUNCTION(BlueprintCallable)
    void CheckServer();

    // 이미지 업로드
    UFUNCTION(BlueprintCallable)
    void UploadImage(const FString& FilePath);

private:

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
