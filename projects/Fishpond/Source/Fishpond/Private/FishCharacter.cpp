// Fill out your copyright notice in the Description page of Project Settings.


#include "FishCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AFishCharacter::AFishCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AFishCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Find and register with the flock manager
	AActor* Found = UGameplayStatics::GetActorOfClass(GetWorld(), AFlockManager::StaticClass());
	FlockManager = Cast<AFlockManager>(Found);

	if (FlockManager)
		FlockManager->RegisterFish(this);
}

void AFishCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (FlockManager)
		FlockManager->UnregisterFish(this);

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AFishCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AFishCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

