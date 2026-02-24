#include "HeavyAnimInstance.h"
#include "HeavyCharacter.h"
#include "HeavyCharacterMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UHeavyAnimInstance::UHeavyAnimInstance()
{
	// Set default thresholds
	MovingSpeedThreshold = 10.0f;
	TurnInPlaceThreshold = 45.0f;
	LowStaminaThreshold = 0.3f;
}

void UHeavyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Cache references to character and movement component
	HeavyCharacter = Cast<AHeavyCharacter>(TryGetPawnOwner());

	if (HeavyCharacter)
	{
		HeavyMovement = Cast<UHeavyCharacterMovementComponent>(HeavyCharacter->GetCharacterMovement());

#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("HeavyAnimInstance initialized for: %s"), *HeavyCharacter->GetName());
#endif
	}
	else
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Error, TEXT("HeavyAnimInstance failed to find HeavyCharacter!"));
#endif
	}
}

void UHeavyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Update all animation data
	UpdateAnimationData(DeltaSeconds);
}

FString UHeavyAnimInstance::GetMovementStateString() const
{
	if (bIsInAir)
		return TEXT("In Air");
	if (bIsSprinting)
		return TEXT("Sprinting");
	if (bIsMoving)
		return TEXT("Walking");
	if (bShouldTurnInPlace)
		return TEXT("Turning");
	return TEXT("Idle");
}

void UHeavyAnimInstance::UpdateAnimationData(float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
	GEngine->AddOnScreenDebugMessage(99, 0.f, bIsInCombatStance ? FColor::Green : FColor::Red,
	                                 FString::Printf(TEXT("AnimInst bIsInCombatStance: %s"),
	                                                 bIsInCombatStance ? TEXT("TRUE") : TEXT("FALSE")));
#endif
	// Try to grab the pawn if we don't have one yet
	APawn* OwningPawn = TryGetPawnOwner();

	if (!OwningPawn)
	{
		return; // Still no pawn, wait for next frame
	}

	// If we have a pawn, but it's not cached as HeavyCharacter yet, try to cast
	if (!HeavyCharacter)
	{
		HeavyCharacter = Cast<AHeavyCharacter>(OwningPawn);

		if (HeavyCharacter)
		{
			HeavyMovement = Cast<UHeavyCharacterMovementComponent>(HeavyCharacter->GetCharacterMovement());
		}
	}

	// FINAL GUARD: If the cast fails (e.g. it's the wrong class), stop here
	if (!HeavyCharacter || !HeavyMovement)
	{
		// This confirms the character in the level IS NOT a child of AHeavyCharacter
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Red,
			                                 TEXT("CRITICAL: Pawn is NOT AHeavyCharacter!"));
		return;
	}

	if (GEngine && HeavyCharacter)
	{
		GEngine->AddOnScreenDebugMessage(5, 0.f, FColor::Yellow,
		                                 FString::Printf(TEXT("Speed: %f | Dir: %f"), GroundSpeed, Direction));
	}

	// === Update Movement Data ===

	// Get velocity from character
	Velocity = HeavyCharacter->GetVelocity();

	// Calculate ground speed (horizontal velocity only)
	FVector HorizontalVelocity = Velocity;
	HorizontalVelocity.Z = 0.f;
	GroundSpeed = HorizontalVelocity.Size();

	// Determine if moving
	bIsMoving = GroundSpeed > MovingSpeedThreshold;

	// Calculate movement direction relative to character rotation
	if (bIsMoving)
	{
		FRotator VelocityRotation = Velocity.Rotation();
		FRotator CharacterRotation = HeavyCharacter->GetActorRotation();

		// Get angle between velocity and character forward (-180 to 180)
		Direction = UKismetMathLibrary::NormalizedDeltaRotator(VelocityRotation, CharacterRotation).Yaw;
	}
	else
	{
		Direction = 0.f;
	}

	// Check if in air
	bIsInAir = HeavyMovement->IsFalling();

	// Get sprint state
	bIsSprinting = HeavyMovement->bIsSprinting;

	// === Update Rotation Data ===

	// Get yaw delta from character
	YawDelta = HeavyCharacter->YawDelta;
	AbsYawDelta = HeavyCharacter->AbsYawDelta;

	// Get combat stance
	bIsInCombatStance = HeavyCharacter->bIsInCombatStance;

	// Modify turn-in-place behavior for combat stance
	// In combat stance, always face mouse, so turn-in-place is more aggressive
	if (bIsInCombatStance)
	{
		bShouldTurnInPlace = AbsYawDelta > 30.0f; // Lower threshold
	}
	else
	{
		bShouldTurnInPlace = !bIsMoving && !bIsInAir && (AbsYawDelta > TurnInPlaceThreshold);
	}

	// === Update Stamina Data ===

	StaminaPercent = HeavyCharacter->GetStaminaPercent();
	bIsExhausted = HeavyCharacter->bIsExhausted;
	bIsStaminaLow = StaminaPercent < LowStaminaThreshold;

	// ... existing code ...

	// Update upper body blend weight
	float TargetWeight = bIsInCombatStance ? 1.0f : 0.0f;
	UpperBodyBlendWeight = FMath::FInterpTo(UpperBodyBlendWeight, TargetWeight, DeltaSeconds, 10.0f);

#if !UE_BUILD_SHIPPING
	if (bIsInCombatStance)
	{
		GEngine->AddOnScreenDebugMessage(98, 0.f, FColor::Cyan,
		                                 FString::Printf(TEXT("UpperBodyBlendWeight: %.2f"), UpperBodyBlendWeight));
	}
#endif

#if !UE_BUILD_SHIPPING && 0 // Set to 1 to enable verbose animation debugging
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(20, 0.f, FColor::Cyan,
		                                 FString::Printf(TEXT("Anim: Speed=%.1f Dir=%.1f Sprint=%d TurnIP=%d"),
		                                                 GroundSpeed, Direction, bIsSprinting, bShouldTurnInPlace));
	}
#endif
}

void UHeavyAnimInstance::AnimNotify_GuardRaised()
{
	// 1. Update the local animation state
	bIsReadyToAttack = true;

	// 2. Push the truth to the Character class
	if (HeavyCharacter)
	{
		HeavyCharacter->bIsReadyToAttack = true;

#if !UE_BUILD_SHIPPING
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, TEXT("NOTIFY: Guard Ready"));
#endif
	}
}
