// Fill out your copyright notice in the Description page of Project Settings.

#include "MasterfulMomentum/Public/HeavyCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "FootstepAudioSystem_Trad.h"
#include "HeavyAnimInstance.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"


class UHeavyCharacterMovementComponent;
// Sets default values
AHeavyCharacter::AHeavyCharacter(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer.SetDefaultSubobjectClass<UHeavyCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Default to not using controller rotation, we want the heavy movement to handle it
	bUseControllerRotationYaw = false;

	// Inside AHeavyCharacter::AHeavyCharacter constructor:
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = DefaultZoomDistance; // Distance behind character
	CameraBoom->bUsePawnControlRotation = true; // Rotate arm based on controller
	CameraBoom->bDoCollisionTest = false; // We'll handle collision manually for better control


	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm


	// Initialize zoom
	TargetArmLength = DefaultZoomDistance;
	// In constructor:
	FootstepAudioSystem = CreateDefaultSubobject<UFootstepAudioSystem_Trad>(TEXT("FootstepAudioSystem"));
}

UHeavyCharacterMovementComponent* AHeavyCharacter::GetHeavyMovement() const
{
	return Cast<UHeavyCharacterMovementComponent>(GetCharacterMovement());
}

// Called when the game starts or when spawned
void AHeavyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Initialize stamina to max
	CurrentStamina = MaxStamina;

	// Initialize rotation tracking
	PreviousRotation = GetActorRotation();

	// Verify assets are assigned
	UE_LOG(LogTemp, Warning, TEXT("MoveAction valid: %s"), MoveAction ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Warning, TEXT("HeavyMovementContext valid: %s"), HeavyMovementContext ? TEXT("YES") : TEXT("NO"));

	// Set initial camera distance
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = DefaultZoomDistance;
		TargetArmLength = DefaultZoomDistance;
	}

	// Force the character into your Project Zomboid-style movement mode
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Custom, 0); // 0 is your CMOVE_HeavyGrounded
		UE_LOG(LogTemp, Warning, TEXT("Movement Mode forced to HeavyGrounded"));
	}
}


void AHeavyCharacter::ForceMovement()
{
	// Force move test: Push forward automatically for 5 seconds
	if (GetWorld()->GetTimeSeconds() < 5.0f)
	{
		// We call AddMovementInput directly, bypassing WASD
		// This populates the 'PendingInputVector' that your component consumes
		AddMovementInput(GetActorForwardVector(), 1.0f);

		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, TEXT("FORCE MOVING..."));
	};
}

void AHeavyCharacter::UpdateRotationTracking(float DeltaTime)
{
	if (DeltaTime <= 0.f)
	{
		return;
	}

	// Get current rotation
	FRotator CurrentRotation = GetActorRotation();

	// Calculate yaw difference from last frame
	float YawDifference = CurrentRotation.Yaw - PreviousRotation.Yaw;

	// Normalize to [-180, 180] range (handles wrapping at 360/-360)
	YawDifference = FMath::UnwindDegrees(YawDifference);

	// Convert to degrees per second
	YawDelta = YawDifference / DeltaTime;

	// Store absolute value for animation blending (turn speed regardless of direction)
	AbsYawDelta = FMath::Abs(YawDelta);

	// Cache current rotation for next frame
	PreviousRotation = CurrentRotation;
}

