// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Rendering/Shaders/FluidGBufferWriteShaders.h"

IMPLEMENT_GLOBAL_SHADER(FFluidGBufferWriteVS,
	"/Plugin/KawaiiFluidSystem/Private/FluidGBufferWrite.usf",
	"MainVS",
	SF_Vertex);

IMPLEMENT_GLOBAL_SHADER(FFluidGBufferWritePS,
	"/Plugin/KawaiiFluidSystem/Private/FluidGBufferWrite.usf",
	"MainPS",
	SF_Pixel);
