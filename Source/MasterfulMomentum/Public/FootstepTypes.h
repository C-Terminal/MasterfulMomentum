#pragma once

#include "CoreMinimal.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "FootstepTypes.generated.h"

UENUM(BlueprintType)
enum class EFootType : uint8
{
	Left,
	Right
};

UENUM(BlueprintType)
enum class ESurfaceType : uint8
{
	Default,
	Concrete,
	Dirt,
	Grass,
	Wood,
	Metal,
	Water
};

USTRUCT(BlueprintType)
struct FFootstepContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ESurfaceType SurfaceType = ESurfaceType::Default;

	UPROPERTY(BlueprintReadOnly)
	float Velocity = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float StaminaPercent = 1.0f;

	UPROPERTY(BlueprintReadOnly)
	bool bIsInCombat = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly)
	EFootType FootType = EFootType::Left;

	UPROPERTY(BlueprintReadOnly)
	FVector Location = FVector::ZeroVector;
};