// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "CustomAPI.generated.h"

static const uint32 kGridSubdivisionX = 32;
static const uint32 kGridSubdivisionY = 16;

//FLensDistortionCameraModel
/** Mathematic camera model for lens distortion/undistortion.
 *
 * Camera matrix =
 *  | F.X  0  C.x |
 *  |  0  F.Y C.Y |
 *  |  0   0   1  |
 */
USTRUCT(BlueprintType)
struct FCustomCameraModel
{
	GENERATED_USTRUCT_BODY()
	FCustomCameraModel()
	{
		K1 = K2 = K3 = P1 = P2 = 0.f;
		F = FVector2D(1.f, 1.f);
		C = FVector2D(0.5f, 0.5f);
	}


	/** Radial parameter #1. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "CustomCameraModel")
	float K1;

	/** Radial parameter #2. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "CustomCameraModel")
	float K2;

	/** Radial parameter #3. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "CustomCameraModel")
	float K3;

	/** Tangential parameter #1. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "CustomCameraModel")
	float P1;

	/** Tangential parameter #2. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "CustomCameraModel")
	float P2;

	/** Camera matrix's Fx and Fy. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "CustomCameraModel")
	FVector2D F;

	/** Camera matrix's Cx and Cy. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "CustomCameraModel")
	FVector2D C;

	
	//UndistortNormalizedViewPosition
	/** Undistorts 3d vector (x, y, z=1.f) in the view space and returns (x', y', z'=1.f). */
	FVector2D CustomViewPosition(FVector2D V) const;

	
	//GetUndistortOverscanFactor
	/** Returns the overscan factor required for the undistort rendering to avoid unrendered distorted pixels. */
	float CustomOverscanFactor(
		float DistortedHorizontalFOV,
		float DistortedAspectRatio) const;


	/** Draws UV displacement map within the output render target.
	 * - Red & green channels hold the distortion displacement;
	 * - Blue & alpha channels hold the undistortion displacement.
	 * @param World Current world to get the rendering settings from (such as feature level).
	 * @param DistortedHorizontalFOV The desired horizontal FOV in the distorted render.
	 * @param DistortedAspectRatio The desired aspect ratio of the distorted render.
	 * @param UndistortOverscanFactor The factor of the overscan for the undistorted render.
	 * @param OutputRenderTarget The render target to draw to. Don't necessarily need to have same resolution or aspect ratio as distorted render.
	 * @param OutputMultiply The multiplication factor applied on the displacement.
	 * @param OutputAdd Value added to the multiplied displacement before storing into the output render target.
	 */
	// DrawUVDisplacementToRenderTarget
	void CustomRenderTarget(
		class UWorld* World,
		float DistortedHorizontalFOV,
		float DistortedAspectRatio,
		float UndistortOverscanFactor,
		class UTextureRenderTarget2D* OutputRenderTarget,
		float OutputMultiply,
		float OutputAdd) const;


	/** Compare two lens distortion models and return whether they are equal. */
	bool operator == (const FCustomCameraModel& Other) const
	{
		return (
			K1 == Other.K1 &&
			K2 == Other.K2 &&
			K3 == Other.K3 &&
			P1 == Other.P1 &&
			P2 == Other.P2 &&
			F == Other.F &&
			C == Other.C);
	}

	/** Compare two lens distortion models and return whether they are different. */
	bool operator != (const FCustomCameraModel& Other) const
	{
		return !(*this == Other);
	}
};

/**
 * Internal intermediary structure derived from FLensDistortionCameraModel by the game thread
 * to hand to the render thread.
 */
struct FCompiledCameraModel
{
	/** Original camera model that has generated this compiled model. */
	FCustomCameraModel OriginalCameraModel;

	/** Camera matrices of the lens distortion for the undistorted and distorted render.
	 *  XY holds the scales factors, ZW holds the translates.
	 */
	FVector4 DistortedCameraMatrix;
	FVector4 UndistortedCameraMatrix;

	/** Output multiply and add of the channel to the render target. */
	FVector2D OutputMultiplyAndAdd;
};

