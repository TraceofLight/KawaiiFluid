// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "Rendering/Shaders/KawaiiFluidCompositeShaders.h"

IMPLEMENT_GLOBAL_SHADER(FKawaiiFluidCompositeVS, "/Plugin/KawaiiFluidSystem/Private/Rendering/KawaiiFluidRenderComposite.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FKawaiiFluidCompositePS, "/Plugin/KawaiiFluidSystem/Private/Rendering/KawaiiFluidRenderComposite.usf", "MainPS", SF_Pixel);

