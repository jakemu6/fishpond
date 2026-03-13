// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "SwimVolume.generated.h"

UCLASS()
class FISHPOND_API ASwimVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASwimVolume();

	UPROPERTY(VisibleAnywhere, Category = "Swim")
	UBoxComponent* SwimBox;
};
