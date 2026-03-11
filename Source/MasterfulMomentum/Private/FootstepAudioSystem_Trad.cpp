#include "FootstepAudioSystem_Trad.h"
#include "Kismet/GameplayStatics.h"

UFootstepAudioSystem_Trad::UFootstepAudioSystem_Trad()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UFootstepAudioSystem_Trad::PlayFootstepImplementation(const FFootstepContext& Context)
{
    USoundBase* Sound = GetSoundForContext(Context);
    
    if (!Sound)
    {
        #if !UE_BUILD_SHIPPING
        UE_LOG(LogTemp, Warning, TEXT("No footstep sound found for surface type"));
        #endif
        return;
    }

    float Volume = CalculateVolume(Context);
    float Pitch = CalculatePitch(Context);

    UGameplayStatics::PlaySoundAtLocation(
        this,
        Sound,
        Context.Location,
        Volume,
        Pitch
    );

    #if !UE_BUILD_SHIPPING
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan,
            FString::Printf(TEXT("Footstep: %s | Vol: %.2f | Pitch: %.2f"),
                *UEnum::GetValueAsString(Context.SurfaceType), Volume, Pitch));
    }
    #endif
}

USoundBase* UFootstepAudioSystem_Trad::GetSoundForContext(const FFootstepContext& Context) const
{
    // Priority: Combat > Sprint > Walk
    const TMap<ESurfaceType, USoundBase*>* SoundMap = nullptr;

    if (Context.bIsInCombat && CombatSounds.Num() > 0)
    {
        SoundMap = &CombatSounds;
    }
    else if (Context.bIsSprinting && SprintSounds.Num() > 0)
    {
        SoundMap = &SprintSounds;
    }
    else
    {
        SoundMap = &WalkSounds;
    }

    // Find sound for this surface type
    if (SoundMap && SoundMap->Contains(Context.SurfaceType))
    {
        return (*SoundMap)[Context.SurfaceType];
    }

    // Fallback to default surface
    if (SoundMap && SoundMap->Contains(ESurfaceType::Default))
    {
        return (*SoundMap)[ESurfaceType::Default];
    }

    return nullptr;
}

float UFootstepAudioSystem_Trad::CalculateVolume(const FFootstepContext& Context) const
{
    float Volume = BaseVolume;

    // Reduce volume when low on stamina (exhausted = quieter steps)
    float StaminaPenalty = (1.0f - Context.StaminaPercent) * StaminaToVolumeMultiplier;
    Volume *= (1.0f - StaminaPenalty);

    // Sprint is louder
    if (Context.bIsSprinting)
    {
        Volume *= 1.2f;
    }

    return FMath::Clamp(Volume, 0.1f, 2.0f);
}

float UFootstepAudioSystem_Trad::CalculatePitch(const FFootstepContext& Context) const
{
    float Pitch = BasePitch;

    // Faster movement = slightly higher pitch
    float VelocityModulation = Context.Velocity * VelocityToPitchMultiplier;
    Pitch += VelocityModulation;

    // Combat stance = slightly lower, heavier pitch
    if (Context.bIsInCombat)
    {
        Pitch *= 0.95f;
    }

    return FMath::Clamp(Pitch, 0.8f, 1.5f);
}