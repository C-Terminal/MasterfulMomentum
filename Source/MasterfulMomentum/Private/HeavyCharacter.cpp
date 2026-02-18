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

	// ForceMovement();
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
	// If you don't see this Cyan message on your screen, 
	// the Enhanced Input system is NOT calling this function.
	GEngine->AddOnScreenDebugMessage(1, 1.f, FColor::Cyan, TEXT("!!! HANDLE MOVE EXECUTING !!!"));

	// ... rest of your code ...
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Get facing direction based on camera/controller
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// MovementVector.Y is Forward/Back due to your YXZ Swizzle
		AddMovementInput(ForwardDirection, MovementVector.Y);
		// MovementVector.X is Left/Right
		AddMovementInput(RightDirection, MovementVector.X);

		// Debug Log to confirm signal arrival
		UE_LOG(LogTemp, Log, TEXT("Input Received: X=%f, Y=%f"), MovementVector.X, MovementVector.Y);


		// NEW: Real-time debug message on your Windows 10 screen
		GEngine->AddOnScreenDebugMessage(1, 1.f, FColor::Cyan,
		                                 FString::Printf(TEXT("Current Velocity: %s"), *GetVelocity().ToString()));
	}
}