// Called every frame
void AHeavyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	if (GetCharacterMovement())
	{
		EMovementMode CurrentMode = GetCharacterMovement()->MovementMode;
		uint8 CustomMode = GetCharacterMovement()->CustomMovementMode;
		GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Yellow,
		                                 FString::Printf(
			                                 TEXT("MovementMode: %d, CustomMode: %d"), CurrentMode, CustomMode));
	}

	if (bEnableCameraPanning)
	{
		UpdateCameraPan(DeltaTime);
	}

	// Update rotation tracking for animations
	UpdateRotationTracking(DeltaTime);

	// NEW: Apply manual turning
	if (FMath::Abs(TurnInput) > 0.01f)
	{
		FRotator CurrentRotation = GetActorRotation();
		FRotator NewRotation = CurrentRotation;
		NewRotation.Yaw += TurnInput * ManualTurnRate * DeltaTime;
		SetActorRotation(NewRotation);
	}

	// Update mouse-based facing
	UpdateMouseFacing(DeltaTime);

	// === NEW CAMERA SYSTEMS ===
	UpdateCameraZoom(DeltaTime);
	UpdateCameraCollision(DeltaTime);
	UpdateOcclusionFading(DeltaTime);

	// Stamina update is now handled in the movement component
	// We just keep the debug display here
#if !UE_BUILD_SHIPPING
	if (GEngine && CurrentStamina < MaxStamina)
	{
		FColor StaminaColor = bIsExhausted
			                      ? FColor::Red
			                      : (CurrentStamina < MinStaminaToSprint ? FColor::Orange : FColor::Green);
		GEngine->AddOnScreenDebugMessage(12, 0.f, StaminaColor,
		                                 FString::Printf(TEXT("Stamina: %.0f%% %s"),
		                                                 GetStaminaPercent() * 100.f,
		                                                 bIsExhausted ? TEXT("[EXHAUSTED]") : TEXT("")));
	}
	// Debug yaw delta
	if (AbsYawDelta > 1.0f) // Only show when turning
	{
		GEngine->AddOnScreenDebugMessage(14, 0.f, FColor::Magenta,
		                                 FString::Printf(TEXT("Yaw Delta: %.1f deg/s"), YawDelta));
	}

	// Camera debug
	if (CameraBoom)
	{
		GEngine->AddOnScreenDebugMessage(18, 0.f, FColor::Cyan,
		                                 FString::Printf(TEXT("Zoom: %.0f%% (%.0f units)"),
		                                                 GetZoomPercent() * 100.f, CameraBoom->TargetArmLength));
	}
#endif
}

void AHeavyCharacter::UpdateCameraPan(float DeltaTime)
{
	if (!CameraBoom || !FollowCamera)
	{
		return;
	}

	// Get normalized mouse position
	FVector2D NormalizedMouse;
	if (!GetNormalizedMousePosition(NormalizedMouse))
	{
		// If we can't get mouse position, smoothly return camera to center
		CurrentCameraOffset = FMath::VInterpTo(CurrentCameraOffset, FVector::ZeroVector, DeltaTime, CameraPanSpeed);
		CameraBoom->SocketOffset = CurrentCameraOffset;
		return;
	}

	// Apply dead zone
	FVector2D PanInput = NormalizedMouse;
	if (FMath::Abs(PanInput.X) < CameraPanDeadZone)
	{
		PanInput.X = 0.0f;
	}
	else
	{
		PanInput.X = (FMath::Abs(PanInput.X) - CameraPanDeadZone) / (1.0f - CameraPanDeadZone) *
			FMath::Sign(PanInput.X);
	}

	if (FMath::Abs(PanInput.Y) < CameraPanDeadZone)
	{
		PanInput.Y = 0.0f;
	}
	else
	{
		PanInput.Y = (FMath::Abs(PanInput.Y) - CameraPanDeadZone) / (1.0f - CameraPanDeadZone) *
			FMath::Sign(PanInput.Y);
	}

	// === FIXED: Use Controller/Camera rotation, not character rotation ===

	// Get the controller's yaw rotation (or camera's fixed rotation if you prefer)
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	const FRotator ControlRotation = PC->GetControlRotation();

	// Get right and forward vectors based on CAMERA view, not character facing
	FVector CameraRight = FRotationMatrix(FRotator(0, ControlRotation.Yaw, 0)).GetUnitAxis(EAxis::Y);
	FVector CameraForward = FRotationMatrix(FRotator(0, ControlRotation.Yaw, 0)).GetUnitAxis(EAxis::X);

	// Calculate desired offset in world space
	// Screen X = Camera Right, Screen Y (inverted) = Camera Forward
	FVector DesiredOffset = (CameraRight * PanInput.X * MaxCameraPanDistance) +
		(CameraForward * -PanInput.Y * MaxCameraPanDistance);

	// Smoothly interpolate to desired offset
	CurrentCameraOffset = FMath::VInterpTo(CurrentCameraOffset, DesiredOffset, DeltaTime, CameraPanSpeed);

	// Apply to spring arm
	CameraBoom->SocketOffset = CurrentCameraOffset;

	// Optional: Debug visualization
#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(50, 0.0f, FColor::Cyan,
		                                 FString::Printf(TEXT("Camera Offset: %s"), *CurrentCameraOffset.ToString()));
		GEngine->AddOnScreenDebugMessage(51, 0.0f, FColor::Yellow,
		                                 FString::Printf(
			                                 TEXT("Mouse Normalized: X=%.2f Y=%.2f"), NormalizedMouse.X,
			                                 NormalizedMouse.Y));
	}
