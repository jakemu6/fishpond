// Fill out your copyright notice in the Description page of Project Settings.


#include "FlockManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AFlockManager::AFlockManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}


// Called when the game starts or when spawned
void AFlockManager::BeginPlay()
{
	Super::BeginPlay();
	
}

void AFlockManager::RegisterFish(ACharacter* Fish)
{
	if (Fish)
		FlockMembers.AddUnique(Fish);
}

void AFlockManager::UnregisterFish(ACharacter* Fish)
{
	FlockMembers.Remove(Fish);
}

FVector AFlockManager::GetFlockForce(ACharacter* RequestingFish)
{
    FVector SeparationForce = FVector::ZeroVector;
    FVector AlignmentForce = FVector::ZeroVector;
    FVector CohesionForce = FVector::ZeroVector;

    int32 AlignmentCount = 0;
    int32 CohesionCount = 0;

    FVector MyLocation = RequestingFish->GetActorLocation();

    for (ACharacter* Member : FlockMembers)
    {
        if (!Member || Member == RequestingFish)
            continue;

        FVector ToMember = Member->GetActorLocation() - MyLocation;
        float Distance = ToMember.Size();

        // Separation — push away from nearby fish
        if (Distance < SeparationRadius && Distance > 0.f)
        {
            SeparationForce += (-ToMember.GetSafeNormal() / Distance);
        }

        // Alignment — match velocity of nearby fish
        if (Distance < AlignmentRadius)
        {
            AlignmentForce += Member->GetCharacterMovement()->Velocity;
            AlignmentCount++;
        }

        // Cohesion — move toward center of nearby fish
        if (Distance < CohesionRadius)
        {
            CohesionForce += Member->GetActorLocation();
            CohesionCount++;
        }
    }

    // Average and normalize alignment
    if (AlignmentCount > 0)
        AlignmentForce = (AlignmentForce / AlignmentCount).GetSafeNormal();

    // Steer toward average cohesion position
    if (CohesionCount > 0)
    {
        FVector AveragePosition = CohesionForce / CohesionCount;
        CohesionForce = (AveragePosition - MyLocation).GetSafeNormal();
    }

    return (SeparationForce * SeparationWeight)
        + (AlignmentForce * AlignmentWeight)
        + (CohesionForce * CohesionWeight);
}

// Called every frame
void AFlockManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

