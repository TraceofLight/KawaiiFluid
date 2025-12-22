// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Rendering/FluidRendererSubsystem.h"
#include "Rendering/FluidSceneViewExtension.h"
#include "Rendering/IKawaiiFluidRenderable.h"
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

	RegisteredRenderables.Empty();
	RegisteredSimulators.Empty();

	Super::Deinitialize();

	UE_LOG(LogTemp, Log, TEXT("FluidRendererSubsystem Deinitialized"));
}

//========================================
// 통합 렌더링 관리
//========================================

void UFluidRendererSubsystem::RegisterRenderable(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	// 인터페이스 구현 확인
	if (!Actor->GetClass()->ImplementsInterface(UKawaiiFluidRenderable::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("FluidRendererSubsystem: %s does not implement IKawaiiFluidRenderable"), *Actor->GetName());
		return;
	}

	// TScriptInterface로 래핑
	TScriptInterface<IKawaiiFluidRenderable> Renderable;
	Renderable.SetObject(Actor);
	Renderable.SetInterface(Cast<IKawaiiFluidRenderable>(Actor));

	// 중복 체크
	bool bAlreadyRegistered = false;
	for (const TScriptInterface<IKawaiiFluidRenderable>& ExistingRenderable : RegisteredRenderables)
	{
		if (ExistingRenderable.GetObject() == Actor)
		{
			bAlreadyRegistered = true;
			break;
		}
	}

	if (!bAlreadyRegistered)
	{
		RegisteredRenderables.Add(Renderable);
		UE_LOG(LogTemp, Log, TEXT("FluidRendererSubsystem: Registered %s"), *Actor->GetName());
	}
}

void UFluidRendererSubsystem::UnregisterRenderable(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	RegisteredRenderables.RemoveAll([Actor](const TScriptInterface<IKawaiiFluidRenderable>& Renderable)
	{
		return Renderable.GetObject() == Actor;
	});

	UE_LOG(LogTemp, Log, TEXT("FluidRendererSubsystem: Unregistered %s"), *Actor->GetName());
}

TArray<IKawaiiFluidRenderable*> UFluidRendererSubsystem::GetAllRenderables() const
{
	TArray<IKawaiiFluidRenderable*> Result;
	Result.Reserve(RegisteredRenderables.Num());

	for (const TScriptInterface<IKawaiiFluidRenderable>& Renderable : RegisteredRenderables)
	{
		if (Renderable.GetInterface())
		{
			Result.Add(Renderable.GetInterface());
		}
	}

	return Result;
}

//========================================
// 레거시 호환성
//========================================

void UFluidRendererSubsystem::RegisterSimulator(AFluidSimulator* Simulator)
{
	// 통합 메서드로 리다이렉트
	RegisterRenderable(Simulator);

	// 레거시 배열에도 추가 (호환성)
	if (Simulator && !RegisteredSimulators.Contains(Simulator))
	{
		RegisteredSimulators.Add(Simulator);
		UE_LOG(LogTemp, Log, TEXT("FluidRendererSubsystem: Registered Simulator (legacy) %s"), *Simulator->GetName());
	}
}

void UFluidRendererSubsystem::UnregisterSimulator(AFluidSimulator* Simulator)
{
	// 통합 메서드로 리다이렉트
	UnregisterRenderable(Simulator);

	// 레거시 배열에서도 제거 (호환성)
	if (Simulator)
	{
		RegisteredSimulators.Remove(Simulator);
		UE_LOG(LogTemp, Log, TEXT("FluidRendererSubsystem: Unregistered Simulator (legacy) %s"), *Simulator->GetName());
	}
}
