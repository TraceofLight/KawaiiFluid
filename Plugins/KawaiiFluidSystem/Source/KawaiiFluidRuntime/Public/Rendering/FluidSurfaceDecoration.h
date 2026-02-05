// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "FluidSurfaceDecoration.generated.h"

/**
 * Texture Addressing Mode for UV coordinates.
 * Controls how textures are sampled when UV goes outside [0,1] range.
 */
UENUM(BlueprintType)
enum class ETextureAddressingMode : uint8
{
	/** Repeat the texture (default tiling) */
	Wrap UMETA(DisplayName = "Wrap (Repeat)"),

	/** Mirror the texture at boundaries (like decals) */
	Mirror UMETA(DisplayName = "Mirror")
};

/**
 * Texture Overlay Settings
 * Adds custom texture layer on top of the fluid surface (e.g., caustics, dirt, patterns).
 */
USTRUCT(BlueprintType)
struct KAWAIIFLUIDRUNTIME_API FSurfaceDecorationLayer
{
	GENERATED_BODY()

	//========================================
	// Enable
	//========================================

	/** Enable texture overlay on fluid surface */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay")
	bool bEnabled = false;

	//========================================
	// Texture
	//========================================

	/** Overlay texture (color/pattern) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Texture", meta = (EditCondition = "bEnabled"))
	TObjectPtr<UTexture2D> Texture = nullptr;

	/** Texture tiling density. Higher = smaller pattern. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Texture", meta = (EditCondition = "bEnabled", ClampMin = "0.0001", ClampMax = "1.0"))
	float TilingScale = 0.01f;

	/** UV wrap mode at texture boundaries */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Texture", meta = (EditCondition = "bEnabled && Texture != nullptr"))
	ETextureAddressingMode AddressingMode = ETextureAddressingMode::Wrap;

	//========================================
	// Normal Map
	//========================================

	/** Normal map for surface detail (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Normal", meta = (EditCondition = "bEnabled"))
	TObjectPtr<UTexture2D> NormalMap = nullptr;

	/** Normal map intensity. Higher = stronger bumps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Normal", meta = (EditCondition = "bEnabled && NormalMap != nullptr", ClampMin = "0.0", ClampMax = "2.0"))
	float NormalStrength = 1.0f;

	//========================================
	// Blending
	//========================================

	/** Overlay opacity. 0 = invisible, 1 = fully visible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Blending", meta = (EditCondition = "bEnabled", ClampMin = "0.0", ClampMax = "1.0"))
	float Opacity = 1.0f;

	/** Multiply blend mode. Off = additive, On = multiply with fluid color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Blending", meta = (EditCondition = "bEnabled"))
	bool bMultiply = false;

	/** Surface angle filter. -1 = all surfaces, 0 = horizontal only, 1 = upward only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Blending", meta = (EditCondition = "bEnabled", ClampMin = "-1.0", ClampMax = "1.0"))
	float NormalZThreshold = -1.0f;

	//========================================
	// Texture Animation
	//========================================

	/** Move texture with fluid flow. Requires Flow Animation to be enabled globally. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Texture", meta = (EditCondition = "bEnabled"))
	bool bUseFlowAnimation = true;

	/** How much flow affects texture movement. 0 = static, 1 = full flow speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Texture", meta = (EditCondition = "bEnabled && bUseFlowAnimation", ClampMin = "0.0", ClampMax = "1.0"))
	float FlowInfluence = 0.5f;

	/** Constant scroll speed (UV units per second). Applied on top of flow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Texture", meta = (EditCondition = "bEnabled"))
	FVector2D ScrollSpeed = FVector2D::ZeroVector;

	/** Animate texture with organic UV jittering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Texture", meta = (EditCondition = "bEnabled"))
	bool bJitterEnabled = false;

	/** Jitter displacement amount. Higher = more movement. (0.01~0.05: subtle, 0.1~0.2: noticeable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Texture", meta = (EditCondition = "bEnabled && bJitterEnabled", ClampMin = "0.0", ClampMax = "0.5"))
	float JitterStrength = 0.03f;

	/** Jitter animation speed. Higher = faster movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Texture", meta = (EditCondition = "bEnabled && bJitterEnabled", ClampMin = "0.1", ClampMax = "10.0"))
	float JitterSpeed = 2.0f;
};

/**
 * Foam Settings
 * Controls foam generation and appearance on fluid surface.
 */
USTRUCT(BlueprintType)
struct KAWAIIFLUIDRUNTIME_API FFoamSettings
{
	GENERATED_BODY()

