// Fill out your copyright notice in the Description page of Project Settings.

#include "MasterfulMomentum/Public/HeavyCharacterMovementComponent.h"

#include "HeavyCharacter.h"

UHeavyCharacterMovementComponent::UHeavyCharacterMovementComponent()
{
	// Enable parameters ensuring we can't turn instantly
	bUseControllerDesiredRotation = false;
	bOrientRotationToMovement = false;
}

void UHeavyCharacterMovementComponent::SetHeavyModeEnabled(bool bEnabled)
{
	if (bEnabled)
	{
		SetMovementMode(MOVE_Custom, CMOVE_HeavyGrounded);
	}
	else
	{
		SetMovementMode(MOVE_Walking);
	}
}

// 1. Hook into the Physics Engine
void UHeavyCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	// Super::PhysCustom(deltaTime, Iterations);

	if (CustomMovementMode == CMOVE_HeavyGrounded)
	{
		PhysHeavyGrounded(deltaTime, Iterations);
	}
}

void UHeavyCharacterMovementComponent::PhysHeavyGrounded(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME) return;

	// Get custom input
	FVector InputVector = CustomInputVector;
	CustomInputVector = FVector::ZeroVector;

	if (!InputVector.IsNearlyZero())
	{
		InputVector = InputVector.GetSafeNormal();
	}

	bool bHasInput = !InputVector.IsNearlyZero();

	// === COMBAT STANCE CHECK ===
	AHeavyCharacter* HeavyChar = Cast<AHeavyCharacter>(CharacterOwner);
	bool bInCombatStance = HeavyChar && HeavyChar->bIsInCombatStance;

	// === SPRINT LOGIC ===
	// Can't sprint in combat stance
	bool bWantsToSprint = HeavyChar && HeavyChar->bWantsToSprint;
	bool bCanSprint = HeavyChar && HeavyChar->CanSprint();
	bool bMovingFastEnough = Velocity.Size() > MinSprintVelocity;

	bool bWasSprintingLastFrame = bIsSprinting;
	bIsSprinting = !bInCombatStance && bWantsToSprint && bCanSprint && bHasInput && (bMovingFastEnough ||
		bWasSprintingLastFrame);

	if (HeavyChar)
	{
		HeavyChar->UpdateStamina(deltaTime, bIsSprinting);
	}

	// === CALCULATE SPEED MULTIPLIERS ===
	float CurrentSpeedMult = 1.0f;
	float CurrentAccelMult = 1.0f;
	float CurrentTurnMult = 1.0f;

	if (bInCombatStance)
	{
		// Apply combat stance speed reduction
		CurrentSpeedMult = HeavyChar->CombatMovementSpeedMultiplier;
		CurrentAccelMult = HeavyChar->CombatMovementSpeedMultiplier;
		CurrentTurnMult = 0.5f; // Slower turning in combat stance
	}
	else if (bIsSprinting)
	{
		// Normal sprint multipliers
		CurrentSpeedMult = SprintSpeedMultiplier;
		CurrentAccelMult = SprintAccelerationMultiplier;
		CurrentTurnMult = SprintTurnRateMultiplier;
	}

	// --- A. Calculate Rotation (Heavy Turning) ---
	// In combat stance, rotation is handled by UpdateMouseFacing, but we still apply input rotation if not in combat
	if (!InputVector.IsNearlyZero() && !bInCombatStance)
	{
		FRotator CurrentRotation = UpdatedComponent->GetComponentRotation();
		FRotator TargetRotation = InputVector.Rotation();

		float EffectiveTurnRate = HeavyTurnRate * CurrentTurnMult;

		FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, TargetRotation, deltaTime, EffectiveTurnRate);
		MoveUpdatedComponent(FVector::ZeroVector, NewRotation, false);
	}

	// --- B. Calculate Forces ---
	FVector CurrentVelocity = Velocity;
	FVector AppliedForce = FVector::ZeroVector;

	if (bHasInput)
	{
		float SurfaceMult = 1.0f;
		float EffectiveAcceleration = HeavyAcceleration * CurrentAccelMult;

		// Angle the input parallel to the current floor so we don't push "into" the ramp
		FVector SlopeInput = InputVector;
		if (CurrentFloor.IsWalkableFloor() && CurrentFloor.HitResult.Normal.Z > KINDA_SMALL_NUMBER)
		{
			SlopeInput = FVector::VectorPlaneProject(InputVector, CurrentFloor.HitResult.Normal).GetSafeNormal();
		}

		AppliedForce = SlopeInput * EffectiveAcceleration * SurfaceMult;

#if !UE_BUILD_SHIPPING
		if (bInCombatStance)
		{
			GEngine->AddOnScreenDebugMessage(13, 0.f, FColor::Orange, TEXT("COMBAT MOVEMENT"));
		}
		else if (bIsSprinting)
		{
			GEngine->AddOnScreenDebugMessage(13, 0.f, FColor::Yellow, TEXT("SPRINTING"));
		}
#endif
	}
	else
	{
		// Braking / Friction Force
		if (!CurrentVelocity.IsNearlyZero())
		{
			FVector FrictionDir = -CurrentVelocity.GetSafeNormal();

			// Use stronger braking in combat stance for quicker stop when releasing keys
			float BrakingForce = bInCombatStance ? HeavyDeceleration * 1.5f : HeavyDeceleration;
			AppliedForce = FrictionDir * BrakingForce;

			if ((AppliedForce * deltaTime).SizeSquared() > CurrentVelocity.SizeSquared())
			{
				CurrentVelocity = FVector::ZeroVector;
				AppliedForce = FVector::ZeroVector;
			}
		}
	}

	// --- C. Update Velocity ---
	Velocity = CurrentVelocity + (AppliedForce * deltaTime);

	float CurrentMaxSpeed = HeavyMaxSpeed * CurrentSpeedMult;

	if (Velocity.Size() > CurrentMaxSpeed)
	{
		Velocity = Velocity.GetSafeNormal() * CurrentMaxSpeed;
	}

	// --- D. Perform Movement ---
	// Store our speed before moving to prevent bleed on geometric transitions
	float OriginalSpeed = Velocity.Size();

	FVector Delta = Velocity * deltaTime;
	FHitResult Hit(1.f);

	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentRotation(), true, Hit);

	// Handle Wall Sliding & Transitions
	if (Hit.IsValidBlockingHit())
	{
		HandleImpact(Hit, deltaTime, Delta);
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);

		// Prevent speed-bleed when hitting the seam between flat ground and a slope
		if (Hit.Normal.Z > 0.7f)
		{
			Velocity = Velocity.GetSafeNormal() * OriginalSpeed;
		}
	}

	// --- E. FLOOR SNAPPING & LEDGE DETECTION ---
	FFindFloorResult FloorResult;
	FindFloor(UpdatedComponent->GetComponentLocation(), FloorResult, false, nullptr);

	CurrentFloor = FloorResult;

	if (FloorResult.IsWalkableFloor())
	{
		TimeSinceLastValidFloor = 0.f;

		const float FloorDist = FloorResult.GetDistanceToFloor();
		const float MaxStepDownHeight = MaxStepHeight;

		if (FloorDist > KINDA_SMALL_NUMBER && FloorDist <= MaxStepDownHeight)
		{
			const FVector DownVector = FVector(0.f, 0.f, -FloorDist);
			FHitResult StepDownHit(1.f);

			SafeMoveUpdatedComponent(DownVector, UpdatedComponent->GetComponentRotation(), true, StepDownHit);

			if (StepDownHit.IsValidBlockingHit() && StepDownHit.Normal.Z > KINDA_SMALL_NUMBER)
			{
				const float VelZ = Velocity.Z;
				Velocity = FVector::VectorPlaneProject(Velocity, StepDownHit.Normal);

				// Restore speed after projecting onto the down-slope
				Velocity = Velocity.GetSafeNormal() * OriginalSpeed;

				if (VelZ < 0.f)
				{
					Velocity.Z = FMath::Min(VelZ, Velocity.Z);
				}
			}
		}
		else if (FloorDist <= KINDA_SMALL_NUMBER)
		{
			if (FloorResult.HitResult.Normal.Z > KINDA_SMALL_NUMBER)
			{
				Velocity = FVector::VectorPlaneProject(Velocity, FloorResult.HitResult.Normal);
			}
		}
	}
	else
	{
		TimeSinceLastValidFloor += deltaTime;
		const float DistanceToFloor = FloorResult.GetDistanceToFloor();

		if (TimeSinceLastValidFloor > LedgeGraceTime &&
			(DistanceToFloor > MaxStepHeight || !FloorResult.bBlockingHit))
		{
			bIsSprinting = false;
			SetMovementMode(MOVE_Falling);

#if !UE_BUILD_SHIPPING
			UE_LOG(LogTemp, Warning, TEXT("HeavyCharacter fell off ledge - switching to MOVE_Falling"));
#endif
		}
	}
}

void UHeavyCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode,
                                                             uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	// If we just landed from falling, return to heavy grounded mode
	if (PreviousMovementMode == MOVE_Falling && MovementMode == MOVE_Walking)
	{
		SetMovementMode(MOVE_Custom, CMOVE_HeavyGrounded);

#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("Landed - returning to HeavyGrounded mode"));
#endif
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Log, TEXT("Movement mode changed: %d -> %d (Custom: %d -> %d)"),
	       (int32)PreviousMovementMode, (int32)MovementMode,
	       PreviousCustomMode, CustomMovementMode);
#endif
}

void UHeavyCharacterMovementComponent::ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations)
{
	Super::ProcessLanded(Hit, remainingTime, Iterations);

	// Additional landing logic if needed
	// For example, apply landing impact, play sound, etc.

#if !UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning, TEXT("ProcessLanded called - Impact Point: %s"), *Hit.ImpactPoint.ToString());
#endif
}

// 3. Network Boilerplate (Crucial for Multiplayer)

void UHeavyCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	// Decode flags here if you add sprinting/etc specific to heavy mode
}

class FNetworkPredictionData_Client* UHeavyCharacterMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UHeavyCharacterMovementComponent* MutableThis = const_cast<UHeavyCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Heavy(*this);
	}
	return ClientPredictionData;
}

void FSavedMove_Heavy::Clear()
{
	Super::Clear();
	// Clear custom flags
}

uint8 FSavedMove_Heavy::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	return Result;
}

bool FSavedMove_Heavy::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_Heavy::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel,
                                  class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
}

FNetworkPredictionData_Client_Heavy::FNetworkPredictionData_Client_Heavy(
	const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_Heavy::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Heavy());
}
