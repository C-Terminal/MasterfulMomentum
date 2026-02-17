// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HeavyCharacterMovementComponent.generated.h"

// Define our custom movement mode ID
UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_HeavyGrounded UMETA(DisplayName = "Heavy Grounded"),
	CMOVE_MAX UMETA(Hidden),
};

/**
 * 
 */
UCLASS()
class MASTERFULMOMENTUM_API UHeavyCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	UHeavyCharacterMovementComponent();
	// --- Configuration ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heavy Movement")
	float HeavyMaxSpeed = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heavy Movement")
	float HeavyAcceleration = 600.f; // Low value = heavy feel

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heavy Movement")
	float HeavyDeceleration = 400.f; // Low value = sliding stop

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heavy Movement")
	float HeavyTurnRate = 120.f; // Degrees per second

protected:
	// --- Core CMC Overrides ---
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	void PhysHeavyGrounded(float deltaTime, int32 Iterations);

	// Network Prediction Boilerplate
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

public:
	// Helper to switch modes easily
	UFUNCTION(BlueprintCallable, Category = "Heavy Movement")
	void SetHeavyModeEnabled(bool bEnabled);
};
	//// --- Networking Support Structs ---
	class FSavedMove_Heavy : public FSavedMove_Character
	{
	public:
		typedef FSavedMove_Character Super;

		// Check if new move is same as old (optimization)
		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
    
		// Pack data for server
		virtual uint8 GetCompressedFlags() const override;
    
		// Unpack data on server
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
    
		// Reset
		virtual void Clear() override;
	};

	class FNetworkPredictionData_Client_Heavy : public FNetworkPredictionData_Client_Character
	{
	public:
		typedef FNetworkPredictionData_Client_Character Super;
		FNetworkPredictionData_Client_Heavy(const UCharacterMovementComponent& ClientMovement);
		virtual FSavedMovePtr AllocateNewMove() override;
	};