#endif
}

bool AHeavyCharacter::GetNormalizedMousePosition(FVector2D& OutNormalizedPosition) const
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return false;
	}

	float MouseX, MouseY;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	// Get viewport size
	int32 ViewportSizeX, ViewportSizeY;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return false;
	}

	// Normalize to [-1, 1] range with (0,0) at screen center
	OutNormalizedPosition.X = (MouseX / (float)ViewportSizeX - 0.5f) * 2.0f;
	OutNormalizedPosition.Y = (MouseY / (float)ViewportSizeY - 0.5f) * 2.0f;

	return true;
}

// Called to bind functionality to input
void AHeavyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (HeavyMovementContext)
			{
				// Remove it first to prevent duplicates
				Subsystem->RemoveMappingContext(HeavyMovementContext);
				// Add with high priority
				Subsystem->AddMappingContext(HeavyMovementContext, 1);
				UE_LOG(LogTemp, Warning, TEXT("Input Mapping Context Added"));
			}
		}
	}

	if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHeavyCharacter::HandleMove);
			UE_LOG(LogTemp, Warning, TEXT("Successfully bound IA_HeavyMove in C++"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("MoveAction is NULL! Check BP Class Defaults."));
		}
		if (SprintAction)
		{
			EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &AHeavyCharacter::SprintPressed);
			EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &AHeavyCharacter::SprintReleased);
			UE_LOG(LogTemp, Warning, TEXT("Successfully bound IA_HeavySprint in C++"));
		}
		// NEW: Turn input
		if (TurnAction)
		{
			EIC->BindAction(TurnAction, ETriggerEvent::Triggered, this, &AHeavyCharacter::HandleTurn);
			EIC->BindAction(TurnAction, ETriggerEvent::Completed, this, &AHeavyCharacter::HandleTurn);
		}
		if (CombatStanceAction)
		{
			EIC->BindAction(CombatStanceAction, ETriggerEvent::Started, this, &AHeavyCharacter::CombatStancePressed);
			EIC->BindAction(CombatStanceAction, ETriggerEvent::Completed, this, &AHeavyCharacter::CombatStanceReleased);
		}
		// NEW: Zoom binding
		if (ZoomAction)
		{
			EIC->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AHeavyCharacter::HandleZoom);
			UE_LOG(LogTemp, Warning, TEXT("Zoom action bound successfully"));
		}
	}
}

void AHeavyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	UE_LOG(LogTemp, Warning, TEXT("Character Possessed by: %s"), *NewController->GetName());
}

void AHeavyCharacter::HandleTurn(const FInputActionValue& Value)
{
	// Get turn input (-1 to 1, where -1 = left, 1 = right)
	TurnInput = Value.Get<float>();

#if !UE_BUILD_SHIPPING
	if (TurnInput != 0.f)
	{
		GEngine->AddOnScreenDebugMessage(15, 0.f, FColor::Orange,
		                                 FString::Printf(TEXT("Turn Input: %.2f"), TurnInput));
	}
#endif
}

