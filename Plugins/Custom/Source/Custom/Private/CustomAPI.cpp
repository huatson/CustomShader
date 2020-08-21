// Copyright Epic Games, Inc. All Rights Reserved.

#include "CustomAPI.h"


#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"
#include "Runtime/Engine/Classes/Engine/World.h"

#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "SceneUtils.h"
#include "SceneInterface.h"
#include "ShaderParameterUtils.h"
#include "Logging/MessageLog.h"
#include "Internationalization/Internationalization.h"




#define LOCTEXT_NAMESPACE "CustomShaderPlugin"



IMPLEMENT_SHADER_TYPE(, FCustomShaderVS, TEXT("/Plugin/Custom/Private/MyShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FCustomShaderPS, TEXT("/Plugin/Custom/Private/MyShader.usf"), TEXT("MainPS"), SF_Pixel)


//DrawUVDisplacementToRenderTarget_RenderThread
static void CustomRenderTarget_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FCompiledCameraModel& CompiledCameraModel,
	const FName& TextureRenderTargetName,
	FTextureRenderTargetResource* OutTextureRenderTargetResource,
	ERHIFeatureLevel::Type FeatureLevel)
{
	check(IsInRenderingThread());

#if WANTS_DRAW_MESH_EVENTS
	FString EventName;
	TextureRenderTargetName.ToString(EventName);
	SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("LensDistortionDisplacementGeneration %s"), *EventName);
#else
	SCOPED_DRAW_EVENT(RHICmdList, CustomRenderTarget_RenderThread);
#endif

	FRHITexture2D* RenderTargetTexture = OutTextureRenderTargetResource->GetRenderTargetTexture();

	RHICmdList.TransitionResource(EResourceTransitionAccess::EWritable, RenderTargetTexture);

	FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::DontLoad_Store, OutTextureRenderTargetResource->TextureRHI);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawUVDisplacement"));
	{
		FIntPoint DisplacementMapResolution(OutTextureRenderTargetResource->GetSizeX(), OutTextureRenderTargetResource->GetSizeY());

		// Update viewport.
		RHICmdList.SetViewport(
			0, 0, 0.f,
			DisplacementMapResolution.X, DisplacementMapResolution.Y, 1.f);

		// Get shaders.
		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
		TShaderMapRef< FCustomShaderVS > VertexShader(GlobalShaderMap);
		TShaderMapRef< FCustomShaderPS > PixelShader(GlobalShaderMap);

		// Set the graphic pipeline state.
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

		// Update viewport.
		RHICmdList.SetViewport(
			0, 0, 0.f,
			OutTextureRenderTargetResource->GetSizeX(), OutTextureRenderTargetResource->GetSizeY(), 1.f);

		// Update shader uniform parameters.
		VertexShader->SetParameters(RHICmdList, VertexShader.GetVertexShader(), CompiledCameraModel, DisplacementMapResolution);
		PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), CompiledCameraModel, DisplacementMapResolution);

		// Draw grid.
		uint32 PrimitiveCount = kGridSubdivisionX * kGridSubdivisionY * 2;
		RHICmdList.DrawPrimitive(0, PrimitiveCount, 1);
	}
	RHICmdList.EndRenderPass();
}


FVector2D FCustomCameraModel::CustomViewPosition(FVector2D EngineV) const
{
	// Engine view space -> standard view space.
	FVector2D V = FVector2D(1, -1) * EngineV;

	FVector2D V2 = V * V;
	float R2 = V2.X + V2.Y;

	// Radial distortion (extra parenthesis to match MF_Undistortion.uasset).
	FVector2D UndistortedV = V * (1.0 + (R2 * K1 + (R2 * R2) * K2 + (R2 * R2 * R2) * K3));

	// Tangential distortion.
	UndistortedV.X += P2 * (R2 + 2 * V2.X) + 2 * P1 * V.X * V.Y;
	UndistortedV.Y += P1 * (R2 + 2 * V2.Y) + 2 * P2 * V.X * V.Y;

	// Returns engine V.
	return UndistortedV * FVector2D(1, -1);
}

