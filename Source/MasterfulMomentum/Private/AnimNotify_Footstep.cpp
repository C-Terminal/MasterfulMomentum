#include "AnimNotify_Footstep.h"
#include "HeavyCharacter.h"
#include "DrawDebugHelpers.h"

UAnimNotify_Footstep::UAnimNotify_Footstep()
{
	// Set default notify color in timeline
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(51, 153, 255, 255); // Nice blue color for footsteps
#endif
}

void UAnimNotify_Footstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}

	// Try to cast to our character type
	AHeavyCharacter* Character = Cast<AHeavyCharacter>(MeshComp->GetOwner());
	if (!Character)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("AnimNotify_Footstep: Owner is not AHeavyCharacter!"));
#endif
		return;
	}

	// Route through character's audio system
	Character->PlayFootstepSound(FootType);

	// Debug visualization
#if !UE_BUILD_SHIPPING
	if (bDebugDrawLocation)
	{
		FVector Location = Character->GetActorLocation();
		FColor DebugColor = (FootType == EFootType::Left) ? FColor::Green : FColor::Red;
        
		DrawDebugSphere(
			Character->GetWorld(),
			Location,
			25.0f,
			12,
			DebugColor,
			false,
			2.0f,
			0,
			2.0f
		);
	}
#endif
}

FString UAnimNotify_Footstep::GetNotifyName_Implementation() const
{
	// Show which foot in the notify name in editor
	FString FootName = (FootType == EFootType::Left) ? TEXT("Left") : TEXT("Right");
	return FString::Printf(TEXT("Footstep (%s)"), *FootName);
}