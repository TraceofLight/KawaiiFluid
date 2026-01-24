// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "Rendering/Shaders/SDFBakeShaders.h"

IMPLEMENT_GLOBAL_SHADER(FSDFBakeCS,
	"/Plugin/KawaiiFluidSystem/Private/SDFBake.usf",
	"SDFBakeCS",
	SF_Compute);

IMPLEMENT_GLOBAL_SHADER(FSDFBakeWithGPUBoundsCS,
	"/Plugin/KawaiiFluidSystem/Private/SDFBakeWithGPUBounds.usf",
	"SDFBakeWithGPUBoundsCS",
	SF_Compute);
