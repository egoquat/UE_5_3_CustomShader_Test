#include "CustomRTRayRunner.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#include "CustomShaderUtility.h"
#include "Engine/World.h"
#include "SceneInterface.h"
#include "../Private/ScenePrivate.h"
#include "Kismet/KismetMaterialLibrary.h"

static int GUseCustomRTRay = 1;
FAutoConsoleVariableRef CUseCustomRTRay(
TEXT("Keti.Sensor.UseCustomRTRay"),
GUseCustomRTRay,
TEXT("default 1\n"),
ECVF_Cheat
);
 
ACustomRTRayRunner::ACustomRTRayRunner()
{
    USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));
    RootComponent = SceneComponent;

    PrimaryActorTick.bCanEverTick = true;
}

void ACustomRTRayRunner::Initialize(AActor* actorView)
{
    ActorView = actorView;

    //UTextureRenderTarget2D* rtRaycastAsset = Cast<UTextureRenderTarget2D>(
    //                        StaticLoadObject(UTextureRenderTarget2D::StaticClass(), this,
    //                            TEXT("/Script/Engine.TextureRenderTarget2D'/Game/Test/TestCustomRTDepthBufferTest/RT_CustomTest01.RT_CustomTest01'")));
    //ensure(rtRaycastAsset);
    //FName nameUniqueRT = MakeUniqueObjectName(rtRaycastAsset->GetOuter(), UTextureRenderTarget2D::StaticClass());
    //UTextureRenderTarget2D* rtRaycast = NewObject<UTextureRenderTarget2D>(rtRaycastAsset, nameUniqueRT);
    //ensure(rtRaycast);  
    //RenderTexture = rtRaycast;
    
    RenderTexture = FCustomShaderUtility::MakeRT(this, 1024, 1024);
    
    CustomRTRay = FCustomRTRayHwrt();
    CustomRTRay.Initialize(ActorView);
    Initialized = false;
 
    if (RenderTexture != nullptr)
        UpdateTestParameters();

#if WITH_EDITOR
    if (GetWorld()->IsPlayInEditor())
    {
        FEditorDelegates::OnPreSwitchBeginPIEAndSIE.AddStatic(&ACustomRTRayRunner::StaticToggleEditorSimulateMode);
    }
#endif
}
 
void ACustomRTRayRunner::UpdateTestParameters()
{
    // Get the player controller
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    FRotator CameraRotation;
    
    if (PlayerController)
    {
        // Get the player camera manager
        APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;

        if (CameraManager)
        {
            FTransform CameraTransform = CameraManager->GetTransform();
            CameraRotation = CameraTransform.Rotator();
        }
    }
    FRotator rotateSensor = ActorView->GetActorRotation();
    
    FCustomRTRayRHIParameters parameters;
    parameters.Scene = &GetWorld()->Scene->GetRenderScene()->RayTracingScene;
    parameters.CachedRenderTargetSize = FIntPoint(RenderTexture->SizeX, RenderTexture->SizeY);
    parameters.RenderTarget = RenderTexture;
    CustomRTRay.RotateView = rotateSensor;
    CustomRTRay.RotateMainCamera = CameraRotation;
    CustomRTRay.UpdateParameters(parameters);
}
 
// Called every frame
void ACustomRTRayRunner::Tick(float DeltaTime)
{
    if (GUseCustomRTRay != 1)
    {
        EndRendering();
        return;
    }
    
    FRayTracingScene* rtScene = &GetWorld()->Scene->GetRenderScene()->RayTracingScene;
    bool isInitializedRT = rtScene->IsCreated();
    if (isInitializedRT == false)
    {
        return;
    }
    
    TranscurredTime+=DeltaTime;
    Super::Tick(DeltaTime);
    if (RenderTexture == nullptr) return;
    
    if(TranscurredTime>1.0f)
    {
        UpdateTestParameters();

        ENQUEUE_RENDER_COMMAND(TestCustomRTRayBeginRendering)(
        [this, &CustomRTRay = CustomRTRay](FRHICommandListImmediate&) mutable
        {
            if(!Initialized)
            {
                CustomRTRay.BeginRendering();
                Initialized = true;
            } 
        });
    }
}

void ACustomRTRayRunner::TickRayPickTest(float DeltaTime)
{
}

void ACustomRTRayRunner::BeginDestroy()
{
    Super::BeginDestroy();
    EndRendering();
}

void ACustomRTRayRunner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    CustomRTRay.FarClipView = FarClipView;
    CustomRTRay.FOVView = FOVView;
}

void ACustomRTRayRunner::EndRendering()
{
    CustomRTRay.EndRendering();
    Initialized = false;
}

void ACustomRTRayRunner::StaticToggleEditorSimulateMode(bool bSimulateOnEditor)
{
    GUseCustomRTRay = bSimulateOnEditor;
}
