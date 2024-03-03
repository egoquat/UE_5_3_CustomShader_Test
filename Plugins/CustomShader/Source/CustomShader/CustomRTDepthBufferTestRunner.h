#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CustomRTDepthBufferTestHwrt.h"
#include "CustomRTDepthBufferTestRunner.generated.h"
 
UCLASS()
class CUSTOMSHADER_API ACustomRTDepthBufferTestRunner : public AActor
{
    GENERATED_BODY()
	
public:	
    ACustomRTDepthBufferTestRunner();
    FCustomRTDepthBufferTestHwrt TestCustomRTDepthBufferTest;
 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShaderDemo)
    UTextureRenderTarget2D* RenderTexture = nullptr;
	
protected:
    virtual void BeginPlay() override;
    void UpdateTestParameters();
 
    float TranscurredTime; 
    bool Initialized;
 
public:	
    virtual void Tick(float DeltaTime) override;
    virtual void BeginDestroy() override;

    void EndRendering();

    void Init();
    static void StaticToggleEditorSimulateMode(bool bSimulateOnEditor); 
};
