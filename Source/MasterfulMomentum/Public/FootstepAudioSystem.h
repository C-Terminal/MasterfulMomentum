#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FootstepTypes.h"
#include "FootstepAudioSystem.generated.h"

/**
 * Base class for footstep audio systems
 * Allows swapping between traditional and parametric implementations
 */
UCLASS(Abstract, Blueprintable)
class MASTERFULMOMENTUM_API UFootstepAudioSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UFootstepAudioSystem();

	// Called when a footstep should play
	UFUNCTION(BlueprintCallable, Category = "Audio")
	virtual void PlayFootstep(const FFootstepContext& Context);

protected:
	// Override in child classes for different implementations
	virtual void PlayFootstepImplementation(const FFootstepContext& Context) PURE_VIRTUAL(UFootstepAudioSystem::PlayFootstepImplementation, );
};