// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FootstepAudioSystem.h"
#include "FootstepTypes.h"
#include "InputActionValue.h"
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

	// === Camera Zoom ===

	/** Current target arm length (what we're lerping towards) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Zoom")
	float TargetArmLength = 600.0f;

	/** Minimum zoom distance (closest to character) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom",
		meta = (ClampMin = "100.0", ClampMax = "1000.0"))
	float MinZoomDistance = 200.0f;

	/** Maximum zoom distance (farthest from character) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom",
		meta = (ClampMin = "200.0", ClampMax = "2000.0"))
	float MaxZoomDistance = 1200.0f;

	/** Default zoom distance (starting position) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	float DefaultZoomDistance = 600.0f;

	/** How fast camera zooms in/out */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float ZoomSpeed = 8.0f;

	/** How much each mouse wheel notch zooms (units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom",
		meta = (ClampMin = "10.0", ClampMax = "200.0"))
	float ZoomIncrement = 100.0f;

	/** Should zoom be smooth or instant? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	bool bSmoothZoom = true;

	// To this:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* ZoomAction;

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

	// === Camera Collision ===

	/** Enable camera collision detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Collision")
	bool bEnableCameraCollision = true;

	/** Collision channel to use for camera traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Collision")
	TEnumAsByte<ECollisionChannel> CameraCollisionChannel = ECC_Camera;

	/** Radius of camera collision sphere */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Collision",
		meta = (ClampMin = "5.0", ClampMax = "50.0"))
	float CameraCollisionProbeSize = 12.0f;

	/** How quickly camera returns to normal position after collision */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Collision",
		meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float CameraCollisionRecoverySpeed = 5.0f;

	// === Camera Occlusion (Fade Walls) ===

	/** Enable automatic wall fading */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Occlusion")
	bool bEnableOcclusionFading = true;

	/** How transparent occluding actors become (0 = invisible, 1 = opaque) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Occlusion",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OcclusionFadeOpacity = 0.2f;

	/** How fast actors fade in/out */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Occlusion",
		meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float OcclusionFadeSpeed = 8.0f;

	/** Actors currently being faded */
	UPROPERTY()
	TArray<AActor*> OccludedActors;

	/** Material parameter name for opacity control */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Occlusion")
	FName OpacityParameterName = "Opacity";

	// === Camera Shake ===

	/** Camera shake class for landing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Shake")
	TSubclassOf<UCameraShakeBase> LandingCameraShake;

	/** Minimum fall velocity to trigger camera shake (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Shake",
		meta = (ClampMin = "100.0", ClampMax = "2000.0"))
	float MinShakeVelocity = 400.0f;

	/** Maximum fall velocity for max shake intensity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Shake",
		meta = (ClampMin = "500.0", ClampMax = "3000.0"))
	float MaxShakeVelocity = 1500.0f;

	/** Scale applied to camera shake */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Shake", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float ShakeIntensityMultiplier = 1.0f;


	// === Camera Update Functions ===

	/** Handle zoom input */
	void HandleZoom(const FInputActionValue& Value);

	/** Update camera zoom smoothly */
	void UpdateCameraZoom(float DeltaTime);

	/** Check for camera collision and adjust position */
	void UpdateCameraCollision(float DeltaTime);

	/** Fade actors blocking the camera view */
	void UpdateOcclusionFading(float DeltaTime);


	/** Set material opacity for an actor */
	void SetActorOpacity(AActor* Actor, float Opacity);

	/** Restore material opacity for an actor */
	void RestoreActorOpacity(AActor* Actor);

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

	//Camera

	/** Trigger camera shake on landing */
	void TriggerLandingCameraShake(float ImpactVelocity);
	/** Get current zoom percentage (0-1, where 0 = min zoom, 1 = max zoom) */
	UFUNCTION(BlueprintPure, Category = "Camera")
	float GetZoomPercent() const;

	/** Reset zoom to default distance */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ResetZoom();

	/** Velocity when character started falling (for landing impact calculation) */
	float FallStartVelocity = 0.f;

private:
	// === Footstep Audio ===

	ESurfaceType GetSurfaceTypeUnderFoot() const;
};
