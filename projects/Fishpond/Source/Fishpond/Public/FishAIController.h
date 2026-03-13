// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Components/StateTreeAIComponent.h"
#include "FishAIController.generated.h"

/**
 * 
 */
UCLASS()
class FISHPOND_API AFishAIController : public AAIController
{
	GENERATED_BODY()
	
public:

	AFishAIController();

	//virtual void OnPossess(APawn* InPawn) override;

	//UPROPERTY(VisibleAnywhere, Category = "AI")
	//UStateTreeAIComponent* StateTreeAIComponent;

};
