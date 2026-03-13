// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/StateTreeTaskBlueprintBase.h"
#include "FishMoveStateTreeTask.generated.h"

/**
 * 
 */
UCLASS()
class FISHPOND_API UFishMoveStateTreeTask : public UStateTreeTaskBlueprintBase
{
	GENERATED_BODY()
	
public:

    UPROPERTY(EditAnywhere, Category = "Fish")
    float AcceptanceRadius = 100.f;

    UPROPERTY(EditAnywhere, Category = "Fish")
    float TimeoutDuration = 5.0f;

    UPROPERTY(EditAnywhere, Category = "Fish")
    float WanderWeight = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Fish")
    float FlockWeight = 1.0f;

protected:

    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
        const FStateTreeTransitionResult& Transition) override;

    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context,
        float DeltaTime) override;

    virtual void ExitState(FStateTreeExecutionContext& Context,
        const FStateTreeTransitionResult& Transition) override;

private:
    FVector TargetLocation;
    float ElapsedTime = 0.f;
};