//FLensDistortionUVGenerationShader
class FCustomShader : public FGlobalShader
{
	DECLARE_INLINE_TYPE_LAYOUT(FCustomShader, NonVirtual);
public:

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("GRID_SUBDIVISION_X"), kGridSubdivisionX);
		OutEnvironment.SetDefine(TEXT("GRID_SUBDIVISION_Y"), kGridSubdivisionY);
	}

	FCustomShader() {}

	FCustomShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PixelUVSize.Bind(Initializer.ParameterMap, TEXT("PixelUVSize"));
		RadialDistortionCoefs.Bind(Initializer.ParameterMap, TEXT("RadialDistortionCoefs"));
		TangentialDistortionCoefs.Bind(Initializer.ParameterMap, TEXT("TangentialDistortionCoefs"));
		DistortedCameraMatrix.Bind(Initializer.ParameterMap, TEXT("DistortedCameraMatrix"));
		UndistortedCameraMatrix.Bind(Initializer.ParameterMap, TEXT("UndistortedCameraMatrix"));
		OutputMultiplyAndAdd.Bind(Initializer.ParameterMap, TEXT("OutputMultiplyAndAdd"));
	}

	template<typename TShaderRHIParamRef>
	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		const TShaderRHIParamRef ShaderRHI,
		const FCompiledCameraModel& CompiledCameraModel,
		const FIntPoint& DisplacementMapResolution)
	{
		FVector2D PixelUVSizeValue(
			1.f / float(DisplacementMapResolution.X), 1.f / float(DisplacementMapResolution.Y));
		FVector RadialDistortionCoefsValue(
			CompiledCameraModel.OriginalCameraModel.K1,
			CompiledCameraModel.OriginalCameraModel.K2,
			CompiledCameraModel.OriginalCameraModel.K3);
		FVector2D TangentialDistortionCoefsValue(
			CompiledCameraModel.OriginalCameraModel.P1,
			CompiledCameraModel.OriginalCameraModel.P2);

		SetShaderValue(RHICmdList, ShaderRHI, PixelUVSize, PixelUVSizeValue);
		SetShaderValue(RHICmdList, ShaderRHI, DistortedCameraMatrix, CompiledCameraModel.DistortedCameraMatrix);
		SetShaderValue(RHICmdList, ShaderRHI, UndistortedCameraMatrix, CompiledCameraModel.UndistortedCameraMatrix);
		SetShaderValue(RHICmdList, ShaderRHI, RadialDistortionCoefs, RadialDistortionCoefsValue);
		SetShaderValue(RHICmdList, ShaderRHI, TangentialDistortionCoefs, TangentialDistortionCoefsValue);
		SetShaderValue(RHICmdList, ShaderRHI, OutputMultiplyAndAdd, CompiledCameraModel.OutputMultiplyAndAdd);
	}

private:

	LAYOUT_FIELD(FShaderParameter, PixelUVSize);
	LAYOUT_FIELD(FShaderParameter, RadialDistortionCoefs);
	LAYOUT_FIELD(FShaderParameter, TangentialDistortionCoefs);
	LAYOUT_FIELD(FShaderParameter, DistortedCameraMatrix);
	LAYOUT_FIELD(FShaderParameter, UndistortedCameraMatrix);
	LAYOUT_FIELD(FShaderParameter, OutputMultiplyAndAdd);
};


//FLensDistortionUVGenerationShader
//FLensDistortionUVGenerationVS
class FCustomShaderVS : public FCustomShader
{
	DECLARE_SHADER_TYPE(FCustomShaderVS, Global);
public:

	/** Default constructor. */
	FCustomShaderVS() {}

	/** Initialization constructor. */
	FCustomShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FCustomShader(Initializer)
	{
	}
};

//FLensDistortionUVGenerationPS
class FCustomShaderPS : public FCustomShader
{
	DECLARE_SHADER_TYPE(FCustomShaderPS, Global);
public:

	/** Default constructor. */
	FCustomShaderPS() {}

	/** Initialization constructor. */
	FCustomShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FCustomShader(Initializer)
	{ }
};

