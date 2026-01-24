// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

class UFluidRendererSubsystem;
struct FPostProcessingInputs;

/**
 * SSFR 렌더링 파이프라인 인젝션을 위한 Scene View Extension
 * 언리얼 렌더링 파이프라인에 커스텀 렌더 패스 추가
 */
class KAWAIIFLUIDRUNTIME_API FFluidSceneViewExtension : public FSceneViewExtensionBase
{
public:
	FFluidSceneViewExtension(const FAutoRegister& AutoRegister, UFluidRendererSubsystem* InSubsystem);
	virtual ~FFluidSceneViewExtension() override;

	// ISceneViewExtension interface
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;

	/**
	 * 렌더 스레드에서 ViewFamily 렌더링 전에 호출
	 * GPU 시뮬레이터에서 RenderResource로 데이터 추출 수행
	 */
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;

	/**
	 * PostProcessing Pass 구독
	 * Tonemap: Custom mode (post-lighting)
	 */
	virtual void SubscribeToPostProcessingPass(
		EPostProcessingPass Pass,
		const FSceneView& InView,
		FPostProcessingPassDelegateArray& InOutPassCallbacks,
		bool bIsPassEnabled) override;


	/**
	 * PrePostProcess - called after Lighting, before PostProcessing
	 * All fluid rendering (ScreenSpace, RayMarching) happens here
	 * Both GBuffer and SceneColor are at internal resolution here
	 */
	virtual void PrePostProcessPass_RenderThread(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FPostProcessingInputs& Inputs) override;

	// End of ISceneViewExtension interface

private:
	/** Check if the view belongs to our Subsystem's World */
	bool IsViewFromOurWorld(const FSceneView& InView) const;

	/** Subsystem 약한 참조 */
	TWeakObjectPtr<UFluidRendererSubsystem> Subsystem;
};