void AHeavyCharacter::HandleMove(const struct FInputActionValue& Value)
{
	GEngine->AddOnScreenDebugMessage(1, 1.f, FColor::Cyan, TEXT("!!! HANDLE MOVE EXECUTING !!!"));

	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		if (UHeavyCharacterMovementComponent* HeavyMovement = GetHeavyMovement())
		{
			FVector InputDir = (ForwardDirection * MovementVector.Y) + (RightDirection * MovementVector.X);
			HeavyMovement->CustomInputVector = InputDir;

#if !UE_BUILD_SHIPPING
			GEngine->AddOnScreenDebugMessage(20, 0.f, FColor::Magenta,
			                                 FString::Printf(
				                                 TEXT("CustomInputVector set to: %s"), *InputDir.ToString()));
#endif
		}

		UE_LOG(LogTemp, Log, TEXT("Input Received: X=%f, Y=%f"), MovementVector.X, MovementVector.Y);

#if !UE_BUILD_SHIPPING
		GEngine->AddOnScreenDebugMessage(1, 1.f, FColor::Cyan,
		                                 FString::Printf(TEXT("Current Velocity: %s"), *GetVelocity().ToString()));
#endif
	}
}

void AHeavyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeavyCharacter, CurrentStamina);
	DOREPLIFETIME(AHeavyCharacter, bIsExhausted);
}


void AHeavyCharacter::OnRep_CurrentStamina()
{
	// This gets called on clients when stamina replicates
	// Perfect place to update UI or play audio feedback

#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(11, 0.f, FColor::Cyan,
		                                 FString::Printf(TEXT("Stamina: %.1f / %.1f (%.0f%%)"),
		                                                 CurrentStamina, MaxStamina, GetStaminaPercent() * 100.f));
	}
#endif
}

void AHeavyCharacter::CombatStancePressed()
{
	bIsInCombatStance = true;
	bFaceMouseCursor = true;

#if !UE_BUILD_SHIPPING
	GEngine->AddOnScreenDebugMessage(16, 2.f, FColor::Green, TEXT("COMBAT STANCE: ON"));
#endif
}

void AHeavyCharacter::CombatStanceReleased()
{
	bIsInCombatStance = false;
	bFaceMouseCursor = false;
	bIsReadyToAttack = false;

#if !UE_BUILD_SHIPPING
	GEngine->AddOnScreenDebugMessage(16, 2.f, FColor::Red, TEXT("COMBAT STANCE: OFF"));
#endif
}

void AHeavyCharacter::UpdateMouseFacing(float DeltaTime)
{
	if (!bFaceMouseCursor)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// Get mouse position in world space
	FHitResult HitResult;
	PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	if (HitResult.bBlockingHit)
	{
		// Calculate direction to mouse cursor
		FVector ToMouse = HitResult.Location - GetActorLocation();
		ToMouse.Z = 0.f; // Keep rotation on horizontal plane

		if (!ToMouse.IsNearlyZero())
		{
			FRotator TargetRotation = ToMouse.Rotation();
			FRotator CurrentRotation = GetActorRotation();

			// Smoothly rotate towards mouse (faster than normal turning for responsiveness)
			FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation,
			                                        DeltaTime, 10.0f); // Fast rotation

			SetActorRotation(NewRotation);
		}
	}
}

void AHeavyCharacter::SprintPressed()
{
	bWantsToSprint = true;
}

void AHeavyCharacter::SprintReleased()
{
	bWantsToSprint = false;
}

