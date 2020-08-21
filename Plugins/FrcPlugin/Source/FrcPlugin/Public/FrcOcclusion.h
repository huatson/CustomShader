#pragma once

#include "Shader.h"
#include "GlobalShader.h"
#include "Serialization/MemoryLayout.h"
#include "LightRendering.h"


class FRCPLUGIN_API FFrcOcclusionVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFrcOcclusionVS, Global);

public:

	FFrcOcclusionVS() {}

	FFrcOcclusionVS(const ShaderMetaType::CompiledShaderInitializerType &Initializer)
		: FGlobalShader(Initializer)
	{
		StencilingGeometryParameters.Bind(Initializer.ParameterMap);
		ViewId.Bind(Initializer.ParameterMap, TEXT("ViewId"));
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	void SetParametersWithBoundingSphere(FRHICommandList& RHICmdList, const FViewInfo& View, const FSphere& BoundingSphere)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, RHICmdList.GetBoundVertexShader(), View.ViewUniformBuffer);

		FVector4 StencilingSpherePosAndScale;
		StencilingGeometry::GStencilSphereVertexBuffer.CalcTransform(StencilingSpherePosAndScale, BoundingSphere, View.ViewMatrices.GetPreViewTranslation());
		StencilingGeometryParameters.Set(RHICmdList, this, StencilingSpherePosAndScale);

		if (GEngine && GEngine->StereoRenderingDevice)
		{
			SetShaderValue(RHICmdList, RHICmdList.GetBoundVertexShader(), ViewId, GEngine->StereoRenderingDevice->GetViewIndexForPass(View.StereoPass));
		}
	}

	void SetParameters(FRHICommandList& RHICmdList, const FViewInfo& View)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, RHICmdList.GetBoundVertexShader(), View.ViewUniformBuffer);

		// Don't transform if rendering frustum
		StencilingGeometryParameters.Set(RHICmdList, this, FVector4(0, 0, 0, 1));

		if (GEngine && GEngine->StereoRenderingDevice)
		{
			SetShaderValue(RHICmdList, RHICmdList.GetBoundVertexShader(), ViewId, GEngine->StereoRenderingDevice->GetViewIndexForPass(View.StereoPass));
		}
	}


private:
	LAYOUT_FIELD(FShaderParameter, ViewId)
	//LAYOUT_FIELD(FCustomStenciling, StencilingGeometryParameters)
	//LAYOUT_FIELD(FStencilingGeometryShaderParameters, StencilingGeometryParameters)
	FStencilingGeometryShaderParameters StencilingGeometryParameters;
};

class FRCPLUGIN_API FFrcOcclusionPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFrcOcclusionPS, Global);

public:
	FFrcOcclusionPS(const ShaderMetaType::CompiledShaderInitializerType &Initializer)
		: FGlobalShader(Initializer)
	{
	}

	FFrcOcclusionPS() {}

};