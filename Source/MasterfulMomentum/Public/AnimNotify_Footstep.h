#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "FootstepTypes.h"
#include "AnimNotify_Footstep.generated.h"

/**
 * Animation notify for triggering character footstep sounds
 * Routes through character's audio system for proper context-aware playback
 */
UCLASS( hidecategories=Object, collapsecategories, meta=(DisplayName="Footstep"))
class MASTERFULMOMENTUM_API UAnimNotify_Footstep : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_Footstep();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
    
	// Override to show our custom name in the editor
	virtual FString GetNotifyName_Implementation() const override;

#if WITH_EDITOR
	// Change the color in the animation timeline for easy identification
	virtual FLinearColor GetEditorColor() override { return FLinearColor(0.2f, 0.6f, 1.0f); }
#endif

protected:
	/** Which foot is making contact */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (DisplayName = "Foot"))
	EFootType FootType = EFootType::Left;

	/** Optional: Override for specific surface type (leave as Default to auto-detect) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Advanced")
	ESurfaceType SurfaceOverride = ESurfaceType::Default;

	/** Optional: Volume multiplier for this specific notify (useful for crouched steps, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Advanced", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float VolumeMultiplier = 1.0f;

	/** Enable to spawn debug sphere at footstep location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Debug")
	bool bDebugDrawLocation = false;
};