void AHeavyCharacter::UpdateStamina(float DeltaTime, bool bIsSprinting)
{
	// Only update on server or in single player
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	const float OldStamina = CurrentStamina;

	if (bIsSprinting)
	{
		// Drain stamina
		CurrentStamina = FMath::Max(0.f, CurrentStamina - (StaminaDrainRate * DeltaTime));

		// Check for exhaustion
		if (CurrentStamina <= 0.f && !bIsExhausted)
		{
			bIsExhausted = true;

#if !UE_BUILD_SHIPPING
			GEngine->AddOnScreenDebugMessage(10, 2.f, FColor::Red, TEXT("EXHAUSTED!"));
#endif
		}
	}

	else
	{
		// Regenerate stamina
		CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + (StaminaRegenRate * DeltaTime));

		// Check for recovery from exhaustion
		if (bIsExhausted && CurrentStamina >= ExhaustionRecoveryThreshold)
		{
			bIsExhausted = false;

#if !UE_BUILD_SHIPPING
			GEngine->AddOnScreenDebugMessage(10, 2.f, FColor::Green, TEXT("Recovered!"));
#endif
		}
	}

	// Optional: Trigger events when stamina changes significantly
	if (FMath::Abs(OldStamina - CurrentStamina) > 0.01f)
	{
		OnRep_CurrentStamina(); // Call manually on server for local feedback
	}
}


bool AHeavyCharacter::CanSprint() const
{
	// Can't sprint if exhausted or don't have minimum stamina
	if (bIsExhausted || CurrentStamina < MinStaminaToSprint)
	{
		return false;
	}

	// Can't sprint if not grounded (optional - remove if you want air sprinting)
	if (GetCharacterMovement() && GetCharacterMovement()->MovementMode != MOVE_Custom)
	{
		return false;
	}

	return true;
}

void AHeavyCharacter::PlayFootstepSound(EFootType FootType)
{
	if (!FootstepAudioSystem)
	{
		return;
	}

	FFootstepContext Context;

	// Build context from current character state

	Context.SurfaceType = GetSurfaceTypeUnderFoot();
	Context.Velocity = GetVelocity().Size();
	Context.StaminaPercent = GetStaminaPercent();
	Context.bIsInCombat = bIsInCombatStance;
	Context.bIsSprinting = bWantsToSprint;
	Context.FootType = FootType;
	Context.Location = GetActorLocation();

	FootstepAudioSystem->PlayFootstep(Context);
}

ESurfaceType AHeavyCharacter::GetSurfaceTypeUnderFoot() const
{
	// Line trace down from character
	FVector Start = GetActorLocation();
	FVector End = Start - FVector(0, 0, 200.0f); // Trace 200 units down

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
	{
		if (HitResult.PhysMaterial.IsValid())
		{
			// You'll map physical materials to surface types
			// For now, return default
			return ESurfaceType::Default;
		}
	}

	return ESurfaceType::Default;
}


// ============================================================================
// CAMERA ZOOM
// ============================================================================

void AHeavyCharacter::HandleZoom(const FInputActionValue& Value)
{
	if (!CameraBoom)
	{
		return;
	}

	// Get scroll input (1.0 = scroll up, -1.0 = scroll down)
	float ScrollValue = Value.Get<float>();

	// Adjust target zoom
	// Positive scroll = zoom in (decrease distance)
	// Negative scroll = zoom out (increase distance)
	TargetArmLength -= ScrollValue * ZoomIncrement;

	// Clamp to min/max
	TargetArmLength = FMath::Clamp(TargetArmLength, MinZoomDistance, MaxZoomDistance);

#if !UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Log, TEXT("Zoom input: %.2f, Target: %.1f"), ScrollValue, TargetArmLength);
#endif
}

void AHeavyCharacter::UpdateCameraZoom(float DeltaTime)
{
	if (!CameraBoom)
	{
		return;
	}

	float CurrentArmLength = CameraBoom->TargetArmLength;

	if (bSmoothZoom)
	{
		// Smooth interpolation
		float NewArmLength = FMath::FInterpTo(CurrentArmLength, TargetArmLength, DeltaTime, ZoomSpeed);
		CameraBoom->TargetArmLength = NewArmLength;
	}
	else
	{
		//TODO: Call this during cutscenes or special events
		// Instant zoom
		CameraBoom->TargetArmLength = TargetArmLength;
	}
}

