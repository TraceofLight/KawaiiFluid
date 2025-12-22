// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/KawaiiRenderParticle.h"
#include "Rendering/IKawaiiFluidRenderable.h"
#include "Rendering/KawaiiFluidRenderingMode.h"
#include "KawaiiFluidDummy.generated.h"

class FKawaiiFluidRenderResource;

/**
 * 테스트 데이터 생성 모드
 */
UENUM(BlueprintType)
enum class ETestDataMode : uint8
{
	Static      UMETA(DisplayName = "Static (고정)"),
	Animated    UMETA(DisplayName = "Animated (애니메이션)"),
	GridPattern UMETA(DisplayName = "Grid Pattern (격자)"),
	Sphere      UMETA(DisplayName = "Sphere (구)"),
	Wave        UMETA(DisplayName = "Wave (파동)")
};

/**
 * 렌더링 테스트용 유체 더미 액터
 * 물리 시뮬레이션 없이 GPU 버퍼 업로드만 수행하여 SSFR 파이프라인 테스트
 */
UCLASS(BlueprintType, HideCategories = (Collision, Physics, LOD, Cooking))
class KAWAIIFLUIDRUNTIME_API AKawaiiFluidDummy : public AActor, public IKawaiiFluidRenderable
{
	GENERATED_BODY()

public:
	AKawaiiFluidDummy();
	virtual ~AKawaiiFluidDummy();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;
	virtual void Tick(float DeltaTime) override;

	//========================================
	// Components
	//========================================

	/** Root 컴포넌트 (에디터에서 이동 가능하도록) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* RootSceneComponent;

	//========================================
	// 테스트 모드 설정
	//========================================

	/** 렌더링 활성화 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Mode")
	bool bEnableRendering = true;

	/** 테스트 데이터 생성 모드 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Mode")
	ETestDataMode DataMode = ETestDataMode::Animated;

	//========================================
	// 파티클 설정
	//========================================

	/** 파티클 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Particles", meta = (ClampMin = "1", ClampMax = "10000"))
	int32 ParticleCount = 500;

	/** 파티클 반경 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Particles", meta = (ClampMin = "1.0", ClampMax = "50.0"))
	float ParticleRadius = 5.0f;

	/** 생성 영역 크기 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Particles")
	FVector SpawnExtent = FVector(100.0f, 100.0f, 100.0f);

	//========================================
	// 애니메이션 설정
	//========================================

	/** 애니메이션 속도 (DataMode가 Animated/Wave일 때) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Animation", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float AnimationSpeed = 1.0f;

	/** 파동 진폭 (Wave 모드, cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Animation", meta = (ClampMin = "1.0", ClampMax = "100.0"))
	float WaveAmplitude = 20.0f;

	/** 파동 주파수 (Wave 모드) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Animation", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float WaveFrequency = 1.0f;

	//========================================
	// 렌더링 방식 선택
	//========================================

	/** 
	 * 렌더링 방식 선택
	 * - DebugMesh: Instanced Static Mesh
	 * - SSFR: Screen Space Fluid Rendering
	 * - Both: 둘 다 (디버그용)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Rendering")
	EKawaiiFluidRenderingMode RenderingMode = EKawaiiFluidRenderingMode::SSFR;

	/** 디버그 메시 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test|Rendering")
	class UInstancedStaticMeshComponent* DebugMeshComponent;

	//========================================
	// 블루프린트 함수
	//========================================

	/** 테스트 데이터 재생성 */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void RegenerateTestData();

	/** GPU 버퍼 강제 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void ForceUpdateGPUBuffer();

	/** 현재 파티클 수 반환 */
	UFUNCTION(BlueprintPure, Category = "Test")
	int32 GetCurrentParticleCount() const { return TestParticles.Num(); }

	//========================================
	// IKawaiiFluidRenderable 인터페이스 구현
	//========================================

	virtual FKawaiiFluidRenderResource* GetFluidRenderResource() const override
	{
		return RenderResource.Get();
	}

	virtual bool IsFluidRenderResourceValid() const override;

	virtual float GetParticleRenderRadius() const override
	{
		return ParticleRadius;
	}

	virtual FString GetDebugName() const override
	{
		return FString::Printf(TEXT("FluidDummy_%s"), *GetName());
	}

	virtual bool ShouldUseSSFR() const override
	{
		return RenderingMode == EKawaiiFluidRenderingMode::SSFR || 
		       RenderingMode == EKawaiiFluidRenderingMode::Both;
	}

	virtual bool ShouldUseDebugMesh() const override
	{
		return RenderingMode == EKawaiiFluidRenderingMode::DebugMesh || 
		       RenderingMode == EKawaiiFluidRenderingMode::Both;
	}

	virtual UInstancedStaticMeshComponent* GetDebugMeshComponent() const override
	{
		return DebugMeshComponent;
	}

	virtual int32 GetParticleCount() const override
	{
		return TestParticles.Num();
	}

private:
	//========================================
	// 테스트 데이터
	//========================================

	/** 테스트 파티클 배열 */
	TArray<FKawaiiRenderParticle> TestParticles;

	/** 애니메이션 시간 */
	float AnimationTime = 0.0f;

	//========================================
	// GPU 렌더 리소스
	//========================================

	/** GPU 렌더 리소스 (SharedPtr로 수명 관리) */
	TSharedPtr<FKawaiiFluidRenderResource> RenderResource;

	//========================================
	// 내부 메서드
	//========================================

	/** GPU 렌더 리소스 초기화 */
	void InitializeRenderResource();

	/** 디버그 메시 초기화 */
	void InitializeDebugMesh();

	/** 디버그 메시 업데이트 */
	void UpdateDebugMeshInstances();

	/** 테스트 파티클 생성 */
	void GenerateTestParticles();

	/** 애니메이션 파티클 업데이트 */
	void UpdateAnimatedParticles(float DeltaTime);

	// 데이터 생성 헬퍼
	void GenerateStaticData();
	void GenerateGridPattern();
	void GenerateSpherePattern();
};
