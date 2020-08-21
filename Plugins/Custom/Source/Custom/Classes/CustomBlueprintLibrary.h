// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CustomAPI.h"
#include "CustomBlueprintLibrary.generated.h"


UCLASS(MinimalAPI, meta=(ScriptName="CustomBlueprintLibrary"))
class UCustomBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()


	/** Returns the overscan factor required for the undistort rendering to avoid unrendered distorted pixels. */
	UFUNCTION(BlueprintPure, Category = "CustomShader")
	static void GetCustomOverscanFactor(
		const FCustomCameraModel& CameraModel,
		float DistortedHorizontalFOV,
		float DistortedAspectRatio,
		float& UndistortOverscanFactor);
		
	/** Draws UV displacement map within the output render target.
	 * - Red & green channels hold the distortion displacement;
	 * - Blue & alpha channels hold the undistortion displacement.
	 * @param DistortedHorizontalFOV The desired horizontal FOV in the distorted render.
	 * @param DistortedAspectRatio The desired aspect ratio of the distorted render.
	 * @param UndistortOverscanFactor The factor of the overscan for the undistorted render.
	 * @param OutputRenderTarget The render target to draw to. Don't necessarily need to have same resolution or aspect ratio as distorted render.
	 * @param OutputMultiply The multiplication factor applied on the displacement.
	 * @param OutputAdd Value added to the multiplied displacement before storing into the output render target.
	 */
	UFUNCTION(BlueprintCallable, Category = "CustomShader", meta = (WorldContext = "WorldContextObject"))
	static void CustomDrawRenderTarget(
		const UObject* WorldContextObject,
		const FCustomCameraModel& CameraModel,
		float DistortedHorizontalFOV,
		float DistortedAspectRatio,
		float UndistortOverscanFactor,
		class UTextureRenderTarget2D* OutputRenderTarget,
		float OutputMultiply = 0.5,
		float OutputAdd = 0.5
		);

	/* Returns true if A is equal to B (A == B) */
	UFUNCTION(BlueprintPure, meta=(DisplayName = "Equal (LensDistortionCameraModel)", CompactNodeTitle = "==", Keywords = "== equal"), Category = "CustomShader")
	static bool EqualEqual_CompareLensDistortionModels(
		const FCustomCameraModel& A,
		const FCustomCameraModel& B)
	{
		return A == B;
	}

	/* Returns true if A is not equal to B (A != B) */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "NotEqual (LensDistortionCameraModel)", CompactNodeTitle = "!=", Keywords = "!= not equal"), Category = "CustomShader")
	static bool NotEqual_CompareLensDistortionModels(
		const FCustomCameraModel& A,
		const FCustomCameraModel& B)
	{
		return A != B;
	}
};
