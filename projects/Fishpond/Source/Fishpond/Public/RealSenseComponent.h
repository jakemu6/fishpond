#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "librealsense2/rs.hpp"
#include "RealSenseComponent.generated.h"

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

    // Is the camera currently running
    UFUNCTION(BlueprintCallable, Category = "RealSense")
    bool IsCameraRunning() const { return bCameraRunning; }

    // Raw depth values in metres, row-major, MeshResX * MeshResY elements
    UFUNCTION(BlueprintCallable, Category = "RealSense")
    TArray<float> GetRawDepthData() const { return RawDepthData; }

    // --- Mesh Settings ---

    // Grid resolution of the generated mesh (downsampled from 640x480 camera feed)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealSense|Mesh")
    int32 MeshResX = 64;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealSense|Mesh")
    int32 MeshResY = 48;

    // World-space extent of the mesh plane in Unreal units
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealSense|Mesh")
    float MeshWorldWidth = 500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealSense|Mesh")
    float MeshWorldHeight = 375.f;

    // How often to rebuild the mesh in seconds (0 = every frame)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealSense|Mesh")
    float MeshUpdateInterval = 0.05f;

    // --- Depth Settings ---

    // Minimum depth in metres — values below this clamp to MinDepth displacement
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealSense")
    float MinDepth = 0.3f;

    // Maximum depth in metres — mesh surface sits at Z=0 at this depth
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealSense")
    float MaxDepth = 3.0f;

    // One metre of depth maps to this many Unreal units of Z displacement
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealSense")
    float DepthToUnrealScale = 100.f;

private:
    rs2::pipeline Pipeline;
    rs2::config Config;

    bool bCameraRunning = false;
    bool bMeshInitialized = false;
    float TimeSinceLastMeshUpdate = 0.f;

    // Raw depth grid, row-major (MeshResX * MeshResY)
    TArray<float> RawDepthData;

    UPROPERTY()
    UProceduralMeshComponent* MeshComponent = nullptr;

    void ProcessDepthFrame(const rs2::frameset& Frames);
    void RebuildMesh();
};
