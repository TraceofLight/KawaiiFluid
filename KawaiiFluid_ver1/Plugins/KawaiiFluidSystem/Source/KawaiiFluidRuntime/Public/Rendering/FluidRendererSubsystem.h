// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FluidRenderingParameters.h"
#include "FluidRendererSubsystem.generated.h"

class AFluidSimulator;
class FFluidSceneViewExtension;

/**
 * 유체 렌더링 월드 서브시스템
 * 월드별로 SSFR 렌더링 파이프라인 관리
 */
UCLASS()
class KAWAIIFLUIDRUNTIME_API UFluidRendererSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// End of USubsystem interface

	/** 시뮬레이터 등록 */
	void RegisterSimulator(AFluidSimulator* Simulator);

	/** 시뮬레이터 해제 */
	void UnregisterSimulator(AFluidSimulator* Simulator);

	/** 등록된 모든 시뮬레이터 */
	const TArray<AFluidSimulator*>& GetRegisteredSimulators() const { return RegisteredSimulators; }

	/** 글로벌 렌더링 파라미터 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Rendering")
	FFluidRenderingParameters RenderingParameters;

	/** View Extension 접근자 */
	TSharedPtr<FFluidSceneViewExtension, ESPMode::ThreadSafe> GetViewExtension() const { return ViewExtension; }

private:
	/** 등록된 시뮬레이터들 */
	UPROPERTY()
	TArray<AFluidSimulator*> RegisteredSimulators;

	/** Scene View Extension (렌더링 파이프라인 인젝션) */
	TSharedPtr<FFluidSceneViewExtension, ESPMode::ThreadSafe> ViewExtension;
};
