// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FootstepAudioSystem.h"
#include "FootstepTypes.h"

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

	// === Combat Stance ===

	/** Is character in combat stance? (RMB held) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsInCombatStance = false;

	/** True when the guard animation has blended in enough to allow attacking */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsReadyToAttack = false;


	/** Speed multiplier when moving in combat stance (0.3 = 30% of normal speed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CombatMovementSpeedMultiplier = 0.4f;
	/** Should character face mouse cursor? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bFaceMouseCursor = false;


	// === Footstep Audio ===

	// Called by animation notifies
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayFootstepSound(EFootType FootType);

protected:
	// === General ===
	virtual void BeginPlay() override;


	// === Footstep Audio ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep Audio")
	UFootstepAudioSystem* FootstepAudioSystem;

	// === Camera Panning ===

	/** Maximum distance camera can pan from character center (in world units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Panning")
	float MaxCameraPanDistance = 400.0f;

	/** How quickly camera moves to target position (higher = snappier) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Panning",
		meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float CameraPanSpeed = 5.0f;

	/** Dead zone in center of screen where camera doesn't pan (0.0 - 0.5) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Panning",
		meta = (ClampMin = "0.0", ClampMax = "0.5"))
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


	// === Turn Input ===

	/** Input action for turning (arrow keys) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* TurnAction;

	/** Desired turn direction from input (-1 = left, 1 = right, 0 = none) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	float TurnInput = 0.f;

	/** How fast to turn when using arrow keys (degrees per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ManualTurnRate = 180.0f;

	void HandleTurn(const FInputActionValue& Value);

	// === Combat Stance ===

	/** Input action for combat stance */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* CombatStanceAction;

	void CombatStancePressed();
	void CombatStanceReleased();
	void UpdateMouseFacing(float DeltaTime);

	// == Stamina System ===
	/** Maximum stamina pool */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
	float MaxStamina = 100.0f;

	/** Current stamina (replicated for multiplayer) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentStamina, Category = "Stamina")
	float CurrentStamina = 100.0f;

	/** Stamina drain per second while sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
	float StaminaDrainRate = 20.0f;

	/** Stamina regeneration per second while not sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
	float StaminaRegenRate = 15.0f;

	/** Minimum stamina required to start sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
	float MinStaminaToSprint = 10.0f;

	/** Stamina must reach this threshold to recover from exhaustion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
	float ExhaustionRecoveryThreshold = 30.0f;


	/** Sprint input action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SprintAction;

	/** Called when stamina changes (for UI updates) */
	UFUNCTION()
	void OnRep_CurrentStamina();

	/** Input callbacks */
	void SprintPressed();
	void SprintReleased();

	// === Animation Data ===

	/** Actor's yaw rotation from previous frame (for turn-in-place) */
	FRotator PreviousRotation;

	/** Update rotation tracking for animation */
	void UpdateRotationTracking(float DeltaTime);

	// === Combat Anims ===
	/** Reference to the single-frame guard montage */
	UPROPERTY(EditDefaultsOnly, Category = "Combat Animations")
	UAnimMontage* RaiseFistsMontage;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Can the character sprint right now? */
	UFUNCTION(BlueprintPure, Category = "Stamina")
	bool CanSprint() const;

	/** Get stamina as a percentage (0-1) for UI */
	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetStaminaPercent() const { return MaxStamina > 0.f ? CurrentStamina / MaxStamina : 0.f; }

	/** Network replication */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Is the player exhausted? (replicated) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Stamina")
	bool bIsExhausted = false;

	/** Does the player want to sprint? (input state) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stamina")
	bool bWantsToSprint = false;

	/** Update stamina (called by movement component) */
	void UpdateStamina(float DeltaTime, bool bIsSprinting);


	// === Anim Data ===
	/** How fast character is turning (degrees per second) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float YawDelta = 0.f;

	/** Absolute yaw speed for animation (always positive) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	float AbsYawDelta = 0.f;

private:
	// === Footstep Audio ===

	ESurfaceType GetSurfaceTypeUnderFoot() const;
};
