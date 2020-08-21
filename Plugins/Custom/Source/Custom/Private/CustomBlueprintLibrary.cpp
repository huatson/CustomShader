// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomBlueprintLibrary.h"



UCustomBlueprintLibrary::UCustomBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{ }


// static
void UCustomBlueprintLibrary::GetCustomOverscanFactor(
	const FCustomCameraModel& CameraModel,
	float DistortedHorizontalFOV,
	float DistortedAspectRatio,
	float& UndistortOverscanFactor)
{
	UndistortOverscanFactor = CameraModel.CustomOverscanFactor(DistortedHorizontalFOV, DistortedAspectRatio);
}


// static
void UCustomBlueprintLibrary::CustomDrawRenderTarget(
	const UObject* WorldContextObject,
	const FCustomCameraModel& CameraModel,
	float DistortedHorizontalFOV,
	float DistortedAspectRatio,
	float UndistortOverscanFactor,
	class UTextureRenderTarget2D* OutputRenderTarget,
	float OutputMultiply,
	float OutputAdd)
{
	CameraModel.CustomRenderTarget(
		WorldContextObject->GetWorld(),
		DistortedHorizontalFOV, DistortedAspectRatio,
		UndistortOverscanFactor, OutputRenderTarget,
		OutputMultiply, OutputAdd);
}
