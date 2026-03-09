// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_FishMove.h"
#include "AIController.h"
#include "FishCharacter.h"
#include "FlockManager.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTTask_FishMove::UBTTask_FishMove()
{
    // Enable tick on this task
    bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_FishMove::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // Return InProgress so TickTask gets called every frame
    ElapsedTime = 0.0f;
    return EBTNodeResult::InProgress;
}

void UBTTask_FishMove::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // Increment timer
    ElapsedTime += DeltaSeconds;
    if (ElapsedTime >= TimeoutDuration)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

    AFishCharacter* Fish = Cast<AFishCharacter>(AIC->GetPawn());
    if (!Fish) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

    // Get target from blackboard
    FVector Target = OwnerComp.GetBlackboardComponent()->GetValueAsVector(TargetLocationKey.SelectedKeyName);
    FVector MyLocation = Fish->GetActorLocation();

    // Check arrival
    if (FVector::Dist(MyLocation, Target) < AcceptanceRadius)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    // Wander force toward target
    FVector WanderForce = (Target - MyLocation).GetSafeNormal();

    // Flock force from manager
    FVector FlockForce = FVector::ZeroVector;
    if (Fish->FlockManager)
        FlockForce = Fish->FlockManager->GetFlockForce(Fish).GetSafeNormal();

    // Blend and apply
    FVector FinalDirection = ((WanderForce * WanderWeight) + (FlockForce * FlockWeight)).GetSafeNormal();
    Fish->AddMovementInput(FinalDirection, 1.0f);

    // Smoothly rotate to face movement direction
    if (!FinalDirection.IsNearlyZero())
    {
        FRotator TargetRot = FinalDirection.Rotation();
        FRotator NewRot = FMath::RInterpTo(Fish->GetActorRotation(), TargetRot, DeltaSeconds, 1.0f);
        Fish->SetActorRotation(NewRot);
    }
}