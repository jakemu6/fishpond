// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FishBrainUtils.generated.h"

/**
 * 
 */
UCLASS()
class FISHPOND_API UFishBrainUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    // Pack 6 sense floats into a 24-byte array ready to send via UDP
    UFUNCTION(BlueprintCallable, Category = "Fish Brain")
    static TArray<uint8> PackSenses(
        float Pressure,
        float ObsLeft,
        float ObsRight,
        float ObsFront,
        float ObsAbove,
        float ObsBelow
    );

    // Unpack the 12-byte UDP response into thrust, yaw, pitch
    UFUNCTION(BlueprintCallable, Category = "Fish Brain")
    static void UnpackMovement(
        const TArray<uint8>& Bytes,
        float& Thrust,
        float& Yaw,
        float& Pitch
    );

};
