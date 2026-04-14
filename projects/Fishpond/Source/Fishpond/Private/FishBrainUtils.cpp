// Fill out your copyright notice in the Description page of Project Settings.


#include "FishBrainUtils.h"

TArray<uint8> UFishBrainUtils::PackSenses(
    float Pressure, float ObsLeft, float ObsRight,
    float ObsFront, float ObsAbove, float ObsBelow)
{
    float Values[6] = { Pressure, ObsLeft, ObsRight, ObsFront, ObsAbove, ObsBelow };
    TArray<uint8> Bytes;
    Bytes.SetNum(24);
    FMemory::Memcpy(Bytes.GetData(), Values, 24);
    return Bytes;
}

void UFishBrainUtils::UnpackMovement(
    const TArray<uint8>& Bytes, float& Thrust, float& Yaw, float& Pitch)
{
    if (Bytes.Num() < 12) return;
    FMemory::Memcpy(&Thrust, Bytes.GetData() + 0, 4);
    FMemory::Memcpy(&Yaw, Bytes.GetData() + 4, 4);
    FMemory::Memcpy(&Pitch, Bytes.GetData() + 8, 4);
}