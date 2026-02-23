#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HeavyAnimInstance.generated.h"

class AHeavyCharacter;
class UHeavyCharacterMovementComponent;

/**
 * Custom Animation Instance for Heavy Character
 * Caches character state for efficient animation blueprint usage
 */
UCLASS()
class MASTERFULMOMENTUM_API UHeavyAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UHeavyAnimInstance();

    /** Called when animation is initialized */
    virtual void NativeInitializeAnimation() override;

    /** Called every frame to update animation data */
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;


    /** Get the character's current movement state as a descriptive string (for debugging) */
    UFUNCTION(BlueprintPure, Category = "Animation")
    FString GetMovementStateString() const;
    
    /** Should play exhausted breathing animation? */
    UFUNCTION(BlueprintPure, Category = "Animation")
    bool ShouldPlayExhaustedBreathing() const { return bIsExhausted || bIsStaminaLow; }
    
    /** Get sprint speed multiplier for animation playback rate */
    UFUNCTION(BlueprintPure, Category = "Animation")
    float GetAnimationPlayRateMultiplier() const { return bIsSprinting ? 1.3f : 1.0f; }

protected:
    // === Cached References ===
    
    /** Reference to the owning Heavy Character (cached for performance) */
    UPROPERTY(BlueprintReadOnly, Category = "Character")
    AHeavyCharacter* HeavyCharacter;

    /** Reference to the movement component (cached for performance) */
    UPROPERTY(BlueprintReadOnly, Category = "Character")
    UHeavyCharacterMovementComponent* HeavyMovement;

    // === Movement State ===
    
    /** Is the character moving? */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsMoving = false;

    /** Character's current ground speed (horizontal velocity magnitude) */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    float GroundSpeed = 0.f;

    /** Character's velocity as a vector (for directional blending) */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    FVector Velocity = FVector::ZeroVector;

    /** Movement direction relative to character rotation (-180 to 180) */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    float Direction = 0.f;

    /** Is the character in the air? */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsInAir = false;

    /** Is the character currently sprinting? */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsSprinting = false;
    
    /** Is Character in combat/ready stance? */
    UPROPERTY(BlueprintReadOnly, Category= "Combat")
    bool bIsInCombatStance = false;

    // === Rotation ===
    
    /** How fast the character is turning (degrees per second) */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    float YawDelta = 0.f;

    /** Absolute yaw speed (always positive, for blend spaces) */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    float AbsYawDelta = 0.f;

    /** Should play turn-in-place animation? (turning while not moving) */
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bShouldTurnInPlace = false;

    // === Stamina ===
    
    /** Current stamina percentage (0-1) */
    UPROPERTY(BlueprintReadOnly, Category = "Stamina")
    float StaminaPercent = 1.0f;

    /** Is the character exhausted? */
    UPROPERTY(BlueprintReadOnly, Category = "Stamina")
    bool bIsExhausted = false;

    /** Is stamina low? (below 30%) - can trigger breathing animations */
    UPROPERTY(BlueprintReadOnly, Category = "Stamina")
    bool bIsStaminaLow = false;

    // === Configuration ===
    
    /** Minimum speed to be considered "moving" (prevents animation jitter) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Config")
    float MovingSpeedThreshold = 10.0f;

    /** Minimum yaw delta to trigger turn-in-place (degrees per second) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Config")
    float TurnInPlaceThreshold = 45.0f;

    /** Stamina threshold for "low stamina" state (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Config")
    float LowStaminaThreshold = 0.3f;

private:
    /** Update all cached animation variables */
    void UpdateAnimationData(float DeltaSeconds);
};