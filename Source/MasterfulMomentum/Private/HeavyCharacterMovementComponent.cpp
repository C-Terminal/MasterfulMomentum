// Fill out your copyright notice in the Description page of Project Settings.

#include "MasterfulMomentum/Public/HeavyCharacterMovementComponent.h"

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

	// Use our custom input instead of ConsumeInputVector
	FVector InputVector = CustomInputVector;
	CustomInputVector = FVector::ZeroVector;

	if (!InputVector.IsNearlyZero())
	{
		InputVector = InputVector.GetSafeNormal();
	}

	bool bHasInput = !InputVector.IsNearlyZero();

	// --- A. Calculate Rotation (Heavy Turning) ---
	if (!InputVector.IsNearlyZero())
	{
		FRotator CurrentRotation = UpdatedComponent->GetComponentRotation();
		FRotator TargetRotation = InputVector.Rotation();
		FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, TargetRotation, deltaTime, HeavyTurnRate);
		MoveUpdatedComponent(FVector::ZeroVector, NewRotation, false);
	}

	// --- B. Calculate Forces ---
	FVector CurrentVelocity = Velocity;
	FVector AppliedForce = FVector::ZeroVector;

	if (bHasInput)
	{
		float SurfaceMult = 1.0f;
		AppliedForce = InputVector * HeavyAcceleration * SurfaceMult;
	}
	else
	{
		// Braking / Friction Force
		if (!CurrentVelocity.IsNearlyZero())
		{
			FVector FrictionDir = -CurrentVelocity.GetSafeNormal();
			AppliedForce = FrictionDir * HeavyDeceleration;

			if ((AppliedForce * deltaTime).SizeSquared() > CurrentVelocity.SizeSquared())
			{
				CurrentVelocity = FVector::ZeroVector;
				AppliedForce = FVector::ZeroVector;
			}
		}
	}

	// --- C. Update Velocity ---
	Velocity = CurrentVelocity + (AppliedForce * deltaTime);

	float CurrentMaxSpeed = HeavyMaxSpeed;
	if (Velocity.Size() > CurrentMaxSpeed)
	{
		Velocity = Velocity.GetSafeNormal() * CurrentMaxSpeed;
	}

	// --- D. Perform Movement ---
	FVector Delta = Velocity * deltaTime;
	FHitResult Hit(1.f);

	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentRotation(), true, Hit);

	// Handle Wall Sliding
	if (Hit.IsValidBlockingHit())
	{
		HandleImpact(Hit, deltaTime, Delta);
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
	}

	// --- E. FLOOR SNAPPING & LEDGE DETECTION ---
	MaintainHorizontalGroundVelocity();

	// Find floor beneath us
	FFindFloorResult FloorResult;
	FindFloor(UpdatedComponent->GetComponentLocation(), FloorResult, false, nullptr);

	// Update the cached floor
	CurrentFloor = FloorResult;

	if (FloorResult.IsWalkableFloor())
	{
		// Reset coyote time when we have valid ground
		TimeSinceLastValidFloor = 0.f;

		// We found a valid floor - check if we need to step down to it
		const float FloorDist = FloorResult.GetDistanceToFloor();
		const float MaxStepDownHeight = MaxStepHeight; // Use base class variable

		if (FloorDist > KINDA_SMALL_NUMBER && FloorDist <= MaxStepDownHeight)
		{
			// Step down to maintain contact with the ground
			const FVector DownVector = FVector(0.f, 0.f, -FloorDist);
			FHitResult StepDownHit(1.f);

			SafeMoveUpdatedComponent(DownVector, UpdatedComponent->GetComponentRotation(), true, StepDownHit);

			if (StepDownHit.IsValidBlockingHit() && StepDownHit.Normal.Z > KINDA_SMALL_NUMBER)
			{
				const float VelZ = Velocity.Z;
				Velocity = FVector::VectorPlaneProject(Velocity, StepDownHit.Normal);

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
		// No walkable floor found - START COYOTE TIME
		TimeSinceLastValidFloor += deltaTime;

		const float DistanceToFloor = FloorResult.GetDistanceToFloor();

		// Only fall if we've exceeded coyote time AND the drop is significant
		if (TimeSinceLastValidFloor > LedgeGraceTime &&
			(DistanceToFloor > MaxStepHeight || !FloorResult.bBlockingHit))
		{
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
