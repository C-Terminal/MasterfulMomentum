// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FootstepAudioSystem.h"
#include "FootstepAudioSystem_Trad.generated.h"

/**
 * 
 */
UCLASS()
class MASTERFULMOMENTUM_API UFootstepAudioSystem_Trad : public UFootstepAudioSystem
{
	GENERATED_BODY()

public:
	UFootstepAudioSystem_Trad();
	
protected:
	
	virtual void PlayFootstepImplementation(const FFootstepContext& Context) override;
	
private:
	// Sound mappings by surface type
	UPROPERTY(EditDefaultsOnly, Category = "Footsteps|Sounds")
	TMap<ESurfaceType, USoundBase*> WalkSounds;

	UPROPERTY(EditDefaultsOnly, Category = "Footsteps|Sounds")
	TMap<ESurfaceType, USoundBase*> SprintSounds;

	UPROPERTY(EditDefaultsOnly, Category = "Footsteps|Sounds")
	TMap<ESurfaceType, USoundBase*> CombatSounds;

	// Volume/pitch modulation
	UPROPERTY(EditDefaultsOnly, Category = "Footsteps|Modulation", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float BaseVolume = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Footsteps|Modulation", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float BasePitch = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Footsteps|Modulation")
	float VelocityToPitchMultiplier = 0.002f; // How much speed affects pitch

	UPROPERTY(EditDefaultsOnly, Category = "Footsteps|Modulation")
	float StaminaToVolumeMultiplier = 0.3f; // How much low stamina reduces volume

	// Helper functions
	USoundBase* GetSoundForContext(const FFootstepContext& Context) const;
	float CalculateVolume(const FFootstepContext& Context) const;
	float CalculatePitch(const FFootstepContext& Context) const;
};
