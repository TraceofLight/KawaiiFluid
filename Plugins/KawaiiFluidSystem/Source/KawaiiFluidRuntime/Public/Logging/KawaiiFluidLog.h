// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Primary plugin-wide category for plugin logging
KAWAIIFLUIDRUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogKawaiiFluid, Log, All);

// Always-on plugin log (subject to category verbosity/runtime config)
#define KF_LOG(Verbosity, Format, ...) \
	UE_LOG(LogKawaiiFluid, Verbosity, Format, ##__VA_ARGS__)

// Debug/Development-only plugin log (compiled out in Shipping/Test builds)
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	#define KF_LOG_DEV(Verbosity, Format, ...) do { } while (0)
#else
	#define KF_LOG_DEV(Verbosity, Format, ...) \
		KF_LOG(Verbosity, Format, ##__VA_ARGS__)
#endif

// Shipping-only plugin log (compiled out in non-Shipping builds)
#if UE_BUILD_SHIPPING
	#define KF_LOG_SHIPPING(Verbosity, Format, ...) \
		KF_LOG(Verbosity, Format, ##__VA_ARGS__)
#else
	#define KF_LOG_SHIPPING(Verbosity, Format, ...) do { } while (0)
#endif
