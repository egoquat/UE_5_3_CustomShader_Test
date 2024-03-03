#include "CustomRTDepthBufferTestHwrt.h"

#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderTargetPool.h"
#include "RHI.h"
#include "Modules/ModuleManager.h"
#include "RayTracingPayloadType.h"
#include "../Private/RayTracing/RayTracingScene.h"
#include "../Private/SceneRendering.h"
#include "RenderGraphUtils.h"
#include "CustomShaderCommon.h"
#include "Runtime/Renderer/Private/ScenePrivate.h"

#define NUM_THREADS_PER_GROUP_DIMENSION 8

class FCustomRTDepthBufferTestRHIRGS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCustomRTDepthBufferTestRHIRGS)
	SHADER_USE_ROOT_PARAMETER_STRUCT(FCustomRTDepthBufferTestRHIRGS, FGlobalShader)
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        SHADER_PARAMETER_SRV(RaytracingAccelerationStructure, TLAS)
        //SHADER_PARAMETER_RDG_BUFFER_SRV(RaytracingAccelerationStructure, TLAS)
    
		SHADER_PARAMETER_UAV(RWTexture2D<float4>, outTex)
        SHADER_PARAMETER(FIntPoint, OutputTextureRes)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, ViewUniformBuffer)
	END_SHADER_PARAMETER_STRUCT()
 
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}
 
	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 /*PermutationId*/)
	{
		return ERayTracingPayloadType::VFX;
	}
 
	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
 
		//We're using it here to add some preprocessor defines. That way we don't have to change both C++ and HLSL code when we change the value for NUM_THREADS_PER_GROUP_DIMENSION
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), NUM_THREADS_PER_GROUP_DIMENSION);

	    OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_PRIMITIVE_SCENE_DATA"), 1);
	    OutEnvironment.SetDefine(TEXT("USE_GLOBAL_GPU_SCENE_DATA"), 1);
	    OutEnvironment.SetDefine(TEXT("RT_PAYLOAD_TYPE"), static_cast<int32>(ERayTracingPayloadType::VFX));
	}
};
 
class FCustomRTDepthBufferTestRHICHS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCustomRTDepthBufferTestRHICHS)
	//SHADER_USE_ROOT_PARAMETER_STRUCT(FCustomRTDepthBufferTestRHICHS, FGlobalShader)
 
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}
 
	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 PermutationId)
	{
	    return ERayTracingPayloadType::VFX;
	}
    
	using FParameters = FEmptyShaderParameters;

    FCustomRTDepthBufferTestRHICHS() = default;
    FCustomRTDepthBufferTestRHICHS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer)
    {}
};
 
class FCustomRTDepthBufferTestRHIMS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCustomRTDepthBufferTestRHIMS)
	//SHADER_USE_ROOT_PARAMETER_STRUCT(FCustomRTDepthBufferTestRHIMS, FGlobalShader)
 
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}
 
	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 PermutationId)
	{
	    return ERayTracingPayloadType::VFX;
	}
 
	using FParameters = FEmptyShaderParameters;

    FCustomRTDepthBufferTestRHIMS() = default;
    FCustomRTDepthBufferTestRHIMS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer)
    {}
};

IMPLEMENT_GLOBAL_SHADER(FCustomRTDepthBufferTestRHIRGS, "/CustomShaders/CustomRTDepthBufferTestShader.usf", "RayTraceTestRGS", SF_RayGen);
IMPLEMENT_GLOBAL_SHADER(FCustomRTDepthBufferTestRHICHS, "/CustomShaders/CustomRTDepthBufferTestShader.usf", "closestHit=RayTraceTestCHS", SF_RayHitGroup);
IMPLEMENT_GLOBAL_SHADER(FCustomRTDepthBufferTestRHIMS,  "/CustomShaders/CustomRTDepthBufferTestShader.usf", "RayTraceTestMS", SF_RayMiss);
 
FCustomRTDepthBufferTestHwrt::FCustomRTDepthBufferTestHwrt()
{
}
 
