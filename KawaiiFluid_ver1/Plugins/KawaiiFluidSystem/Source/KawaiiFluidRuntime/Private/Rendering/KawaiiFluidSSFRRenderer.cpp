// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Rendering/KawaiiFluidSSFRRenderer.h"
#include "Interfaces/IKawaiiFluidDataProvider.h"
#include "Rendering/FluidRendererSubsystem.h"

UKawaiiFluidSSFRRenderer::UKawaiiFluidSSFRRenderer()
{
	// No component tick needed - UObject doesn't tick
}

void UKawaiiFluidSSFRRenderer::Initialize(UWorld* InWorld, AActor* InOwner)
{
	CachedWorld = InWorld;
	CachedOwner = InOwner;

	if (!CachedWorld)
	{
		UE_LOG(LogTemp, Warning, TEXT("KawaiiFluidSSFRRenderer::Initialize - No world context provided"));
	}

	if (!CachedOwner)
	{
		UE_LOG(LogTemp, Warning, TEXT("KawaiiFluidSSFRRenderer::Initialize - No owner actor provided"));
	}

	// Cache renderer subsystem for ViewExtension access
	if (CachedWorld)
	{
		RendererSubsystem = CachedWorld->GetSubsystem<UFluidRendererSubsystem>();

		if (!RendererSubsystem)
		{
			UE_LOG(LogTemp, Warning, TEXT("KawaiiFluidSSFRRenderer: Failed to get FluidRendererSubsystem"));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("KawaiiFluidSSFRRenderer: Initialized (FluidColor: %s, MaxParticles: %d)"),
		*FluidColor.ToString(),
		MaxRenderParticles);
}

void UKawaiiFluidSSFRRenderer::Cleanup()
{
	// Clear cached data
	CachedParticlePositions.Empty();
	RendererSubsystem = nullptr;
	bIsRenderingActive = false;

	// Clear cached references
	CachedWorld = nullptr;
	CachedOwner = nullptr;
	bEnabled = false;
}

void UKawaiiFluidSSFRRenderer::ApplySettings(const FKawaiiFluidSSFRRendererSettings& Settings)
{
	bEnabled = Settings.bEnabled;
	FluidColor = Settings.FluidColor;
	Metallic = Settings.Metallic;
	Roughness = Settings.Roughness;
	RefractiveIndex = Settings.RefractiveIndex;
	MaxRenderParticles = Settings.MaxRenderParticles;
	DepthBufferScale = Settings.DepthBufferScale;
	bUseThicknessBuffer = Settings.bUseThicknessBuffer;
	DepthSmoothingIterations = Settings.DepthSmoothingIterations;
	FilterRadius = Settings.FilterRadius;
	SurfaceTension = Settings.SurfaceTension;
	FoamThreshold = Settings.FoamThreshold;
	FoamColor = Settings.FoamColor;
	bShowDebugVisualization = Settings.bShowDebugVisualization;
	bShowRenderTargets = Settings.bShowRenderTargets;
}

void UKawaiiFluidSSFRRenderer::UpdateRendering(const IKawaiiFluidDataProvider* DataProvider, float DeltaTime)
{
	if (!bEnabled || !DataProvider)
	{
		bIsRenderingActive = false;
		return;
	}

	// Get simulation data from DataProvider
	const TArray<FFluidParticle>& SimParticles = DataProvider->GetParticles();

	if (SimParticles.Num() == 0)
	{
		bIsRenderingActive = false;
		LastRenderedParticleCount = 0;
		return;
	}

	// Limit number of particles to render
	int32 NumParticles = FMath::Min(SimParticles.Num(), MaxRenderParticles);

	// Get particle radius
	float ParticleRadius = DataProvider->GetParticleRenderRadius();

	// Update GPU resources
	UpdateGPUResources(SimParticles, ParticleRadius);

	// Execute SSFR pipeline (via ViewExtension)
	ExecuteSSFRPipeline();

	// Update stats
	LastRenderedParticleCount = NumParticles;
	bIsRenderingActive = true;
}

void UKawaiiFluidSSFRRenderer::UpdateGPUResources(const TArray<FFluidParticle>& Particles, float ParticleRadius)
{
	// Limit particle count
	int32 NumParticles = FMath::Min(Particles.Num(), MaxRenderParticles);

	// Update position data cache
	CachedParticlePositions.SetNum(NumParticles);
	for (int32 i = 0; i < NumParticles; ++i)
	{
		CachedParticlePositions[i] = Particles[i].Position;
	}

	CachedParticleRadius = ParticleRadius;

	// TODO: Upload to GPU buffers
	// - Structured Buffer for particle positions
	// - Constant Buffer for rendering parameters
	// - Interface with ViewExtension for GPU rendering
}

void UKawaiiFluidSSFRRenderer::ExecuteSSFRPipeline()
{
	// TODO: Trigger SSFR rendering via ViewExtension
	// - Pass particle data to ViewExtension
	// - Execute depth pass
	// - Execute thickness pass (if enabled)
	// - Execute smoothing/filtering passes
	// - Execute surface reconstruction
	// - Composite with scene

	// Note: Actual implementation requires ViewExtension integration
	// which communicates with the cached RendererSubsystem
}
