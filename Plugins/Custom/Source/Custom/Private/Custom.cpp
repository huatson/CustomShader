// Copyright Epic Games, Inc. All Rights Reserved.


#include "Custom.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

DEFINE_LOG_CATEGORY(CustomPluginLog)

#define LOCTEXT_NAMESPACE "FCustomModule"

void FCustomModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString basedir = IPluginManager::Get().FindPlugin(TEXT("Custom"))->GetBaseDir();
	//basedir ==> C:/UE4/CustomShader/Plugins/Custom
	UE_LOG(CustomPluginLog, Warning, TEXT("Base directory: %s"), *basedir);
	FString PluginShaderDir = FPaths::Combine(basedir, TEXT("Shaders"));
	UE_LOG(CustomPluginLog, Warning, TEXT("Plugin directory: %s"), *PluginShaderDir);
	// VirtualShaderDirectory, RealShaderDirectory
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/Custom"), PluginShaderDir);
}

void FCustomModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCustomModule, Custom)