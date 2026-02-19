// Fill out your copyright notice in the Description page of Project Settings.

#include "MasterfulMomentum/Public/HeavyCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"


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
	CameraBoom->TargetArmLength = 400.0f; // Distance behind character
	CameraBoom->bUsePawnControlRotation = true; // Rotate arm based on controller

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
}

UHeavyCharacterMovementComponent* AHeavyCharacter::GetHeavyMovement() const
{
	return Cast<UHeavyCharacterMovementComponent>(GetCharacterMovement());
}

// Called when the game starts or when spawned
void AHeavyCharacter::BeginPlay()
{
	Super::BeginPlay();


	// Verify assets are assigned
	UE_LOG(LogTemp, Warning, TEXT("MoveAction valid: %s"), MoveAction ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Warning, TEXT("HeavyMovementContext valid: %s"), HeavyMovementContext ? TEXT("YES") : TEXT("NO"));


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
	}
}

void AHeavyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	UE_LOG(LogTemp, Warning, TEXT("Character Possessed by: %s"), *NewController->GetName());
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

		// Instead of AddMovementInput, set it directly on your custom component
		if (UHeavyCharacterMovementComponent* HeavyMovement = GetHeavyMovement())
		{
			FVector InputDir = (ForwardDirection * MovementVector.Y) + (RightDirection * MovementVector.X);
			HeavyMovement->CustomInputVector = InputDir;

			GEngine->AddOnScreenDebugMessage(20, 0.f, FColor::Magenta,
			                                 FString::Printf(
				                                 TEXT("CustomInputVector set to: %s"), *InputDir.ToString()));
		}
	}
}
