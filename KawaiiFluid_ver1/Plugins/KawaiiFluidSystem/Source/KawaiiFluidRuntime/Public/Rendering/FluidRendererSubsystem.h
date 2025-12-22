// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FluidRenderingParameters.h"
#include "FluidRendererSubsystem.generated.h"

class AFluidSimulator;
class FFluidSceneViewExtension;
class IKawaiiFluidRenderable;

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

	//========================================
	// 통합 렌더링 관리 (IKawaiiFluidRenderable)
	//========================================

	/** 렌더링 가능한 액터 등록 (Simulator, TestActor 등) */
	void RegisterRenderable(AActor* Actor);

	/** 렌더링 가능한 액터 해제 */
	void UnregisterRenderable(AActor* Actor);

	/** 등록된 모든 렌더링 가능한 액터 반환 */
	TArray<IKawaiiFluidRenderable*> GetAllRenderables() const;

	//========================================
	// 레거시 호환성 (기존 코드 지원)
	//========================================

	/** 시뮬레이터 등록 (내부적으로 RegisterRenderable 호출) */
	void RegisterSimulator(AFluidSimulator* Simulator);

	/** 시뮬레이터 해제 (내부적으로 UnregisterRenderable 호출) */
	void UnregisterSimulator(AFluidSimulator* Simulator);

	/** 등록된 모든 시뮬레이터 (레거시) */
	const TArray<AFluidSimulator*>& GetRegisteredSimulators() const { return RegisteredSimulators; }

	//========================================
	// 렌더링 파라미터
	//========================================

	/** 글로벌 렌더링 파라미터 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Rendering")
	FFluidRenderingParameters RenderingParameters;

	/** View Extension 접근자 */
	TSharedPtr<FFluidSceneViewExtension, ESPMode::ThreadSafe> GetViewExtension() const { return ViewExtension; }

private:
	//========================================
	// 통합 저장소
	//========================================

	/** 등록된 렌더링 가능한 모든 액터 (Simulator, TestActor 등) */
	UPROPERTY()
	TArray<TScriptInterface<IKawaiiFluidRenderable>> RegisteredRenderables;

	//========================================
	// 레거시 저장소 (호환성 유지)
	//========================================

	/** 등록된 시뮬레이터들 (레거시) */
	UPROPERTY()
	TArray<AFluidSimulator*> RegisteredSimulators;

	/** Scene View Extension (렌더링 파이프라인 인젝션) */
	TSharedPtr<FFluidSceneViewExtension, ESPMode::ThreadSafe> ViewExtension;
};
