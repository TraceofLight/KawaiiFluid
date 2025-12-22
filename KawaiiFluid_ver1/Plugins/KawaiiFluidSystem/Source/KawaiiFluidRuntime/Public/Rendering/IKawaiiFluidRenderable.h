// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "IKawaiiFluidRenderable.generated.h"

class FKawaiiFluidRenderResource;

/**
 * UInterface (Unreal Reflection System용)
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UKawaiiFluidRenderable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Kawaii 유체 렌더링 가능한 액터가 구현해야 하는 인터페이스
 * Simulator, TestActor 등 모든 유체 렌더링 소스에 사용
 */
class IKawaiiFluidRenderable
{
	GENERATED_BODY()

public:
	/**
	 * GPU 렌더 리소스 반환
	 * @return GPU 버퍼를 포함한 렌더 리소스
	 */
	virtual FKawaiiFluidRenderResource* GetFluidRenderResource() const = 0;

	/**
	 * 렌더 리소스 유효성 확인
	 * @return 렌더링 가능한 상태인지 여부
	 */
	virtual bool IsFluidRenderResourceValid() const = 0;

	/**
	 * 파티클 렌더링 반경 반환 (cm)
	 */
	virtual float GetParticleRenderRadius() const = 0;

	/**
	 * 디버그 이름 반환 (프로파일링용)
	 */
	virtual FString GetDebugName() const = 0;

	//========================================
	// 렌더링 모드 관련
	//========================================

	/**
	 * SSFR 렌더링 사용 여부
	 * @return SSFR 모드 또는 Both 모드일 경우 true
	 */
	virtual bool ShouldUseSSFR() const = 0;

	/**
	 * 디버그 메시 렌더링 사용 여부
	 * @return DebugMesh 모드 또는 Both 모드일 경우 true
	 */
	virtual bool ShouldUseDebugMesh() const = 0;

	/**
	 * 디버그 메시 컴포넌트 반환
	 * @return DebugMesh 모드에서 사용할 InstancedStaticMeshComponent
	 */
	virtual UInstancedStaticMeshComponent* GetDebugMeshComponent() const = 0;

	/**
	 * 파티클 개수 반환
	 * @return 현재 시뮬레이션/테스트 중인 파티클 개수
	 */
	virtual int32 GetParticleCount() const = 0;
};