	//========================================
	// Enable
	//========================================

	/** Enable foam effect on the fluid surface */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam")
	bool bEnabled = false;

	//========================================
	// Appearance
	//========================================

	/** Foam color. Multiplied with texture if assigned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Appearance", meta = (EditCondition = "bEnabled"))
	FLinearColor FoamColor = FLinearColor::White;

	/** Overall brightness. Higher = brighter foam. Supports HDR (values > 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Appearance", meta = (EditCondition = "bEnabled", ClampMin = "0.0", ClampMax = "5.0"))
	float Intensity = 1.0f;

	//========================================
	// Texture
	//========================================

	/** Foam pattern texture. If not set, foam renders as solid color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Texture", meta = (EditCondition = "bEnabled"))
	TObjectPtr<UTexture2D> FoamTexture = nullptr;

	/** Texture tiling density. Higher = smaller pattern. (0.01 = 1m per tile, 0.1 = 10cm per tile) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Texture", meta = (EditCondition = "bEnabled", ClampMin = "0.0001", ClampMax = "1.0"))
	float TilingScale = 0.02f;

	/** UV wrap mode at texture boundaries */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Texture", meta = (EditCondition = "bEnabled && FoamTexture != nullptr"))
	ETextureAddressingMode AddressingMode = ETextureAddressingMode::Wrap;

	//========================================
	// Texture Animation
	//========================================

	/** Move foam texture with fluid flow. Requires Flow Animation to be enabled globally. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Texture", meta = (EditCondition = "bEnabled"))
	bool bUseFlowAnimation = true;

	/** Animate texture with organic UV jittering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Texture", meta = (EditCondition = "bEnabled"))
	bool bJitterEnabled = false;

	/** Jitter displacement amount. Higher = more movement. (0.01~0.05: subtle, 0.1~0.2: noticeable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Texture", meta = (EditCondition = "bEnabled && bJitterEnabled", ClampMin = "0.0", ClampMax = "0.5"))
	float JitterStrength = 0.03f;

	/** Jitter animation speed. Higher = faster movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Texture", meta = (EditCondition = "bEnabled && bJitterEnabled", ClampMin = "0.1", ClampMax = "10.0"))
	float JitterSpeed = 2.0f;

	//========================================
	// Generation: Velocity
	//========================================

	/** Minimum velocity (cm/s) to generate foam. Foam appears where fluid moves faster than this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Generation", meta = (EditCondition = "bEnabled", ClampMin = "0.0", ClampMax = "1000.0"))
	float VelocityThreshold = 100.0f;

	//========================================
	// Generation: Wave Crest
	//========================================

	/** Generate foam at wave peaks and breaking points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Generation", meta = (EditCondition = "bEnabled"))
	bool bWaveCrestFoam = true;

	/** Wave crest foam intensity. Higher = more foam at wave peaks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Generation", meta = (EditCondition = "bEnabled && bWaveCrestFoam", ClampMin = "0.0", ClampMax = "2.0"))
	float WaveCrestFoamStrength = 1.0f;

	//========================================
	// Generation: Thin Areas
	//========================================

	/** Generate foam in thin fluid regions (spray, droplets, sheet edges) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Generation", meta = (EditCondition = "bEnabled"))
	bool bThicknessFoam = true;

	/** Thickness threshold. Foam appears where fluid is thinner than this value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Generation", meta = (EditCondition = "bEnabled && bThicknessFoam", ClampMin = "0.01", ClampMax = "5.0"))
	float ThicknessThreshold = 0.5f;

	/** Thin-area foam intensity. Higher = more foam in thin regions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Generation", meta = (EditCondition = "bEnabled && bThicknessFoam", ClampMin = "0.0", ClampMax = "1.0"))
	float ThicknessFoamStrength = 0.3f;

	//========================================
	// Edge Softening
	//========================================

	/** Blur velocity texture to soften foam boundaries. Removes sharp particle edges. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Edge Softening", meta = (EditCondition = "bEnabled"))
	bool bVelocitySmoothing = true;

	/** Blur radius in pixels. Higher = softer edges. (3~10: subtle, 10~20: very soft) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Edge Softening", meta = (EditCondition = "bEnabled && bVelocitySmoothing", ClampMin = "1.0", ClampMax = "30.0"))
	float VelocitySmoothingRadius = 8.0f;

	/** Blur iterations. Higher = smoother but slower. 1~2 is usually enough. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam|Edge Softening", meta = (EditCondition = "bEnabled && bVelocitySmoothing", ClampMin = "1", ClampMax = "5"))
	int32 VelocitySmoothingIterations = 1;
};

/**
 * Emissive Settings
 * Controls glow/emission effects for lava, magic, or radioactive fluids.
 */
USTRUCT(BlueprintType)
struct KAWAIIFLUIDRUNTIME_API FEmissiveSettings
{
	GENERATED_BODY()