void FCustomRTDepthBufferTestHwrt::BeginRendering(UWorld* world)
{
	//If the handle is already initialized and valid, no need to do anything
	if (PostOpaqueRenderDelegate.IsValid())
	{
		return;
	}

	const FName RendererModuleName("Renderer");
	IRendererModule* RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
	if (RendererModule)
	{
		PostOpaqueRenderDelegate = RendererModule->RegisterPostOpaqueRenderDelegate(FPostOpaqueRenderDelegate::CreateRaw(this, &FCustomRTDepthBufferTestHwrt::Execute_RenderThread));
	}
	
	FIntPoint TextureSize = { CachedParams.RenderTexture->SizeX, CachedParams.RenderTexture->SizeY };
	FRHITextureCreateDesc TextureDesc = FRHITextureCreateDesc::Create2D(TEXT("RaytracingTestOutput"), TextureSize.X, TextureSize.Y, CachedParams.RenderTexture->GetFormat());
	TextureDesc.AddFlags(TexCreate_ShaderResource | TexCreate_UAV);
	ShaderOutputTexture = RHICreateTexture(TextureDesc);
    FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
    ShaderOutputTextureUAV = RHICmdList.CreateUnorderedAccessView(ShaderOutputTexture);
    FSceneInterface* scene = world->Scene;
    RayTracingSceneView = GetRayTracingSceneView(RHICmdList, scene);
}
 
void FCustomRTDepthBufferTestHwrt::EndRendering()
{
	if (!PostOpaqueRenderDelegate.IsValid())
	{
		return;
	}

	const FName RendererModuleName("Renderer");
	IRendererModule* RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
	if (RendererModule)
	{
		RendererModule->RemovePostOpaqueRenderDelegate(PostOpaqueRenderDelegate);
	}
 
	PostOpaqueRenderDelegate.Reset();
}
 
void FCustomRTDepthBufferTestHwrt::UpdateParameters(FCustomRTDepthBufferTestRHIParameters& DrawParameters)
{
	CachedParams = DrawParameters;
	bCachedParamsAreValid = true;
}

FRHIShaderResourceView* FCustomRTDepthBufferTestHwrt::GetRayTracingSceneView(const FSceneInterface* InScene)
{
    return GetRayTracingSceneView(FRHICommandListImmediate::Get(), InScene);
}

FRHIShaderResourceView* FCustomRTDepthBufferTestHwrt::GetRayTracingSceneView(FRHICommandListBase& RHICmdList, const FSceneInterface* InScene)
{
    if (const FScene* Scene = InScene->GetRenderScene())
    {
        return Scene->RayTracingScene.CreateLayerViewRHI(RHICmdList, ERayTracingSceneLayer::Base);
    }

    return nullptr;
}
 
