// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FishBrainUtils.generated.h"

UCLASS()
class FISHPOND_API UFishBrainUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Pack fish ID + 6 obstacle senses + 3 school senses into a 40-byte UDP packet.
    //
    // School inputs (send 0, 0, 0.5 when schooling is not active):
    //   SchoolYaw:   -1=group hard left,  0=group straight ahead, +1=group hard right
    //   SchoolPitch: -1=group below,      0=same depth,           +1=group above
    //   SchoolCrowd:  0=isolated/alone,   0.4=comfortable,         1=too crowded
    UFUNCTION(BlueprintCallable, Category = "Fish Brain")
    static TArray<uint8> PackSenses(
        int32 FishId,
        float Pressure,
        float ObsLeft,
        float ObsRight,
        float ObsFront,
        float ObsAbove,
        float ObsBelow,
        float SchoolYaw,
        float SchoolPitch,
        float SchoolCrowd
    );

    // Unpack the 16-byte UDP response into fish ID, thrust, yaw, pitch.
    // Response format is unchanged.
    UFUNCTION(BlueprintCallable, Category = "Fish Brain")
    static void UnpackMovement(
        const TArray<uint8>& Bytes,
        int32& FishId,
        float& Thrust,
        float& Yaw,
        float& Pitch
    );
};
