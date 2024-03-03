
#include "CustomShaderUtility.h"

#include "Engine/Texture2DDynamic.h"
#include "Engine/TextureRenderTarget2D.h"

static FReadSurfaceDataFlags ReadPixelFlags(ERangeCompressionMode::RCM_UNorm);

UTextureRenderTarget2D* FCustomShaderUtility::MakeRT(const AActor* actorOwned, int resX, int resY)
{
    UTextureRenderTarget2D* renderTarget = NewObject<UTextureRenderTarget2D>(actorOwned->GetRootComponent());
    //renderTargetOut->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
    renderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8_SRGB;
    renderTarget->CompressionSettings = TextureCompressionSettings::TC_Default;
    renderTarget->SRGB = false;
    renderTarget->bAutoGenerateMips = false;
    renderTarget->bGPUSharedFlag = true;
    renderTarget->AddressX = TextureAddress::TA_Clamp;
    renderTarget->AddressY = TextureAddress::TA_Clamp;
    renderTarget->InitAutoFormat(resX, resY);
    renderTarget->UpdateResourceImmediate(true);
    return renderTarget;
}

UTexture2D* FCustomShaderUtility::CreateTexture2D(int32 InSizeX, int32 InSizeY, UObject* InOuter, EPixelFormat InFormat, const FName InName)
{
    LLM_SCOPE(ELLMTag::Textures);

    UTexture2D* NewTexture = NULL;
    if (InSizeX > 0 && InSizeY > 0 &&
        (InSizeX % GPixelFormats[InFormat].BlockSizeX) == 0 &&
        (InSizeY % GPixelFormats[InFormat].BlockSizeY) == 0)
    {
        NewTexture = NewObject<UTexture2D>(InOuter, InName, RF_Public | RF_Standalone);
        NewTexture->SetPlatformData(new FTexturePlatformData());
        NewTexture->GetPlatformData()->SizeX = InSizeX;
        NewTexture->GetPlatformData()->SizeY = InSizeY;
        NewTexture->GetPlatformData()->PixelFormat = InFormat;

        // Allocate first mipmap.
        int32 NumBlocksX = InSizeX / GPixelFormats[InFormat].BlockSizeX;
        int32 NumBlocksY = InSizeY / GPixelFormats[InFormat].BlockSizeY;
        FTexture2DMipMap* Mip = new FTexture2DMipMap();
        NewTexture->GetPlatformData()->Mips.Add(Mip);
        Mip->SizeX = InSizeX;
        Mip->SizeY = InSizeY;
        Mip->BulkData.Lock(LOCK_READ_WRITE);
        Mip->BulkData.Realloc(NumBlocksX * NumBlocksY * GPixelFormats[InFormat].BlockBytes);
        Mip->BulkData.Unlock();
    }
    else
    {
        UE_LOG(LogTexture, Warning, TEXT("Invalid parameters specified for UTexture2D::CreateTransient()"));
    }
    return NewTexture;
}

UTexture2DDynamic* FCustomShaderUtility::CreateTexture2DDynamicInternal(int32 InSizeX, int32 InSizeY, EPixelFormat InFormat, UObject* InOuter, const FName InName)
{
    ensure(InSizeX > 0 && InSizeY > 0);
    
    FTexture2DDynamicCreateInfo CreateInfo(InFormat);
    EPixelFormat DesiredFormat = EPixelFormat(CreateInfo.Format);
    	
    UTexture2DDynamic* NewTexture = NewObject<UTexture2DDynamic>(InOuter, InName);
    if (NewTexture != NULL)
    {
        NewTexture->Filter = CreateInfo.Filter;
        NewTexture->SamplerAddressMode = CreateInfo.SamplerAddressMode;
        NewTexture->SRGB = CreateInfo.bSRGB;

        // Disable compression
        NewTexture->CompressionSettings		= TC_Default;
#if WITH_EDITORONLY_DATA
        NewTexture->CompressionNone			= true;
        NewTexture->MipGenSettings			= TMGS_NoMipmaps;
        NewTexture->CompressionNoAlpha		= true;
        NewTexture->DeferCompression		= false;
#endif // #if WITH_EDITORONLY_DATA
        if ( CreateInfo.bIsResolveTarget )
        {
            NewTexture->bNoTiling			= false;
        }
        else
        {
            // Untiled format
            NewTexture->bNoTiling			= true;
        }

        NewTexture->Init(InSizeX, InSizeY, DesiredFormat, CreateInfo.bIsResolveTarget);
    }
    return NewTexture;
}

UTexture2DDynamic* FCustomShaderUtility::CreateTexture2DDynamic(int32 InSizeX, int32 InSizeY, EPixelFormat InFormat, UObject* InOuter, const FName InName)
{
    UTexture2DDynamic* Texture = CreateTexture2DDynamicInternal(InSizeX, InSizeY, InFormat, InOuter, InName);
    Texture->SRGB = true;
    Texture->Format = InFormat;
    Texture->UpdateResource();
    
    return Texture;
}

bool FCustomShaderUtility::ReadPixels(UTextureRenderTarget2D* renderTargetTex, TArray<FColor>& bitmapOut)
{
    FTextureRenderTargetResource* renderTargetResource = renderTargetTex->GameThread_GetRenderTargetResource();
    if (renderTargetResource)
    {
        bool bRead = renderTargetResource->ReadPixels(bitmapOut, ReadPixelFlags);
        return bRead;
    }
    return false;
}

void FCustomShaderUtility::MakeFOVProjectionClippingMatrix(const FIntPoint& res, float FOVYaw, float InNearClippingPlane, float InFarClippingPlane, FMatrix& ProjectionMatrix)
{
    ProjectionMatrix = FReverseZClipPerspectiveMatrix(
       FOVYaw,
       res.X,
       res.Y,
       InNearClippingPlane,
       InFarClippingPlane
       );
}
