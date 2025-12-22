// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/KawaiiRenderParticle.h"
#include "FluidRenderingTestActor.generated.h"

class FKawaiiFluidRenderResource;

/**
 * 더미 데이터 생성 모드
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
 * 렌더링 테스트용 더미 파티클 액터
 * 물리 시뮬레이션 없이 GPU 버퍼 업로드만 수행하여 SSFR 파이프라인 테스트
 */
UCLASS(BlueprintType, HideCategories = (Collision, Physics, LOD, Cooking))
class KAWAIIFLUIDRUNTIME_API AFluidRenderingTestActor : public AActor
{
	GENERATED_BODY()

public:
	AFluidRenderingTestActor();
	virtual ~AFluidRenderingTestActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;
	virtual void Tick(float DeltaTime) override;

	//========================================
	// Components
	//========================================

	/** Root 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* RootSceneComponent;

	//========================================
	// 테스트 모드 설정
	//========================================

	/** 렌더링 활성화 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Mode")
	bool bEnableRendering = true;

	/** 더미 데이터 생성 모드 */
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
	// 블루프린트 함수
	//========================================

	/** 더미 데이터 재생성 */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void RegenerateDummyData();

	/** GPU 버퍼 강제 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "Test")
	void ForceUpdateGPUBuffer();

	/** 현재 파티클 수 반환 */
	UFUNCTION(BlueprintPure, Category = "Test")
	int32 GetCurrentParticleCount() const { return DummyParticles.Num(); }

	//========================================
	// 렌더 리소스 접근 (FluidRendererSubsystem용)
	//========================================

	/** GPU 렌더 리소스 반환 */
	FKawaiiFluidRenderResource* GetRenderResource() const { return RenderResource.Get(); }

	/** 렌더 리소스 유효성 확인 */
	bool IsRenderResourceValid() const;

private:
	//========================================
	// 더미 데이터
	//========================================

	/** 더미 파티클 배열 */
	TArray<FKawaiiRenderParticle> DummyParticles;

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

	/** 더미 파티클 생성 */
	void GenerateDummyParticles();

	/** 애니메이션 파티클 업데이트 */
	void UpdateAnimatedParticles(float DeltaTime);

	// 데이터 생성 헬퍼
	void GenerateStaticData();
	void GenerateGridPattern();
	void GenerateSpherePattern();
};
