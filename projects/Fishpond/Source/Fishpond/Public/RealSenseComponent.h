#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "librealsense2/rs.hpp"
#include "RealSenseComponent.generated.h"

// Struct to expose tracked player data to Blueprint
USTRUCT(BlueprintType)
struct FTrackedPlayer
{
    GENERATED_BODY()

    // World position of the tracked player
    UPROPERTY(BlueprintReadOnly, Category = "RealSense")
    FVector Position = FVector::ZeroVector;

    // Depth distance in meters
    UPROPERTY(BlueprintReadOnly, Category = "RealSense")
    float Depth = 0.f;

    // Whether this player is currently being tracked
    UPROPERTY(BlueprintReadOnly, Category = "RealSense")
    bool bIsValid = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDetected, FTrackedPlayer, PlayerData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerLost);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FISHPOND_API URealSenseComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URealSenseComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // Initialize and start the RealSense camera
    UFUNCTION(BlueprintCallable, Category = "RealSense")
    bool StartCamera();

    // Stop the RealSense camera
    UFUNCTION(BlueprintCallable, Category = "RealSense")
    void StopCamera();

    // Get the current tracked player data
    UFUNCTION(BlueprintCallable, Category = "RealSense")
    FTrackedPlayer GetTrackedPlayer() const { return TrackedPlayer; }

    // Is the camera currently running
    UFUNCTION(BlueprintCallable, Category = "RealSense")
    bool IsCameraRunning() const { return bCameraRunning; }

    // Minimum depth in meters to detect a player
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealSense")
    float MinDepth = 0.3f;

    // Maximum depth in meters to detect a player
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealSense")
    float MaxDepth = 3.0f;

    // Scale factor to convert RealSense depth units to Unreal units
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealSense")
    float DepthToUnrealScale = 100.f;

    // Fired when a player is detected
    UPROPERTY(BlueprintAssignable, Category = "RealSense")
    FOnPlayerDetected OnPlayerDetected;

    // Fired when the player is no longer detected
    UPROPERTY(BlueprintAssignable, Category = "RealSense")
    FOnPlayerLost OnPlayerLost;

private:
    // RealSense pipeline and config
    rs2::pipeline Pipeline;
    rs2::config Config;
	TOptional<rs2::align> Align;

    // Current tracked player
    FTrackedPlayer TrackedPlayer;

    bool bCameraRunning = false;
    bool bWasTrackingLastFrame = false;

    // Process the latest depth frame
    void ProcessDepthFrame(const rs2::frameset& Frames);

    // Convert RealSense 3D point to Unreal world space
    FVector RealSenseToUnreal(float X, float Y, float Z) const;
};