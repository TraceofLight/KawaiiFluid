// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Rendering/FluidRendererSubsystem.h"
#include "Rendering/FluidSceneViewExtension.h"
#include "Core/FluidSimulator.h"

void UFluidRendererSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Scene View Extension 생성 및 등록
	ViewExtension = FSceneViewExtensions::NewExtension<FFluidSceneViewExtension>(this);

	UE_LOG(LogTemp, Log, TEXT("FluidRendererSubsystem Initialized"));
}

void UFluidRendererSubsystem::Deinitialize()
{
	// View Extension 해제
	ViewExtension.Reset();

	RegisteredSimulators.Empty();

	Super::Deinitialize();

	UE_LOG(LogTemp, Log, TEXT("FluidRendererSubsystem Deinitialized"));
}

void UFluidRendererSubsystem::RegisterSimulator(AFluidSimulator* Simulator)
{
	if (Simulator && !RegisteredSimulators.Contains(Simulator))
	{
		RegisteredSimulators.Add(Simulator);
		UE_LOG(LogTemp, Log, TEXT("Registered FluidSimulator: %s"), *Simulator->GetName());
	}
}

void UFluidRendererSubsystem::UnregisterSimulator(AFluidSimulator* Simulator)
{
	if (Simulator)
	{
		RegisteredSimulators.Remove(Simulator);
		UE_LOG(LogTemp, Log, TEXT("Unregistered FluidSimulator: %s"), *Simulator->GetName());
	}
}
