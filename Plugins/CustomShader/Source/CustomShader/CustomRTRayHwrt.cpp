#include "CustomRTRayHwrt.h"
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
#include "CustomShaderUtility.h"

#define TEST_USE_INV_TRANSLATE_FORCE 0
#define NUM_THREADS_PER_GROUP_DIMENSION 8

class FCustomRTRayRHIRGS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCustomRTRayRHIRGS)
	SHADER_USE_ROOT_PARAMETER_STRUCT(FCustomRTRayRHIRGS, FGlobalShader)
	
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER_UAV(RWTexture2D<float4>, outTex)
		SHADER_PARAMETER_SRV(RaytracingAccelerationStructure, TLAS)
        //SHADER_PARAMETER_RDG_BUFFER_SRV(RaytracingAccelerationStructure, TLAS)
        SHADER_PARAMETER(FIntPoint, OutputTextureRes)
        SHADER_PARAMETER(FVector3f, PositionWorldView)
        SHADER_PARAMETER(FMatrix44f, PositionToTranslatedWorldView)
        SHADER_PARAMETER(float, FarClipView)
        SHADER_PARAMETER(FVector3f, RollPitchYawView)
        SHADER_PARAMETER(FVector3f, RollPitchYawCamera)
        SHADER_PARAMETER(FVector3f, UpVecView)
        SHADER_PARAMETER(FVector3f, RightVecView)
        SHADER_PARAMETER(FVector3f, ForwardVecView)
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
 
class FCustomRTRayRHICHS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCustomRTRayRHICHS)
	//SHADER_USE_ROOT_PARAMETER_STRUCT(FCustomRTRayRHICHS, FGlobalShader)
 
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}
 
	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 PermutationId)
	{
	    return ERayTracingPayloadType::VFX;
	}
    
	using FParameters = FEmptyShaderParameters;

    FCustomRTRayRHICHS() = default;
    FCustomRTRayRHICHS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer)
    {}
};
 
class FCustomRTRayRHIMS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCustomRTRayRHIMS)
	//SHADER_USE_ROOT_PARAMETER_STRUCT(FCustomRTRayRHIMS, FGlobalShader)
 
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}
 
	static ERayTracingPayloadType GetRayTracingPayloadType(const int32 PermutationId)
	{
	    return ERayTracingPayloadType::VFX;
	}
 
	using FParameters = FEmptyShaderParameters;

    FCustomRTRayRHIMS() = default;
    FCustomRTRayRHIMS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer)
    {}
};

IMPLEMENT_GLOBAL_SHADER(FCustomRTRayRHIRGS, "/CustomShaders/CustomRTRayShader.usf", "RayTraceTestRGS", SF_RayGen);
IMPLEMENT_GLOBAL_SHADER(FCustomRTRayRHICHS, "/CustomShaders/CustomRTRayShader.usf", "closestHit=RayTraceTestCHS", SF_RayHitGroup);
IMPLEMENT_GLOBAL_SHADER(FCustomRTRayRHIMS,  "/CustomShaders/CustomRTRayShader.usf", "RayTraceTestMS", SF_RayMiss);
 
FCustomRTRayHwrt::FCustomRTRayHwrt()
{
}
 
void FCustomRTRayHwrt::BeginRendering()
{
	//If the handle is already initialized and valid, no need to do anything
	if (PostOpaqueRenderDelegate.IsValid())
	{
		return;
	}
	//Get the Renderer Module and add our entry to the callbacks so it can be executed each frame after the scene rendering is done
	const FName RendererModuleName("Renderer");
	IRendererModule* RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
	if (RendererModule)
	{
		PostOpaqueRenderDelegate = RendererModule->RegisterPostOpaqueRenderDelegate(FPostOpaqueRenderDelegate::CreateRaw(this, &FCustomRTRayHwrt::Execute_RenderThread));
	}
 
	// create output texture
	FIntPoint TextureSize = { CachedParams.RenderTarget->SizeX, CachedParams.RenderTarget->SizeY };
	FRHITextureCreateDesc TextureDesc = FRHITextureCreateDesc::Create2D(TEXT("RaytracingTestOutput"), TextureSize.X, TextureSize.Y, CachedParams.RenderTarget->GetFormat());
	TextureDesc.AddFlags(TexCreate_ShaderResource | TexCreate_UAV);
	ShaderOutputTexture = RHICreateTexture(TextureDesc);
    FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
	ShaderOutputTextureUAV = RHICmdList.CreateUnorderedAccessView(ShaderOutputTexture);
}
 
//Stop the compute shader execution
void FCustomRTRayHwrt::EndRendering()
{
	//If the handle is not valid then there's no cleanup to do
	if (!PostOpaqueRenderDelegate.IsValid())
	{
		return;
	}
	//Get the Renderer Module and remove our entry from the PostOpaqueRender callback
	const FName RendererModuleName("Renderer");
	IRendererModule* RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
	if (RendererModule)
	{
		RendererModule->RemovePostOpaqueRenderDelegate(PostOpaqueRenderDelegate);
	}
 
	PostOpaqueRenderDelegate.Reset();
}
 
