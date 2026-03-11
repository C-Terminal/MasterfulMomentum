#include "FootstepAudioSystem.h"

UFootstepAudioSystem::UFootstepAudioSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFootstepAudioSystem::PlayFootstep(const FFootstepContext& Context)
{
	PlayFootstepImplementation(Context);
}