// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Data/KawaiiFluidPresetDataAsset.h"
#include "GPU/GPUFluidSimulatorShaders.h"  // For GPU_MORTON_GRID_AXIS_BITS

UKawaiiFluidPresetDataAsset::UKawaiiFluidPresetDataAsset()
{
	// Default values are set in header
	// Calculate initial derived parameters
	RecalculateDerivedParameters();
}

void UKawaiiFluidPresetDataAsset::RecalculateDerivedParameters()
{
	//========================================
	// Particle Size Parameters
	//========================================

	// Clamp SpacingRatio to valid range
	SpacingRatio = FMath::Clamp(SpacingRatio, 0.1f, 0.7f);

	// ParticleSpacing = SmoothingRadius * SpacingRatio (cm)
	ParticleSpacing = SmoothingRadius * SpacingRatio;

	// Convert spacing to meters for mass calculation
	const float Spacing_m = ParticleSpacing * 0.01f;

	// ParticleMass = RestDensity * d³ (kg)
	// This ensures uniform grid at spacing d achieves RestDensity
	ParticleMass = RestDensity * Spacing_m * Spacing_m * Spacing_m;

	// Ensure minimum mass
	ParticleMass = FMath::Max(ParticleMass, 0.001f);

	// ParticleRadius = Spacing / 2 (cm)
	// Renders particles with slight overlap for continuous fluid appearance
	ParticleRadius = ParticleSpacing * 0.5f;

	// Ensure minimum radius
	ParticleRadius = FMath::Max(ParticleRadius, 0.1f);

	// Estimate neighbor count: N ≈ (4/3)π × (h/d)³ = (4/3)π × (1/SpacingRatio)³
	const float HOverD = 1.0f / SpacingRatio;
	EstimatedNeighborCount = FMath::RoundToInt((4.0f / 3.0f) * PI * HOverD * HOverD * HOverD);

	//========================================
	// Z-Order Sorting Parameters (Auto-calculated)
	//========================================
	// GridAxisBits is a GLOBAL CONSTANT from GPUFluidSimulatorShaders.h
	// All presets use the same value because it's a shader compile-time constant.
	//
	// Formulas:
	//   GridResolution = 2^GridAxisBits (GPU_MORTON_GRID_SIZE)
	//   MortonBits = GridAxisBits × 3
	//   MaxCells = GridResolution³ (GPU_MAX_CELLS)
	//   CellSize = SmoothingRadius (optimal for SPH)
	//   BoundsExtent = GridResolution × CellSize
	//   SimulationBounds = ±BoundsExtent/2

	// Use global shader constant (NOT per-preset editable)
	GridAxisBits = GPU_MORTON_GRID_AXIS_BITS;

	// Grid Resolution = 2^GridAxisBits (from shader constant)
	ZOrderGridResolution = GPU_MORTON_GRID_SIZE;

	// Morton Code bits = GridAxisBits × 3 (X, Y, Z each get GridAxisBits)
	ZOrderMortonBits = GPU_MORTON_GRID_AXIS_BITS * 3;

	// Max Cells = GridResolution³ (from shader constant)
	ZOrderMaxCells = GPU_MAX_CELLS;

	// Cell Size = SmoothingRadius (optimal for SPH neighbor search: CellSize = h)
	ZOrderCellSize = SmoothingRadius;

	// Bounds Extent = GridResolution × CellSize (total simulation domain size per axis)
	ZOrderBoundsExtent = static_cast<float>(ZOrderGridResolution) * ZOrderCellSize;

	// Simulation Bounds = ±BoundsExtent/2 (centered around component origin)
	const float HalfExtent = ZOrderBoundsExtent * 0.5f;
	SimulationBoundsMin = FVector(-HalfExtent, -HalfExtent, -HalfExtent);
	SimulationBoundsMax = FVector(HalfExtent, HalfExtent, HalfExtent);
}

#if WITH_EDITOR
void UKawaiiFluidPresetDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Validate parameters
	if (SmoothingRadius < 1.0f)
	{
		SmoothingRadius = 1.0f;
	}

	// Check which property changed
	const FName PropertyName = PropertyChangedEvent.Property ?
		PropertyChangedEvent.Property->GetFName() : NAME_None;

	// Recalculate derived parameters when relevant properties change
	// - Particle Size: SmoothingRadius, RestDensity, SpacingRatio
	// - Z-Order Sorting: SmoothingRadius affects CellSize and BoundsExtent
	// Note: GridAxisBits is a global constant (GPU_MORTON_GRID_AXIS_BITS), not editable per-preset
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UKawaiiFluidPresetDataAsset, SmoothingRadius) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UKawaiiFluidPresetDataAsset, RestDensity) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UKawaiiFluidPresetDataAsset, SpacingRatio))
	{
		RecalculateDerivedParameters();
	}
}
#endif
