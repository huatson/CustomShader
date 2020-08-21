// Copyright Epic Games, Inc. All Rights Reserved.

#include "FrcPlugin.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

DEFINE_LOG_CATEGORY(FrcPluginLog)

#define LOCTEXT_NAMESPACE "FFrcPluginModule"

void FFrcPluginModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString basedir = IPluginManager::Get().FindPlugin(TEXT("FrcPlugin"))->GetBaseDir();
	UE_LOG(FrcPluginLog, Warning, TEXT("Base directory: %s"), *basedir);
	FString RealShaderDirectory = FPaths::Combine(basedir, TEXT("Shaders"));
	UE_LOG(FrcPluginLog, Warning, TEXT("Plugin directory: %s"), *RealShaderDirectory);
	//								VirtualShaderDirectory,		RealShaderDirectory
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/FrcPlugin"), RealShaderDirectory);
}

void FFrcPluginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UE_LOG(FrcPluginLog, Warning, TEXT("Unloading FrcPlugin"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFrcPluginModule, FrcPlugin)