#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CustomRTRayHwrt.h"
#include "CustomRTRayRunner.generated.h"
 
UCLASS()
class CUSTOMSHADER_API ACustomRTRayRunner : public AActor
{
    GENERATED_BODY()
	
public:	
    ACustomRTRayRunner();
    FCustomRTRayHwrt CustomRTRay;
 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomRTRay)
    class UTextureRenderTarget2D* RenderTexture = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomRTRay)
    float FOVView = FConstCustomRay::FOV;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomRTRay)
    float FarClipView = FConstCustomRay::FarClipView;

    void Initialize(AActor* actorView);
protected:
    void UpdateTestParameters();
 
    float TranscurredTime; ///< allows us to add a delay on BeginPlay() 
    bool Initialized;
    AActor* ActorView = nullptr;
 
public:	
    virtual void Tick(float DeltaTime) override;
    virtual void TickRayPickTest(float DeltaTime);
    virtual void BeginDestroy() override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

    void EndRendering();

    static void StaticToggleEditorSimulateMode(bool bSimulateOnEditor); 
};