float AHeavyCharacter::GetZoomPercent() const
{
	if (!CameraBoom)
	{
		return 0.5f;
	}

	// Normalize current zoom to 0-1 range
	// 0 = fully zoomed in (MinZoomDistance)
	// 1 = fully zoomed out (MaxZoomDistance)
	float Range = MaxZoomDistance - MinZoomDistance;
	if (Range <= 0.f)
	{
		return 0.5f;
	}

	return (CameraBoom->TargetArmLength - MinZoomDistance) / Range;
}

void AHeavyCharacter::ResetZoom()
{
	TargetArmLength = DefaultZoomDistance;
}

// ============================================================================
// CAMERA COLLISION
// ============================================================================

void AHeavyCharacter::UpdateCameraCollision(float DeltaTime)
{
	if (!bEnableCameraCollision || !CameraBoom || !FollowCamera)
	{
		return;
	}

	// Get camera and character positions
	FVector CameraLocation = FollowCamera->GetComponentLocation();
	FVector CharacterLocation = GetActorLocation();
	FVector BoomOrigin = CameraBoom->GetComponentLocation();

	// Trace from boom origin to camera
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;

	bool bHit = GetWorld()->SweepSingleByChannel(
		Hit,
		BoomOrigin,
		CameraLocation,
		FQuat::Identity,
		CameraCollisionChannel,
		FCollisionShape::MakeSphere(CameraCollisionProbeSize),
		QueryParams
	);

	if (bHit && Hit.bBlockingHit)
	{
		// Calculate how much closer the camera needs to be
		float DistanceToHit = (Hit.Location - BoomOrigin).Size();
		float DesiredArmLength = DistanceToHit - CameraCollisionProbeSize;

		// Clamp to reasonable range
		DesiredArmLength = FMath::Clamp(DesiredArmLength, MinZoomDistance, CameraBoom->TargetArmLength);

		// Snap camera closer immediately to avoid clipping
		CameraBoom->TargetArmLength = DesiredArmLength;

#if ENABLE_DRAW_DEBUG && !UE_BUILD_SHIPPING
		DrawDebugSphere(GetWorld(), Hit.Location, CameraCollisionProbeSize, 8, FColor::Red, false, 0.f);
#endif
	}
	else
	{
		// No collision - smoothly return to target zoom
		float CurrentArmLength = CameraBoom->TargetArmLength;
		float NewArmLength = FMath::FInterpTo(CurrentArmLength, TargetArmLength, DeltaTime,
		                                      CameraCollisionRecoverySpeed);
		CameraBoom->TargetArmLength = NewArmLength;
	}
}

// ============================================================================
// OCCLUSION FADING
// ============================================================================

void AHeavyCharacter::UpdateOcclusionFading(float DeltaTime)
{
	if (!bEnableOcclusionFading || !FollowCamera)
	{
		return;
	}

	FVector CameraLocation = FollowCamera->GetComponentLocation();
	FVector CharacterLocation = GetActorLocation();

	// Trace from camera to character
	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;

	GetWorld()->LineTraceMultiByChannel(
		Hits,
		CameraLocation,
		CharacterLocation,
		ECC_Visibility,
		QueryParams
	);

	// Track which actors should be faded this frame
	TArray<AActor*> ActorsToFade;

	for (const FHitResult& Hit : Hits)
	{
		if (Hit.GetActor() && Hit.GetActor() != this)
		{
			AActor* HitActor = Hit.GetActor();

			// Only fade actors with static meshes
			if (HitActor->FindComponentByClass<UStaticMeshComponent>())
			{
				ActorsToFade.Add(HitActor);
			}
		}
	}

	// Fade new occluding actors
	for (AActor* Actor : ActorsToFade)
	{
		if (!OccludedActors.Contains(Actor))
		{
			OccludedActors.Add(Actor);
		}
		SetActorOpacity(Actor, OcclusionFadeOpacity);
	}

	// Restore actors no longer occluding
	for (int32 i = OccludedActors.Num() - 1; i >= 0; --i)
	{
		AActor* Actor = OccludedActors[i];
		if (!ActorsToFade.Contains(Actor))
		{
			RestoreActorOpacity(Actor);
			OccludedActors.RemoveAt(i);
		}
	}

#if ENABLE_DRAW_DEBUG && !UE_BUILD_SHIPPING
	DrawDebugLine(GetWorld(), CameraLocation, CharacterLocation, FColor::Yellow, false, 0.f);
#endif
}

