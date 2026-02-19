// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HeavyCharacterMovementComponent.h"
#include "HeavyCharacter.generated.h"

UCLASS()
class MASTERFULMOMENTUM_API AHeavyCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	// This runs when the pawn is possessed and ready for input

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* HeavyMovementContext;
	// Sets default values for this character's properties
	// AHeavyCharacter();
	AHeavyCharacter(const FObjectInitializer& ObjectInitializer);

	// Helper getter
	UFUNCTION(BlueprintPure, Category = "Heavy Movement")
	UHeavyCharacterMovementComponent* GetHeavyMovement() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* FollowCamera;
	// A pointer to your IMC asset so you can assign it in the Editor
	
protected:
	// === General ===
	virtual void BeginPlay() override;
	
	// === Camera Panning ===
    
	/** Maximum distance camera can pan from character center (in world units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Panning")
	float MaxCameraPanDistance = 400.0f;

	/** How quickly camera moves to target position (higher = snappier) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Panning", meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float CameraPanSpeed = 5.0f;

	/** Dead zone in center of screen where camera doesn't pan (0.0 - 0.5) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Panning", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float CameraPanDeadZone = 0.1f;

	/** Should camera panning be enabled? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Panning")
	bool bEnableCameraPanning = true;

	/** Current camera offset (smoothed) */
	FVector CurrentCameraOffset;

	/** Calculate camera pan offset based on mouse position */
	void UpdateCameraPan(float DeltaTime);

	/** Get normalized mouse position relative to screen center (-1 to 1) */
	bool GetNormalizedMousePosition(FVector2D& OutNormalizedPosition) const;

	// === Movement ===
	void ForceMovement();

	// Bind this to IA_HeavyMove in the BP details
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	/**
	 * 
	 * @param Value should be Axis2D 
	 */
	void HandleMove(const struct FInputActionValue& Value);

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PossessedBy(AController* NewController) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input

};
