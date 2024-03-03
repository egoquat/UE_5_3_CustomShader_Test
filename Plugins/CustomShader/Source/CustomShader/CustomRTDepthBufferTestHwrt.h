#pragma once
 
#include "CoreMinimal.h"
 
#include "RenderGraphUtils.h"
#include "Engine/TextureRenderTargetVolume.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2DDynamic.h"
 
class FRayTracingScene;
 
struct  FCustomRTDepthBufferTestRHIParameters
{
 
	FIntPoint GetRenderTargetSize() const
	{
		return CachedRenderTargetSize;
	}
 
	FCustomRTDepthBufferTestRHIParameters() {}; 
	FCustomRTDepthBufferTestRHIParameters(UTextureRenderTarget2D* RenderTextureIn)
		: RenderTexture(RenderTextureIn)
	{
		CachedRenderTargetSize = RenderTexture ? FIntPoint(RenderTexture->SizeX, RenderTexture->SizeY) : FIntPoint::ZeroValue;
	}
 
    UTextureRenderTarget2D* RenderTexture = nullptr;
    FRayTracingScene* Scene = nullptr;
    FIntPoint CachedRenderTargetSize = FIntPoint::ZeroValue;
};
 
class CUSTOMSHADER_API FCustomRTDepthBufferTestHwrt
{
public:
	FCustomRTDepthBufferTestHwrt();
 
	void BeginRendering(UWorld* world);
	void EndRendering();
	void UpdateParameters(FCustomRTDepthBufferTestRHIParameters& DrawParameters);
private:
    FRHIShaderResourceView* GetRayTracingSceneView(FRHICommandListBase& RHICmdList, const FSceneInterface* Scene);
    FRHIShaderResourceView* GetRayTracingSceneView(const FSceneInterface* Scene);
    
	void Execute_RenderThread(FPostOpaqueRenderParameters& Parameters);
	
	FDelegateHandle PostOpaqueRenderDelegate;
	FCustomRTDepthBufferTestRHIParameters CachedParams;
	volatile bool bCachedParamsAreValid;
 
	FTexture2DRHIRef ShaderOutputTexture;
	FUnorderedAccessViewRHIRef ShaderOutputTextureUAV;
	FShaderResourceViewRHIRef RayTracingSceneView;
};