	//========================================
	// Enable
	//========================================

	/** Enable glow/emission effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Emissive")
	bool bEnabled = false;

	//========================================
	// Appearance
	//========================================

	/** Glow color (HDR supported) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Emissive|Appearance", meta = (EditCondition = "bEnabled", HDR))
	FLinearColor EmissiveColor = FLinearColor(1.0f, 0.3f, 0.05f, 1.0f);

	/** Minimum glow brightness. Always visible even when stationary. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Emissive|Appearance", meta = (EditCondition = "bEnabled", ClampMin = "0.0", ClampMax = "100.0"))
	float MinEmissive = 2.0f;

	/** Dynamic glow multiplier. Scales velocity and pulse effects. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Emissive|Appearance", meta = (EditCondition = "bEnabled", ClampMin = "0.0", ClampMax = "100.0"))
	float Intensity = 10.0f;

	//========================================
	// Velocity Response
	//========================================

	/** Faster flow = brighter glow. Good for lava (hot = fast = bright). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Emissive|Velocity", meta = (EditCondition = "bEnabled"))
	bool bVelocityEmissive = true;

	/** Velocity sensitivity. Higher = glows brighter at lower speeds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Emissive|Velocity", meta = (EditCondition = "bEnabled && bVelocityEmissive", ClampMin = "0.1", ClampMax = "5.0"))
	float VelocitySensitivity = 1.0f;

	//========================================
	// Pulse Animation
	//========================================

	/** Pulse cycle time in seconds. 0 = no pulse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Emissive|Pulse", meta = (EditCondition = "bEnabled", ClampMin = "0.0", ClampMax = "10.0"))
	float PulsePeriod = 0.0f;

	/** Pulse brightness variation. 0 = steady, 1 = full range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Emissive|Pulse", meta = (EditCondition = "bEnabled && PulsePeriod > 0", ClampMin = "0.0", ClampMax = "1.0"))
	float PulseAmplitude = 0.2f;
};

/**
 * Flow Animation Settings
 * Controls flow-based UV animation using particle velocity.
 * Affects Foam and Overlay textures that opt-in to flow animation.
 */
USTRUCT(BlueprintType)
struct KAWAIIFLUIDRUNTIME_API FFlowMapSettings
{
	GENERATED_BODY()

	//========================================
	// Enable
	//========================================

