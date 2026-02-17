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
	// Sets default values for this character's properties
	// AHeavyCharacter();
	AHeavyCharacter(const FObjectInitializer& ObjectInitializer);

	// Helper getter
	UFUNCTION(BlueprintPure, Category = "Heavy Movement")
	UHeavyCharacterMovementComponent* GetHeavyMovement() const;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