//LensUndistortViewportUVIntoViewSpace
/** Undistorts top left originated viewport UV into the view space (x', y', z'=1.f) */
static FVector2D CustomUVIntoViewSpace(
	const FCustomCameraModel& CameraModel,
	float TanHalfDistortedHorizontalFOV, float DistortedAspectRatio,
	FVector2D DistortedViewportUV)
{
	FVector2D AspectRatioAwareF = CameraModel.F * FVector2D(1, -DistortedAspectRatio);
	return CameraModel.CustomViewPosition((DistortedViewportUV - CameraModel.C) / AspectRatioAwareF);
}


/** Compiles the camera model. */
float FCustomCameraModel::CustomOverscanFactor(
	float DistortedHorizontalFOV, float DistortedAspectRatio) const
{
	// If the lens distortion model is identity, then early return 1.
	if (*this == FCustomCameraModel())
	{
		return 1.0f;
	}

	float TanHalfDistortedHorizontalFOV = FMath::Tan(DistortedHorizontalFOV * 0.5f);

	// Get the position in the view space at z'=1 of different key point in the distorted Viewport UV coordinate system.
	// This very approximative to know the required overscan scale factor of the undistorted viewport, but works really well in practice.
	//
	//  Undistorted UV position in the view space:
	//                 ^ View space's Y
	//                 |
	//        0        1        2
	//     
	//        7        0        3 --> View space's X
	//     
	//        6        5        4
	FVector2D UndistortCornerPos0 = CustomUVIntoViewSpace(
		*this, TanHalfDistortedHorizontalFOV, DistortedAspectRatio, FVector2D(0.0f, 0.0f));
	FVector2D UndistortCornerPos1 = CustomUVIntoViewSpace(
		*this, TanHalfDistortedHorizontalFOV, DistortedAspectRatio, FVector2D(0.5f, 0.0f));
	FVector2D UndistortCornerPos2 = CustomUVIntoViewSpace(
		*this, TanHalfDistortedHorizontalFOV, DistortedAspectRatio, FVector2D(1.0f, 0.0f));
	FVector2D UndistortCornerPos3 = CustomUVIntoViewSpace(
		*this, TanHalfDistortedHorizontalFOV, DistortedAspectRatio, FVector2D(1.0f, 0.5f));
	FVector2D UndistortCornerPos4 = CustomUVIntoViewSpace(
		*this, TanHalfDistortedHorizontalFOV, DistortedAspectRatio, FVector2D(1.0f, 1.0f));
	FVector2D UndistortCornerPos5 = CustomUVIntoViewSpace(
		*this, TanHalfDistortedHorizontalFOV, DistortedAspectRatio, FVector2D(0.5f, 1.0f));
	FVector2D UndistortCornerPos6 = CustomUVIntoViewSpace(
		*this, TanHalfDistortedHorizontalFOV, DistortedAspectRatio, FVector2D(0.0f, 1.0f));
	FVector2D UndistortCornerPos7 = CustomUVIntoViewSpace(
		*this, TanHalfDistortedHorizontalFOV, DistortedAspectRatio, FVector2D(0.0f, 0.5f));

	// Find min and max of the inner square of undistorted Viewport in the view space at z'=1.
	FVector2D MinInnerViewportRect;
	FVector2D MaxInnerViewportRect;
	MinInnerViewportRect.X = FMath::Max3(UndistortCornerPos0.X, UndistortCornerPos6.X, UndistortCornerPos7.X);
	MinInnerViewportRect.Y = FMath::Max3(UndistortCornerPos4.Y, UndistortCornerPos5.Y, UndistortCornerPos6.Y);
	MaxInnerViewportRect.X = FMath::Min3(UndistortCornerPos2.X, UndistortCornerPos3.X, UndistortCornerPos4.X);
	MaxInnerViewportRect.Y = FMath::Min3(UndistortCornerPos0.Y, UndistortCornerPos1.Y, UndistortCornerPos2.Y);

	check(MinInnerViewportRect.X < 0.f);
	check(MinInnerViewportRect.Y < 0.f);
	check(MaxInnerViewportRect.X > 0.f);
	check(MaxInnerViewportRect.Y > 0.f);

	// Compute tan(VerticalFOV * 0.5)
	float TanHalfDistortedVerticalFOV = TanHalfDistortedHorizontalFOV / DistortedAspectRatio;

	// Compute the required undistorted viewport scale on each axes.
	FVector2D ViewportScaleUpFactorPerViewAxis = 0.5 * FVector2D(
		TanHalfDistortedHorizontalFOV / FMath::Max(-MinInnerViewportRect.X, MaxInnerViewportRect.X),
		TanHalfDistortedVerticalFOV / FMath::Max(-MinInnerViewportRect.Y, MaxInnerViewportRect.Y));

	// Scale up by 2% more the undistorted viewport size in the view space to work
	// around the fact that odd undistorted positions might not exactly be at the minimal
	// in case of a tangential distorted barrel lens distortion.
	const float ViewportScaleUpConstMultiplier = 1.02f;
	return FMath::Max(ViewportScaleUpFactorPerViewAxis.X, ViewportScaleUpFactorPerViewAxis.Y) * ViewportScaleUpConstMultiplier;
}


