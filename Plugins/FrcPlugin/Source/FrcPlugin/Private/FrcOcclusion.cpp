
#include "FrcOcclusion.h"

#define LOCTEXT_NAMESPACE "FrcPlugin"

// default, non-instanced shader implementation
IMPLEMENT_SHADER_TYPE(, FFrcOcclusionVS, TEXT("/Plugin/FrcPlugin/Private/OcclusionQueryVS.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FFrcOcclusionPS, TEXT("/Plugin/FrcPlugin/Private/OcclusionQueryPS.usf"), TEXT("MainPS"), SF_Pixel);

//IMPLEMENT_GLOBAL_SHADER(FFrcOcclusionVS, TEXT("/Plugin/FrcPlugin/Private/OcclusionQueryVS.usf"), TEXT("MainVS"), SF_Vertex);
//IMPLEMENT_GLOBAL_SHADER(FFrcOcclusionPS, TEXT("/Plugin/FrcPlugin/Private/OcclusionQueryPS.usf"), TEXT("MainPS"), SF_Pixel);

#undef LOCTEXT_NAMESPACE