// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomShader.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RHICommandList.h"
#include "RenderGraphBuilder.h"
#include "RenderTargetPool.h"
#include "Runtime/Core/Public/Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FCustomShaderModule"

void FCustomShaderModule::StartupModule()
{
    TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("CustomShader"));
    ensure(plugin);
    FString PluginShaderDir = FPaths::Combine(plugin->GetBaseDir(), TEXT("Shaders/Private"));
    AddShaderSourceDirectoryMapping(TEXT("/CustomShaders"), PluginShaderDir);
}

void FCustomShaderModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCustomShaderModule, CustomShader)