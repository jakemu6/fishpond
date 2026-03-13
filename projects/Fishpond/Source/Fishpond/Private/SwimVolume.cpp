// Fill out your copyright notice in the Description page of Project Settings.


#include "SwimVolume.h"

ASwimVolume::ASwimVolume()
{
    SwimBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SwimBox"));
    RootComponent = SwimBox;
}