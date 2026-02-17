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
	Super::PhysCustom(deltaTime, Iterations);

	if (CustomMovementMode == CMOVE_HeavyGrounded)
	{
		PhysHeavyGrounded(deltaTime, Iterations);
	}
}

// 2. Implement the custom physics
void UHeavyCharacterMovementComponent::PhysHeavyGrounded(float deltaTime, int32 Iterations)
{
	// TODO: Implement custom physics
	if (deltaTime < MIN_TICK_TIME) return;

	// A. Calculate Input
	//We don't want the raw input vector to snap our velocity
	FVector InputVector = ConsumeInputVector();
	if (!InputVector.IsNearlyZero())
	{
		InputVector = InputVector.GetSafeNormal();
	}

	// B. Calculate Rotation (Heavy Turning)
	// Project Zomboid style: Character rotates slowly, velocity follows.
	if (!InputVector.IsNearlyZero())
	{
		FRotator CurrentRotation = UpdatedComponent->GetComponentRotation();
		FRotator TargetRotation = InputVector.Rotation();

		// RInterpConstantTo ensures a fixed turn rate (Tank-like or Heavy Human)
		// RInterpTo would be "Spring-like" (fast at first, slow at end)
		FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, TargetRotation, deltaTime, HeavyTurnRate);
        
		MoveUpdatedComponent(FVector::ZeroVector, NewRotation, false);
	}

	// C. Calculate Velocity (Newtonian Physics)
	// Velocity += Acceleration * dt

	// Are we trying to move?
	bool bHasInput = !InputVector.IsNearlyZero();
    
	FVector CurrentVelocity = Velocity;
	FVector AppliedForce = FVector::ZeroVector;

	if (bHasInput)
	{
		// Add Acceleration in the direction of INPUT (or facing, depending on preference)
		// Using InputVector makes it responsive. Using GetActorForwardVector() makes it "tank controls".

		float SurfaceMult = 1.0f;
		if (CurrentFloor.IsWalkableFloor()) {
			// Read Physical Material from the hit result
			UPhysicalMaterial* PhysMat = CurrentFloor.HitResult.PhysMaterial.Get();
			if (PhysMat) SurfaceMult = PhysMat->Friction; // Or a custom lookup table
		}
		
		// Apply to acceleration logic
		AppliedForce = InputVector * HeavyAcceleration * SurfaceMult;
	}
	else
	{
		/**
		 *
		* Practical Example
		* Imagine:
		* Current velocity = 100 units/second forward
		* Applied force = -150 units/second² (braking)
		* deltaTime = 0.016 seconds (60 FPS)
		* Left side: ( -150 * 0.016 )² = ( -2.4 )² = 5.76 Right side: 100² = 10,000
		*
		* Since 5.76 < 10,000, the condition is false - we continue braking.
		*
		* But if:
		* Current velocity = 2 units/second forward
		*
		*
		* Same braking force and deltaTime
		* Left side: 5.76 (same) Right side: 2² = 4
		*  Since 5.76 > 4, the condition is true - we'd overshoot zero, so we stop immediately.
		 *
		 */
		
		// Braking / Friction
		// If no input, apply a force opposite to velocity
		if (!CurrentVelocity.IsNearlyZero())
		{
			FVector FrictionDir = -CurrentVelocity.GetSafeNormal();
			AppliedForce = FrictionDir * HeavyDeceleration;
            
			// Prevent overshoot (jitter at 0 speed)
			if ((AppliedForce * deltaTime).SizeSquared() > CurrentVelocity.SizeSquared())
			{
				CurrentVelocity = FVector::ZeroVector;
				AppliedForce = FVector::ZeroVector;
			}
		}

		// Apply the force
		Velocity = CurrentVelocity + (AppliedForce * deltaTime);

		// TODO: Clamp to Max Speed (extensible for Encumbrance later)
		float CurrentMaxSpeed = HeavyMaxSpeed; // * GetEncumbranceMultiplier();
		if (Velocity.Size() > CurrentMaxSpeed)
		{
			Velocity = Velocity.GetSafeNormal() * CurrentMaxSpeed;
		}

		// D. Move the Component
		// SafeMove handles collision, sliding along walls, and stepping up stairs.
		FVector Delta = Velocity * deltaTime;
		FHitResult Hit(1.f);
    
		SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentRotation(), true, Hit);

		// Handle Wall Sliding
		if (Hit.IsValidBlockingHit())
		{
			HandleImpact(Hit, deltaTime, Delta);
			SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
		}

		// Handle Floor sticking (keep us on the ground)
		FFindFloorResult FloorResult;
		FindFloor(UpdatedComponent->GetComponentLocation(), FloorResult, false);
		if (FloorResult.IsWalkableFloor())
		{
			//TODO: Review Floor snapping
			// Basic snap-to-floor logic usually goes here or is handled by SafeMove
			// For simplicity in this snippet, we assume flat ground or simple slopes.
		}
	}
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

void FSavedMove_Heavy::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
}

FNetworkPredictionData_Client_Heavy::FNetworkPredictionData_Client_Heavy(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_Heavy::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Heavy());
}