	/** Enable flow-based texture animation. Textures move with fluid velocity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Flow Animation")
	bool bEnabled = false;

	//========================================
	// Animation Speed
	//========================================

	/** Overall flow animation speed. Higher = faster texture movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Flow Animation", meta = (EditCondition = "bEnabled", ClampMin = "0.0", ClampMax = "10.0"))
	float FlowSpeed = 1.0f;

	/** UV distortion amount. Higher = more warping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Flow Animation", meta = (EditCondition = "bEnabled", ClampMin = "0.0", ClampMax = "1.0"))
	float DistortionStrength = 0.1f;

	//========================================
	// Velocity Accumulation
	//========================================

	/** Velocity to UV scale. Higher = faster texture movement per velocity unit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Flow Animation",
		meta = (EditCondition = "bEnabled", ClampMin = "0.01", ClampMax = "100.0"))
	float VelocityScale = 1.0f;

	/** Flow decay rate when velocity stops. 0 = no decay, higher = returns to rest faster. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Flow Animation",
		meta = (EditCondition = "bEnabled", ClampMin = "0.0", ClampMax = "5.0"))
	float FlowDecay = 0.1f;

	/** Maximum flow offset (cm) before UV wrapping. Prevents overflow in long sessions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Flow Animation",
		meta = (EditCondition = "bEnabled", ClampMin = "10.0", ClampMax = "10000.0", AdvancedDisplay))
	float MaxFlowOffset = 1000.0f;
};

/**
 * Surface Decoration Parameters
 * Main settings for fluid surface visual effects (foam, glow, texture overlays).
 */
USTRUCT(BlueprintType)
struct KAWAIIFLUIDRUNTIME_API FSurfaceDecorationParams
{
	GENERATED_BODY()

	/** Enable surface decoration effects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration")
	bool bEnabled = false;

	//========================================
	// Foam
	//========================================

	/** Foam/bubble effect for water surfaces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Foam", meta = (EditCondition = "bEnabled", ShowOnlyInnerProperties))
	FFoamSettings Foam;

	//========================================
	// Overlay
	//========================================

	/** Custom texture overlay (caustics, dirt, patterns) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay", meta = (EditCondition = "bEnabled", ShowOnlyInnerProperties))
	FSurfaceDecorationLayer Layer;

	/** Master opacity for overlay. Multiplied with overlay's own opacity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Blending", meta = (EditCondition = "bEnabled", ClampMin = "0.0", ClampMax = "1.0"))
	float LayerFinalOpacity = 1.0f;

	/** Blend with fluid color. 0 = overlay only, 1 = tinted by fluid color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Blending", meta = (EditCondition = "bEnabled", ClampMin = "0.0", ClampMax = "1.0"))
	float LayerBlendWithFluidColor = 0.5f;

	/** Apply scene lighting to overlay texture */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Lighting", meta = (EditCondition = "bEnabled"))
	bool bApplyLightingToLayer = true;

	/** Overlay specular intensity. Higher = shinier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Lighting", meta = (EditCondition = "bEnabled && bApplyLightingToLayer", ClampMin = "0.0", ClampMax = "2.0"))
	float LayerSpecularStrength = 0.3f;

	/** Overlay specular roughness. 0 = mirror-like, 1 = matte. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Overlay|Lighting", meta = (EditCondition = "bEnabled && bApplyLightingToLayer", ClampMin = "0.0", ClampMax = "1.0"))
	float LayerSpecularRoughness = 0.5f;

	//========================================
	// Flow Animation
	//========================================

	/** Flow animation settings. Controls how Foam and Overlay textures move with fluid velocity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Flow Animation", meta = (EditCondition = "bEnabled", ShowOnlyInnerProperties))
	FFlowMapSettings FlowMap;

	//========================================
	// Emissive
	//========================================

	/** Glow/emission effect for lava, magic, or radioactive fluids */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Surface Decoration|Emissive", meta = (EditCondition = "bEnabled", ShowOnlyInnerProperties))
	FEmissiveSettings Emissive;

	FSurfaceDecorationParams() = default;
};
