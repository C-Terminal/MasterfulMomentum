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
    CustomInputVector = FVector::ZeroVector; // Clear it for next frame
    
    UE_LOG(LogTemp, Warning, TEXT("CustomInputVector: %s"), *InputVector.ToString());

    if (!InputVector.IsNearlyZero())
    {
       InputVector = InputVector.GetSafeNormal();
    }

    bool bHasInput = !InputVector.IsNearlyZero();
    UE_LOG(LogTemp, Warning, TEXT("bHasInput: %s"), bHasInput ? TEXT("TRUE") : TEXT("FALSE"));
    
    // --- B. Calculate Rotation (Heavy Turning) ---
    if (!InputVector.IsNearlyZero())
    {
       FRotator CurrentRotation = UpdatedComponent->GetComponentRotation();
       FRotator TargetRotation = InputVector.Rotation();

       // Use constant interpolation for a "tanky" or heavy human feel
       FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, TargetRotation, deltaTime, HeavyTurnRate);

       MoveUpdatedComponent(FVector::ZeroVector, NewRotation, false);
    }
    
    FVector CurrentVelocity = Velocity;
    FVector AppliedForce = FVector::ZeroVector;

    if (bHasInput)
    {
       float SurfaceMult = 1.0f;
       // if (CurrentFloor.IsWalkableFloor()) 
       // {
       //    // Dynamic traction based on the physical material of the floor
       //    UPhysicalMaterial* PhysMat = CurrentFloor.HitResult.PhysMaterial.Get();
       //    if (PhysMat) SurfaceMult = PhysMat->Friction;
       // }

       // Acceleration Force
       AppliedForce = InputVector * HeavyAcceleration * SurfaceMult;
       UE_LOG(LogTemp, Warning, TEXT("AppliedForce: %s"), *AppliedForce.ToString());
    }
    else
    {
       // Braking / Friction Force
       if (!CurrentVelocity.IsNearlyZero())
       {
          FVector FrictionDir = -CurrentVelocity.GetSafeNormal();
          AppliedForce = FrictionDir * HeavyDeceleration;

          // Prevent overshoot: if the braking force would flip direction, just stop.
          if ((AppliedForce * deltaTime).SizeSquared() > CurrentVelocity.SizeSquared())
          {
             CurrentVelocity = FVector::ZeroVector;
             AppliedForce = FVector::ZeroVector;
          }
       }
    }

    // --- D. Final Integration & Movement ---
    // These lines must be outside the 'else' block so the character actually moves!

    // Update Velocity: v = v0 + at
    Velocity = CurrentVelocity + (AppliedForce * deltaTime);

    // Clamp to Max Speed (CurrentMaxSpeed can be modified by encumbrance later)
    float CurrentMaxSpeed = HeavyMaxSpeed;
    if (Velocity.Size() > CurrentMaxSpeed)
    {
       Velocity = Velocity.GetSafeNormal() * CurrentMaxSpeed;
    }

    // Calculate the distance to move this frame
    FVector Delta = Velocity * deltaTime;
    FHitResult Hit(1.f);

    // Use SafeMove to handle collisions and steps automatically
    SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentRotation(), true, Hit);

    // Handle Wall Sliding / Impacts
    if (Hit.IsValidBlockingHit())
    {
       HandleImpact(Hit, deltaTime, Delta);
       SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
    }

    // Ground Snapping
    FFindFloorResult FloorResult;
    FindFloor(UpdatedComponent->GetComponentLocation(), FloorResult, false);
    if (FloorResult.IsWalkableFloor())
    {
       // Additional ground-alignment logic can go here if needed
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
