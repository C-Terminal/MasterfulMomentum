// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HeavyCharacterMovementComponent.h"
#include "HeavyCharacter.generated.h"

UCLASS()
class MASTERFULMOMENTUM_API AHeavyCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	// This runs when the pawn is possessed and ready for input
	virtual void PawnClientRestart() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* HeavyMovementContext;
	// Sets default values for this character's properties
	// AHeavyCharacter();
	AHeavyCharacter(const FObjectInitializer& ObjectInitializer);

	// Helper getter
	UFUNCTION(BlueprintPure, Category = "Heavy Movement")
	UHeavyCharacterMovementComponent* GetHeavyMovement() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* FollowCamera;
	// A pointer to your IMC asset so you can assign it in the Editor
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void ForceMovement();

	// Bind this to IA_HeavyMove in the BP details
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	void HandleMove(const struct FInputActionValue& Value);

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PossessedBy(AController* NewController) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input

};
