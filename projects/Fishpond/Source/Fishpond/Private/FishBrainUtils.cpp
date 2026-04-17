// Fill out your copyright notice in the Description page of Project Settings.


#include "FishBrainUtils.h"

TArray<uint8> UFishBrainUtils::PackSenses(
    int32 FishId,
    float Pressure, float ObsLeft, float ObsRight,
    float ObsFront, float ObsAbove, float ObsBelow)
{
    TArray<uint8> Bytes;
    Bytes.SetNum(28);
    uint32 Id = (uint32)FishId;
    FMemory::Memcpy(Bytes.GetData() + 0, &Id, 4);
    float Values[6] = { Pressure, ObsLeft, ObsRight, ObsFront, ObsAbove, ObsBelow };
    FMemory::Memcpy(Bytes.GetData() + 4, Values, 24);
    return Bytes;
}

void UFishBrainUtils::UnpackMovement(
    const TArray<uint8>& Bytes, int32& FishId, float& Thrust, float& Yaw, float& Pitch)
{
    if (Bytes.Num() < 16) return;
    uint32 Id;
    FMemory::Memcpy(&Id,     Bytes.GetData() + 0,  4);
    FMemory::Memcpy(&Thrust, Bytes.GetData() + 4,  4);
    FMemory::Memcpy(&Yaw,    Bytes.GetData() + 8,  4);
    FMemory::Memcpy(&Pitch,  Bytes.GetData() + 12, 4);
    FishId = (int32)Id;
}