// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "Rendering/FluidThicknessShaders.h"

IMPLEMENT_GLOBAL_SHADER(FFluidThicknessVS, "/Plugin/KawaiiFluidSystem/Private/FluidThickness.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FFluidThicknessPS, "/Plugin/KawaiiFluidSystem/Private/FluidThickness.usf", "MainPS", SF_Pixel);
