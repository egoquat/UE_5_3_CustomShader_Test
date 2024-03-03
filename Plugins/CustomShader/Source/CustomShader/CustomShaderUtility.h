
#pragma once

class FClipPerspectiveMatrix : public FMatrix
{
public:
    FClipPerspectiveMatrix(float HalfFOVX, float width, float height, float NearPlane, float FarPlane);
};

FORCEINLINE FClipPerspectiveMatrix::FClipPerspectiveMatrix(float HalfFOVX, float width, float height, float NearPlane, float FarPlane)
    : FMatrix(
        FPlane(1.0f / FMath::Tan(HalfFOVX), 0.0f,	0.0f,										0.0f),
        FPlane(0.0f,							    1.0f / FMath::Tan(HalfFOVX) / (height / width),	    0.0f,													0.0f),
        FPlane(0.0f,							    0.0f,								FarPlane / (FarPlane - NearPlane),			1.0f),
        FPlane(0.0f,							    0.0f,								(NearPlane * FarPlane) / (FarPlane - NearPlane),	0.0f)
    )
{
}

class FReverseZClipPerspectiveMatrix : public FMatrix
{
public:
    FReverseZClipPerspectiveMatrix(float HalfFOVX, float w, float h, float Near, float Far);
};

// 참고 : https://tomhultonharrop.com/mathematics/graphics/2023/08/06/reverse-z.html
FORCEINLINE FReverseZClipPerspectiveMatrix::FReverseZClipPerspectiveMatrix(
                                                        float HalfFOVX, float w, float h, float Near, float Far)
{
    FMatrix perspective (
        FPlane(1.0f / FMath::Tan(HalfFOVX), 0.0f,					             0.0f,						  0.0f),
        FPlane(0.0f,						1.0f / FMath::Tan(HalfFOVX) / (h/w), 0.0f,						  0.0f),
        FPlane(0.0f,						0.0f,								 Far / (Far - Near),		  1.0f),
        FPlane(0.0f,						0.0f,								 (Near * Far) / (Far - Near), 0.0f));
        
    FMatrix reversez_normalize {    FPlane(1.0f, 0.0f, 0.0f, 0.0f),
                                    FPlane(0.0f, 1.0f, 0.0f, 0.0f),
                                    FPlane(0.0f, 0.0f, 0.5f, 0.0f),
                                    FPlane(0.0f, 0.0f, 0.5f, 1.0f)};

    FMatrix Result;
    VectorMatrixMultiply( &Result, &perspective, &reversez_normalize );
    FMemory::Memcpy(this, &Result, sizeof(*this));
}

struct FCustomShaderUtility
{
    static UTextureRenderTarget2D* MakeRT(const AActor* actorOwned, int resX, int resY);
    static UTexture2D* CreateTexture2D(int32 InSizeX, int32 InSizeY, UObject* InOuter, EPixelFormat InFormat, const FName InName);
    static UTexture2DDynamic* CreateTexture2DDynamicInternal(int32 InSizeX, int32 InSizeY, EPixelFormat InFormat, UObject* InOuter, const FName InName = NAME_None);
    static UTexture2DDynamic* CreateTexture2DDynamic(int32 InSizeX, int32 InSizeY, EPixelFormat InFormat, UObject* InOuter, const FName InName = NAME_None);
    
    static bool ReadPixels(UTextureRenderTarget2D* renderTargetTex, TArray<FColor>& bitmapOut);

    static void MakeFOVProjectionClippingMatrix(const FIntPoint& res, float FOVYaw, float InNearClippingPlane, float InFarClippingPlane, FMatrix& ProjectionMatrix);
};