void FCustomRTRayHwrt::UpdateParameters(FCustomRTRayRHIParameters& DrawParameters)
{
	CachedParams = DrawParameters;
	bCachedParamsAreValid = true;
}

void FCustomRTRayHwrt::Initialize(AActor* actorView)
{
    ActorView = actorView;
}

static const FMatrix InvertProjectionMatrix( const FMatrix& M )
{
	if( M.M[1][0] == 0.0f &&
		M.M[3][0] == 0.0f &&
		M.M[0][1] == 0.0f &&
		M.M[3][1] == 0.0f &&
		M.M[0][2] == 0.0f &&
		M.M[1][2] == 0.0f &&
		M.M[0][3] == 0.0f &&
		M.M[1][3] == 0.0f &&
		M.M[2][3] == 1.0f &&
		M.M[3][3] == 0.0f )
	{
		// Solve the common case directly with very high precision.

	    /*
		M = 
		| a | 0 | 0 | 0 |
		| 0 | b | 0 | 0 |
		| s | t | c | 1 |
		| 0 | 0 | d | 0 |
		*/

		double a = M.M[0][0];
		double b = M.M[1][1];
		double c = M.M[2][2];
		double d = M.M[3][2];
		double s = M.M[2][0];
		double t = M.M[2][1];

		return FMatrix(
			FPlane( 1.0 / a, 0.0f, 0.0f, 0.0f ),
			FPlane( 0.0f, 1.0 / b, 0.0f, 0.0f ),
			FPlane( 0.0f, 0.0f, 0.0f, 1.0 / d ),
			FPlane( -s/a, -t/b, 1.0f, -c/d )
		);
	}
	else
	{
		return M.Inverse();
	}
}

