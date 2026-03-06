// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlockManager.generated.h"

UCLASS()
class FISHPOND_API AFlockManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFlockManager();

	void RegisterFish(ACharacter* Fish);
	void UnregisterFish(ACharacter* Fish);

    FVector GetFlockForce(ACharacter* RequestingFish);

    UPROPERTY(EditAnywhere, Category = "Flocking")
    float SeparationRadius = 150.f;

    UPROPERTY(EditAnywhere, Category = "Flocking")
    float AlignmentRadius = 300.f;

    UPROPERTY(EditAnywhere, Category = "Flocking")
    float CohesionRadius = 300.f;

    UPROPERTY(EditAnywhere, Category = "Flocking")
    float SeparationWeight = 1.5f;

    UPROPERTY(EditAnywhere, Category = "Flocking")
    float AlignmentWeight = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Flocking")
    float CohesionWeight = 1.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
    UPROPERTY()
    TArray<ACharacter*> FlockMembers;

};
