// Fill out your copyright notice in the Description page of Project Settings.


#include "FishMoveStateTreeTask.h"
#include "FishCharacter.h"
#include "FlockManager.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "SwimVolume.h"
#include "Kismet/GameplayStatics.h"

EStateTreeRunStatus UFishMoveStateTreeTask::EnterState(FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition)
{
    ElapsedTime = 0.f;

    // Get the fish and pick a starting target
    AAIController* AIC = Cast<AAIController>(Context.GetOwner());
    if (!AIC) return EStateTreeRunStatus::Failed;

    AFishCharacter* Fish = Cast<AFishCharacter>(AIC->GetPawn());
    if (!Fish) return EStateTreeRunStatus::Failed;

    ASwimVolume* SwimVolume = Cast<ASwimVolume>(UGameplayStatics::GetActorOfClass(AIC->GetWorld(), ASwimVolume::StaticClass()));

    if (!SwimVolume) return EStateTreeRunStatus::Failed;

    FVector Center = SwimVolume->SwimBox->GetComponentLocation();
    FVector Extent = SwimVolume->SwimBox->GetScaledBoxExtent();

    TargetLocation = FMath::RandPointInBox(FBox(Center - Extent, Center + Extent));

    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus UFishMoveStateTreeTask::Tick(FStateTreeExecutionContext& Context,
    float DeltaTime)
{
    AAIController* AIC = Cast<AAIController>(Context.GetOwner());
    if (!AIC) return EStateTreeRunStatus::Failed;

    AFishCharacter* Fish = Cast<AFishCharacter>(AIC->GetPawn());
    if (!Fish) return EStateTreeRunStatus::Failed;

    // Timeout check
    ElapsedTime += DeltaTime;
    if (ElapsedTime >= TimeoutDuration)
        return EStateTreeRunStatus::Failed;

    // Arrival check
    if (FVector::Dist(Fish->GetActorLocation(), TargetLocation) < AcceptanceRadius)
        return EStateTreeRunStatus::Succeeded;

    // Wander and flock forces
    FVector WanderForce = (TargetLocation - Fish->GetActorLocation()).GetSafeNormal();
    FVector FlockForce = FVector::ZeroVector;

    if (Fish->FlockManager)
        FlockForce = Fish->FlockManager->GetFlockForce(Fish).GetSafeNormal();

    FVector FinalDirection = ((WanderForce * WanderWeight) +
        (FlockForce * FlockWeight)).GetSafeNormal();

    Fish->AddMovementInput(FinalDirection, 1.0f);

    // Smooth rotation
    if (!FinalDirection.IsNearlyZero())
    {
        FRotator TargetRot = FinalDirection.Rotation();
        FRotator NewRot = FMath::RInterpTo(
            Fish->GetActorRotation(), TargetRot, DeltaTime, 2.0f);
        Fish->SetActorRotation(NewRot);
    }

    return EStateTreeRunStatus::Running;
}

void UFishMoveStateTreeTask::ExitState(FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition)
{
    // Clean up if needed
}