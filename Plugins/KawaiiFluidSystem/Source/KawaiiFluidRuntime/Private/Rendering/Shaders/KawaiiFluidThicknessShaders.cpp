// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "Rendering/Shaders/KawaiiFluidThicknessShaders.h"

IMPLEMENT_GLOBAL_SHADER(FKawaiiFluidThicknessVS, "/Plugin/KawaiiFluidSystem/Private/Rendering/KawaiiFluidRenderThickness.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FKawaiiFluidThicknessPS, "/Plugin/KawaiiFluidSystem/Private/Rendering/KawaiiFluidRenderThickness.usf", "MainPS", SF_Pixel);
