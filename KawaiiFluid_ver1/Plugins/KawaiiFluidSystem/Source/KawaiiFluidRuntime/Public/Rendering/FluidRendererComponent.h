// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FluidRenderingParameters.h"
#include "FluidRendererComponent.generated.h"

class AFluidSimulator;

/**
 * 유체 렌더러 컴포넌트
 * FluidSimulator에 붙여서 SSFR 렌더링 활성화
 */
UCLASS(ClassGroup=(KawaiiFluid), meta=(BlueprintSpawnableComponent))
class KAWAIIFLUIDRUNTIME_API UFluidRendererComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFluidRendererComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 렌더링 파라미터 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Rendering")
	FFluidRenderingParameters RenderingParameters;

	/** 이 컴포넌트에만 적용되는 로컬 파라미터 사용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Rendering")
	bool bUseLocalParameters = true;

	/** 부모 시뮬레이터 캐싱 */
	UPROPERTY()
	AFluidSimulator* OwnerSimulator;

	/** 렌더링 파라미터 가져오기 (로컬 또는 글로벌) */
	UFUNCTION(BlueprintCallable, Category = "Fluid Rendering")
	FFluidRenderingParameters GetEffectiveRenderingParameters() const;

	/** 렌더링 품질 변경 */
	UFUNCTION(BlueprintCallable, Category = "Fluid Rendering")
	void SetRenderingQuality(EFluidRenderingQuality Quality);

	/** 렌더링 활성화/비활성화 */
	UFUNCTION(BlueprintCallable, Category = "Fluid Rendering")
	void SetRenderingEnabled(bool bEnabled);

private:
	void CacheOwnerSimulator();
};