void FCustomRTRayHwrt::Execute_RenderThread(FPostOpaqueRenderParameters& Parameters)
#if RHI_RAYTRACING
{
	FRDGBuilder* GraphBuilder = Parameters.GraphBuilder;
	FRHICommandListImmediate& RHICmdList = GraphBuilder->RHICmdList;
	
	if (!(bCachedParamsAreValid && CachedParams.RenderTarget))
	{
		return;
	}

    bool isOutTextureReady = ShaderOutputTexture.IsValid() == true;
    if (isOutTextureReady == false) return;
    
	check(IsInRenderingThread());

    FIntPoint TextureSize = { CachedParams.RenderTarget->SizeX, CachedParams.RenderTarget->SizeY };

    FMatrix44f SVPositionToTranslatedWorld;

#if TEST_USE_INV_TRANSLATE_FORCE == 1
	// set shader parameters
    const int32 SizeX = TextureSize.X; // 1241
    const int32 SizeY = TextureSize.Y; // 920
    float XAxisMultiplier = 1.0f;
    float YAxisMultiplier = SizeX / (float)SizeY; // 1.34891307
    
    float HalfFOV = FMath::Max(0.001f, FOVView) * ((float)UE_PI / 180.0f) / 2.0f; // 0.785398245
    float ClippingPlane = GNearClippingPlane; // 10.0f

    FMatrix projectionMatrix = FReversedZPerspectiveMatrix(
                HalfFOV,
                HalfFOV,
                XAxisMultiplier,
                YAxisMultiplier,
                ClippingPlane,
                ClippingPlane);

    projectionMatrix = AdjustProjectionMatrixForRHI(projectionMatrix);
    FMatrix invProjectionMatrix = InvertProjectionMatrix(projectionMatrix);

    const FTransform& actorTransform = ActorView->GetActorTransform();
    FRotator rotateActor = actorTransform.Rotator();
    FMatrix viewRotationMatrix = FInverseRotationMatrix(rotateActor);
    FVector3d localViewOrigin = actorTransform.GetLocation();
    FMatrix InvTranslatedViewMatrix = viewRotationMatrix.GetTransposed();
    FMatrix InvViewProjectionMatrix = invProjectionMatrix * InvTranslatedViewMatrix;
    
    const FVector2f ViewSizeAndInvSize = FVector2f(1.0f / (float)TextureSize.X, 1.0f / (float)TextureSize.Y);
    float Mx = 2.0f * ViewSizeAndInvSize.X;
    float My = -2.0f * ViewSizeAndInvSize.Y;
    float Ax = -1.0f - 2.0f * 0.0f * ViewSizeAndInvSize.X;
    float Ay = 1.0f + 2.0f * 0.0f * ViewSizeAndInvSize.Y;
    
    SVPositionToTranslatedWorld = FMatrix44f(
    	FMatrix(FPlane(Mx, 0, 0, 0),
    		FPlane(0, My, 0, 0),
    		FPlane(0, 0, 1, 0),
    		FPlane(Ax, Ay, 0, 1)) * InvViewProjectionMatrix);
#endif // #if TEST_USE_INV_TRANSLATE_FORCE == 1
    
	FCustomRTRayRHIRGS::FParameters *PassParameters = GraphBuilder->AllocParameters<FCustomRTRayRHIRGS::FParameters>();
	PassParameters->ViewUniformBuffer = Parameters.View->ViewUniformBuffer;
#if (ENGINE_MINOR_VERSION < 3)
	PassParameters->TLAS = CachedParams.Scene->GetLayerSRVChecked(ERayTracingSceneLayer::Base);
#else
    //PassParameters->TLAS = CachedParams.Scene->GetLayerView(ERayTracingSceneLayer::Base);
#endif  // ENGINE_MINOR_VERSION < 3
    FRotator vecForCoord = RotateView;
	PassParameters->outTex = ShaderOutputTextureUAV;
    PassParameters->OutputTextureRes = TextureSize;
    PassParameters->PositionWorldView = FVector3f(ActorView->GetActorLocation());
    PassParameters->PositionToTranslatedWorldView = SVPositionToTranslatedWorld;
    PassParameters->FarClipView = FarClipView;
    PassParameters->RollPitchYawView = FVector3f(RotateView.Euler());
    PassParameters->RollPitchYawCamera = FVector3f(RotateMainCamera.Euler());
    PassParameters->ForwardVecView = FVector3f(vecForCoord.Vector());
    vecForCoord.Yaw = vecForCoord.Yaw + 90.0f;
    PassParameters->RightVecView = FVector3f(vecForCoord.Vector());
    vecForCoord.Yaw = vecForCoord.Yaw - 90.0f;
    vecForCoord.Pitch = vecForCoord.Pitch + 90.0f;
    PassParameters->UpVecView = FVector3f(vecForCoord.Vector());
    // set shader parameters //
 
	// define render pass needed parameters
	TShaderMapRef<FCustomRTRayRHIRGS> CustomRTRayRHIRGS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FRHIRayTracingScene* RHIScene = CachedParams.Scene->GetRHIRayTracingScene();
    //FRHIRayTracingScene* RHIScene = Parameters.View->GetRayTracingSceneChecked();
 
	// add the ray trace dispatch pass
	GraphBuilder->AddPass(
		RDG_EVENT_NAME("CustomRTRayHwrt"),
		PassParameters,
		ERDGPassFlags::Compute,
		[PassParameters, CustomRTRayRHIRGS, TextureSize, RHIScene](FRHIRayTracingCommandList& RHICmdList)
		{
			FRayTracingShaderBindingsWriter GlobalResources;
			SetShaderParameters(GlobalResources, CustomRTRayRHIRGS, *PassParameters);
 
			FRayTracingPipelineStateInitializer PSOInitializer;
			PSOInitializer.MaxPayloadSizeInBytes = sizeof(FVFXTracePayload);
			PSOInitializer.bAllowHitGroupIndexing = false;
 
			// Set RayGen shader
			TArray<FRHIRayTracingShader*> RayGenShaderTable;
			RayGenShaderTable.Add(GetGlobalShaderMap(GMaxRHIFeatureLevel)->GetShader<FCustomRTRayRHIRGS>().GetRayTracingShader());
			PSOInitializer.SetRayGenShaderTable(RayGenShaderTable);
 
			// Set ClosestHit shader
			TArray<FRHIRayTracingShader*> RayHitShaderTable;
			RayHitShaderTable.Add(GetGlobalShaderMap(GMaxRHIFeatureLevel)->GetShader<FCustomRTRayRHICHS>().GetRayTracingShader());
			PSOInitializer.SetHitGroupTable(RayHitShaderTable);
			
			// Set Miss shader
			TArray<FRHIRayTracingShader*> RayMissShaderTable;
			RayMissShaderTable.Add(GetGlobalShaderMap(GMaxRHIFeatureLevel)->GetShader<FCustomRTRayRHIMS>().GetRayTracingShader());
			PSOInitializer.SetMissShaderTable(RayMissShaderTable);
 
			// dispatch ray trace shader
			FRayTracingPipelineState* PipeLine = PipelineStateCache::GetAndOrCreateRayTracingPipelineState(RHICmdList, PSOInitializer);
			RHICmdList.SetRayTracingMissShader(RHIScene, 0, PipeLine, 0 /* ShaderIndexInPipeline */, 0, nullptr, 0);
			RHICmdList.RayTraceDispatch(PipeLine, CustomRTRayRHIRGS.GetRayTracingShader(), RHIScene, GlobalResources, TextureSize.X, TextureSize.Y);
		}
	);
 
	// Copy textures from the shader output to our render target
	// this is done as a render pass with the graph builder
	FTexture2DRHIRef OriginalRT = CachedParams.RenderTarget->GetRenderTargetResource()->GetTexture2DRHI();
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