#pragma once
 
#include "CoreMinimal.h"
#include "RenderGraphUtils.h"
#include "CustomShaderCommon.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"
 
class FRayTracingScene;
 
struct  FCustomRTRayRHIParameters
{
 
	FIntPoint GetRenderTargetSize() const
	{
		return CachedRenderTargetSize;
	}
 
	FCustomRTRayRHIParameters() {}; // consider delete this, otherwise the target size will not be set, or just add setter
	FCustomRTRayRHIParameters(UTextureRenderTarget2D* IORenderTarget)
		: RenderTarget(IORenderTarget)
	{
		CachedRenderTargetSize = RenderTarget ? FIntPoint(RenderTarget->SizeX, RenderTarget->SizeY) : FIntPoint::ZeroValue;
	}
 
	UTextureRenderTarget2D* RenderTarget = nullptr;
	FRayTracingScene* Scene = nullptr;
	FIntPoint CachedRenderTargetSize = FIntPoint::ZeroValue;
};
 
class CUSTOMSHADER_API FCustomRTRayHwrt
{
public:
	FCustomRTRayHwrt();
 
	void BeginRendering();
	void EndRendering();
	void UpdateParameters(FCustomRTRayRHIParameters& DrawParameters);

    void Initialize(AActor* actorView);
private:
	void Execute_RenderThread(FPostOpaqueRenderParameters& Parameters);
	
	FDelegateHandle PostOpaqueRenderDelegate;
	FCustomRTRayRHIParameters CachedParams;
	volatile bool bCachedParamsAreValid;
 
	FTexture2DRHIRef ShaderOutputTexture;
	FUnorderedAccessViewRHIRef ShaderOutputTextureUAV;
    AActor* ActorView = nullptr;

public:
    float FOVView = FConstCustomRay::FOV;
    float FarClipView = FConstCustomRay::FarClipView;
    FRotator RotateView = FRotator::ZeroRotator;
    FRotator RotateMainCamera = FRotator::ZeroRotator;
};