void AHeavyCharacter::SetActorOpacity(AActor* Actor, float Opacity)
{
	if (!Actor)
	{
		return;
	}

	TArray<UStaticMeshComponent*> MeshComponents;
	Actor->GetComponents<UStaticMeshComponent>(MeshComponents);

	for (UStaticMeshComponent* MeshComp : MeshComponents)
	{
		if (!MeshComp)
		{
			continue;
		}

		// Get all materials on the mesh
		int32 NumMaterials = MeshComp->GetNumMaterials();
		for (int32 i = 0; i < NumMaterials; ++i)
		{
			UMaterialInterface* Material = MeshComp->GetMaterial(i);
			if (!Material)
			{
				continue;
			}

			// Create dynamic material instance if not already created
			UMaterialInstanceDynamic* DynMaterial = Cast<UMaterialInstanceDynamic>(Material);
			if (!DynMaterial)
			{
				DynMaterial = MeshComp->CreateDynamicMaterialInstance(i, Material);
			}

			if (DynMaterial)
			{
				// Set opacity parameter (requires material to have this parameter)
				DynMaterial->SetScalarParameterValue(OpacityParameterName, Opacity);

				// Ensure the mesh renders translucent
				MeshComp->SetCastShadow(false);
			}
		}
	}
}

void AHeavyCharacter::RestoreActorOpacity(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	TArray<UStaticMeshComponent*> MeshComponents;
	Actor->GetComponents<UStaticMeshComponent>(MeshComponents);

	for (UStaticMeshComponent* MeshComp : MeshComponents)
	{
		if (!MeshComp)
		{
			continue;
		}

		int32 NumMaterials = MeshComp->GetNumMaterials();
		for (int32 i = 0; i < NumMaterials; ++i)
		{
			UMaterialInterface* Material = MeshComp->GetMaterial(i);
			UMaterialInstanceDynamic* DynMaterial = Cast<UMaterialInstanceDynamic>(Material);

			if (DynMaterial)
			{
				// Restore to full opacity
				DynMaterial->SetScalarParameterValue(OpacityParameterName, 1.0f);
			}
		}

		// Restore shadow casting
		MeshComp->SetCastShadow(true);
	}
}

// ============================================================================
// CAMERA SHAKE ON LANDING
// ============================================================================

void AHeavyCharacter::TriggerLandingCameraShake(float ImpactVelocity)
{
	if (!LandingCameraShake)
	{
		return;
	}

	// Ignore small impacts
	if (FMath::Abs(ImpactVelocity) < MinShakeVelocity)
	{
		return;
	}

	// Calculate shake intensity based on fall velocity
	float VelocityRange = MaxShakeVelocity - MinShakeVelocity;
	float NormalizedVelocity = FMath::Clamp(
		(FMath::Abs(ImpactVelocity) - MinShakeVelocity) / VelocityRange,
		0.0f,
		1.0f
	);

	float ShakeScale = NormalizedVelocity * ShakeIntensityMultiplier;

	// Play camera shake
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->ClientStartCameraShake(LandingCameraShake, ShakeScale);

#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("Landing shake: Velocity=%.1f, Scale=%.2f"), ImpactVelocity, ShakeScale);
#endif
	}
}
