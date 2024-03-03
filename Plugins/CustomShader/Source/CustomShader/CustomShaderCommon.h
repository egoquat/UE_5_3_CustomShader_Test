#pragma once

struct FVFXTracePayload
{
    float HitT;
    uint32 PrimitiveIndex;
    uint32 InstanceIndex;
    float Barycentrics[2];
    float WorldPosition[3];
    float WorldNormal[3];
};

struct FConstCustomRay
{
    inline static const float FOV = 90.0f;
    inline static const float FarClipView = 20000.0f;
};


