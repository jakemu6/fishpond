// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FishMove.generated.h"

/**
 * 
 */
UCLASS()
class FISHPOND_API UBTTask_FishMove : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
    UBTTask_FishMove();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, Category = "Fish")
    float AcceptanceRadius = 100.f;

    UPROPERTY(EditAnywhere, Category = "Fish")
    float WanderWeight = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Fish")
    float FlockWeight = 1.0f;

    // Key in your blackboard that stores the target location
    UPROPERTY(EditAnywhere, Category = "Fish")
    FBlackboardKeySelector TargetLocationKey;

    UPROPERTY(EditAnywhere, Category = "Fish")
    float TimeoutDuration = 5.0f;  // adjust as needed

private:
    float ElapsedTime = 0.0f;
};