void FCustomCameraModel::CustomRenderTarget(
	UWorld* World,
	float DistortedHorizontalFOV,
	float DistortedAspectRatio,
	float UndistortOverscanFactor,
	UTextureRenderTarget2D* OutputRenderTarget,
	float OutputMultiply,
	float OutputAdd) const
{
	check(IsInGameThread());

	if (!OutputRenderTarget)
	{
		FMessageLog("Blueprint").Warning(
			LOCTEXT("LensDistortionCameraModel_DrawUVDisplacementToRenderTarget",
			"DrawUVDisplacementToRenderTarget: Output render target is required."));
		return;
	}

	// Compiles the camera model to know the overscan scale factor.
	float TanHalfUndistortedHorizontalFOV = FMath::Tan(DistortedHorizontalFOV * 0.5f) * UndistortOverscanFactor;
	float TanHalfUndistortedVerticalFOV = TanHalfUndistortedHorizontalFOV / DistortedAspectRatio;

	// Output.
	FCompiledCameraModel CompiledCameraModel;
	CompiledCameraModel.OriginalCameraModel = *this;

	CompiledCameraModel.DistortedCameraMatrix.X = 1.0f / TanHalfUndistortedHorizontalFOV;
	CompiledCameraModel.DistortedCameraMatrix.Y = 1.0f / TanHalfUndistortedVerticalFOV;
	CompiledCameraModel.DistortedCameraMatrix.Z = 0.5f;
	CompiledCameraModel.DistortedCameraMatrix.W = 0.5f;

	CompiledCameraModel.UndistortedCameraMatrix.X = F.X;
	CompiledCameraModel.UndistortedCameraMatrix.Y = F.Y * DistortedAspectRatio;
	CompiledCameraModel.UndistortedCameraMatrix.Z = C.X;
	CompiledCameraModel.UndistortedCameraMatrix.W = C.Y;

	CompiledCameraModel.OutputMultiplyAndAdd.X = OutputMultiply;
	CompiledCameraModel.OutputMultiplyAndAdd.Y = OutputAdd;

	const FName TextureRenderTargetName = OutputRenderTarget->GetFName();
	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();

	ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();

	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[CompiledCameraModel, TextureRenderTargetResource, TextureRenderTargetName, FeatureLevel](FRHICommandListImmediate& RHICmdList)
		{
			CustomRenderTarget_RenderThread(
				RHICmdList,
				CompiledCameraModel,
				TextureRenderTargetName,
				TextureRenderTargetResource,
				FeatureLevel);
		}
	);
}

#undef LOCTEXT_NAMESPACE
