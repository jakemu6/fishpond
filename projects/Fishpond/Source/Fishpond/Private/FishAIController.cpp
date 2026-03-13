// Fill out your copyright notice in the Description page of Project Settings.


#include "FishAIController.h"
#include "FishCharacter.h"

AFishAIController::AFishAIController()
{
    PrimaryActorTick.bCanEverTick = true;

    //StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAIComponent"));
}

//void AFishAIController::OnPossess(APawn* InPawn)
//{
//    Super::OnPossess(InPawn);
//
//    AFishCharacter* Fish = Cast<AFishCharacter>(InPawn);
//    if (!Fish)
//    {
//        UE_LOG(LogTemp, Warning, TEXT("FishAIController: Possessed pawn is not a FishCharacter"));
//        return;
//    }
//
//    if (StateTreeAIComponent)
//    {
//        UE_LOG(LogTemp, Log, TEXT("FishAIController: Starting State Tree"));
//        StateTreeAIComponent->StartLogic();
//    }
//    else
//    {
//        UE_LOG(LogTemp, Warning, TEXT("FishAIController: No StateTreeComponent found"));
//    }
//}