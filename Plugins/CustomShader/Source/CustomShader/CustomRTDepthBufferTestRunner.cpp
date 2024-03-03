#include "CustomRTDepthBufferTestRunner.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#include "Engine/World.h"
#include "SceneInterface.h"
#include "../Private/ScenePrivate.h"
#include "Engine/Texture2DDynamic.h"
#include "CustomShaderUtility.h"

static int GUseCustomRTDepthBufferTestTest = 1;
FAutoConsoleVariableRef CUseCustomRTDepthBufferTestTest(
TEXT("Keti.Sensor.UseCustomRTDepthBufferTestTest"),
GUseCustomRTDepthBufferTestTest,
TEXT("default 1\n"),
ECVF_Cheat
);

static int GReadCustomRTDepthBufferTestTest = 0;
FAutoConsoleVariableRef CReadCustomRTDepthBufferTestTest(
TEXT("Keti.Sensor.ReadCustomRTDepthBufferTestTest"),
GReadCustomRTDepthBufferTestTest,
TEXT("default 0\n"),
ECVF_Cheat
);
 
ACustomRTDepthBufferTestRunner::ACustomRTDepthBufferTestRunner()
{
    USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));
    RootComponent = SceneComponent;

    PrimaryActorTick.bCanEverTick = true;
}

void ACustomRTDepthBufferTestRunner::BeginPlay()
{
    Super::BeginPlay();
    TestCustomRTDepthBufferTest = FCustomRTDepthBufferTestHwrt();
    Initialized = false;
 
    if (RenderTexture != nullptr)
        UpdateTestParameters();

#if WITH_EDITOR
    if (GetWorld()->IsPlayInEditor())
    {
        FEditorDelegates::OnPreSwitchBeginPIEAndSIE.AddStatic(&ACustomRTDepthBufferTestRunner::StaticToggleEditorSimulateMode);
    }
#endif
}
 
void ACustomRTDepthBufferTestRunner::UpdateTestParameters()
{
    FCustomRTDepthBufferTestRHIParameters parameters;
    parameters.Scene = &GetWorld()->Scene->GetRenderScene()->RayTracingScene;
    parameters.CachedRenderTargetSize = FIntPoint(RenderTexture->SizeX, RenderTexture->SizeY);
    parameters.RenderTexture = RenderTexture;
    TestCustomRTDepthBufferTest.UpdateParameters(parameters);
}

void ACustomRTDepthBufferTestRunner::Tick(float DeltaTime)
{
    if (GUseCustomRTDepthBufferTestTest != 1)
    {
        EndRendering();
        return;
    }

    UWorld* world = GetWorld();
    
    FRayTracingScene* rtScene = &world->Scene->GetRenderScene()->RayTracingScene;
    bool isInitializedRT = rtScene->IsCreated();
    if (isInitializedRT == false)
    {
        return;
    }
    
    TranscurredTime+=DeltaTime;
    Super::Tick(DeltaTime);
    
    if(RenderTexture != nullptr && TranscurredTime>1.0f)
    {
        UpdateTestParameters();

        if(!Initialized)
        {
            ENQUEUE_RENDER_COMMAND(TestCustomRTDepthBufferTestBeginRendering)(
            [this, world, &TestCustomRTDepthBufferTest = TestCustomRTDepthBufferTest](FRHICommandListImmediate&) mutable
            {
                TestCustomRTDepthBufferTest.BeginRendering(world);
                Initialized = true;
            });
        }
        
    }

    if (GReadCustomRTDepthBufferTestTest != 0)
    {
        TArray<FColor> bitmap;
        FCustomShaderUtility::ReadPixels(RenderTexture, bitmap);
        GReadCustomRTDepthBufferTestTest = 0;
    }
}
void ACustomRTDepthBufferTestRunner::BeginDestroy()
{
    Super::BeginDestroy();
    EndRendering();
}

void ACustomRTDepthBufferTestRunner::EndRendering()
{
    TestCustomRTDepthBufferTest.EndRendering();
    Initialized = false;
}

void ACustomRTDepthBufferTestRunner::Init()
{
    RenderTexture = FCustomShaderUtility::MakeRT(this, 1024, 1024);
}

void ACustomRTDepthBufferTestRunner::StaticToggleEditorSimulateMode(bool bSimulateOnEditor)
{
    GUseCustomRTDepthBufferTestTest = bSimulateOnEditor;
}