void FCustomRTDepthBufferTestHwrt::Execute_RenderThread(FPostOpaqueRenderParameters& Parameters)
#if RHI_RAYTRACING
{
    bool bCalledBySceneCapture = Parameters.View->PlayerIndex == INDEX_NONE;
    if (bCalledBySceneCapture == true) return;
    
	FRDGBuilder* GraphBuilder = Parameters.GraphBuilder;    
	FRHICommandListImmediate& RHICmdList = GraphBuilder->RHICmdList;
	if (!(bCachedParamsAreValid && CachedParams.RenderTexture))
	{
		return;
	}

    bool isOutTextureReady = ShaderOutputTexture.IsValid() == true;
    if (isOutTextureReady == false) return;
	
	check(IsInRenderingThread());

    FIntPoint TextureSize = { CachedParams.RenderTexture->SizeX, CachedParams.RenderTexture->SizeY };
 
	// set shader parameters
	FCustomRTDepthBufferTestRHIRGS::FParameters *PassParameters = GraphBuilder->AllocParameters<FCustomRTDepthBufferTestRHIRGS::FParameters>();
	PassParameters->ViewUniformBuffer = Parameters.View->ViewUniformBuffer;
#if (ENGINE_MINOR_VERSION < 3)
	PassParameters->TLAS = CachedParams.Scene->GetLayerSRVChecked(ERayTracingSceneLayer::Base);
#else
    //1.
    //PassParameters->TLAS             = CachedParams.Scene->GetLayerView(ERayTracingSceneLayer::Base);
    //PassParameters->TLAS = Parameters.View->GetRayTracingSceneLayerViewChecked(ERayTracingSceneLayer::Base);
    //GetRayTracingSceneLayerViewChecked
    
    //2.
    //FSceneInterface* scene = GWorld->Scene;
    //FShaderResourceViewRHIRef rayTracingSceneView = GetRayTracingSceneView(RHICmdList, scene);
    //PassParameters->TLAS = rayTracingSceneView;
    PassParameters->TLAS = RayTracingSceneView;
#endif  // ENGINE_MINOR_VERSION < 3	
	PassParameters->outTex = ShaderOutputTextureUAV;
    PassParameters->OutputTextureRes = TextureSize; 
 
	// define render pass needed parameters
	TShaderMapRef<FCustomRTDepthBufferTestRHIRGS> CustomRTDepthBufferTestRHIRGS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FRHIRayTracingScene* RHIScene = CachedParams.Scene->GetRHIRayTracingScene();
    //FRHIRayTracingScene* RHIScene = Parameters.View->GetRayTracingSceneChecked();
 
	// add the ray trace dispatch pass
	GraphBuilder->AddPass(
		RDG_EVENT_NAME("CustomRTDepthBufferTestHwrt"),
		PassParameters,
		ERDGPassFlags::Compute,
		[PassParameters, CustomRTDepthBufferTestRHIRGS, TextureSize, RHIScene](FRHIRayTracingCommandList& RHICmdList)
		{
			FRayTracingShaderBindingsWriter GlobalResources;
			SetShaderParameters(GlobalResources, CustomRTDepthBufferTestRHIRGS, *PassParameters);
 
			FRayTracingPipelineStateInitializer PSOInitializer;
			PSOInitializer.MaxPayloadSizeInBytes = sizeof(FVFXTracePayload);
			PSOInitializer.bAllowHitGroupIndexing = false;
 
			// Set RayGen shader
			TArray<FRHIRayTracingShader*> RayGenShaderTable;
			RayGenShaderTable.Add(GetGlobalShaderMap(GMaxRHIFeatureLevel)->GetShader<FCustomRTDepthBufferTestRHIRGS>().GetRayTracingShader());
			PSOInitializer.SetRayGenShaderTable(RayGenShaderTable);
 
			// Set ClosestHit shader
			TArray<FRHIRayTracingShader*> RayHitShaderTable;
			RayHitShaderTable.Add(GetGlobalShaderMap(GMaxRHIFeatureLevel)->GetShader<FCustomRTDepthBufferTestRHICHS>().GetRayTracingShader());
			PSOInitializer.SetHitGroupTable(RayHitShaderTable);
			
			// Set Miss shader
			TArray<FRHIRayTracingShader*> RayMissShaderTable;
			RayMissShaderTable.Add(GetGlobalShaderMap(GMaxRHIFeatureLevel)->GetShader<FCustomRTDepthBufferTestRHIMS>().GetRayTracingShader());
			PSOInitializer.SetMissShaderTable(RayMissShaderTable);
 
			// dispatch ray trace shader
			FRayTracingPipelineState* PipeLine = PipelineStateCache::GetAndOrCreateRayTracingPipelineState(RHICmdList, PSOInitializer);
			RHICmdList.SetRayTracingMissShader(RHIScene, 0, PipeLine, 0 /* ShaderIndexInPipeline */, 0, nullptr, 0);
			RHICmdList.RayTraceDispatch(PipeLine, CustomRTDepthBufferTestRHIRGS.GetRayTracingShader(), RHIScene, GlobalResources, TextureSize.X, TextureSize.Y);
		}
	);
 
	FTexture2DRHIRef OriginalRT = CachedParams.RenderTexture->GetRenderTargetResource()->GetTexture2DRHI();
	FRDGTexture* OutputRDGTexture = GraphBuilder->RegisterExternalTexture(CreateRenderTarget(ShaderOutputTexture, TEXT("RaytracingTestOutputRT")));
	FRDGTexture* CopyToRDGTexture = GraphBuilder->RegisterExternalTexture(CreateRenderTarget(OriginalRT, TEXT("RaytracingTestCopyToRT")));
	FRHICopyTextureInfo CopyInfo;
	CopyInfo.Size = FIntVector(TextureSize.X, TextureSize.Y, 0);
	AddCopyTexturePass(*GraphBuilder, OutputRDGTexture, CopyToRDGTexture, CopyInfo);
}
#else // !RHI_RAYTRACING
{
	unimplemented();
}
#endif