#include "RealSenseComponent.h"

URealSenseComponent::URealSenseComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void URealSenseComponent::BeginPlay()
{
    Super::BeginPlay();

    // Create the procedural mesh on our owner actor
    MeshComponent = NewObject<UProceduralMeshComponent>(GetOwner(), TEXT("DepthMesh"));
    MeshComponent->bUseAsyncCooking = true;
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
    MeshComponent->RegisterComponent();
    MeshComponent->AttachToComponent(
        GetOwner()->GetRootComponent(),
        FAttachmentTransformRules::KeepRelativeTransform);

    StartCamera();
}

void URealSenseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopCamera();
    Super::EndPlay(EndPlayReason);
}

bool URealSenseComponent::StartCamera()
{
    try
    {
        Config.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
        Pipeline.start(Config);
        bCameraRunning = true;
        UE_LOG(LogTemp, Log, TEXT("RealSense: Camera started successfully"));
        return true;
    }
    catch (const rs2::error& e)
    {
        UE_LOG(LogTemp, Error, TEXT("RealSense: Failed to start camera: %s"),
            *FString(e.what()));
        return false;
    }
}

void URealSenseComponent::StopCamera()
{
    if (bCameraRunning)
    {
        try
        {
            Pipeline.stop();
            bCameraRunning = false;
            UE_LOG(LogTemp, Log, TEXT("RealSense: Camera stopped"));
        }
        catch (const rs2::error& e)
        {
            UE_LOG(LogTemp, Error, TEXT("RealSense: Failed to stop camera: %s"),
                *FString(e.what()));
        }
    }
}

void URealSenseComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bCameraRunning) return;

    try
    {
        rs2::frameset Frames;
        if (Pipeline.poll_for_frames(&Frames))
        {
            ProcessDepthFrame(Frames);
        }
    }
    catch (const rs2::error& e)
    {
        UE_LOG(LogTemp, Warning, TEXT("RealSense: Frame error: %s"), *FString(e.what()));
    }

    // Rebuild the mesh on the configured interval
    TimeSinceLastMeshUpdate += DeltaTime;
    if (TimeSinceLastMeshUpdate >= MeshUpdateInterval && RawDepthData.Num() > 0)
    {
        RebuildMesh();
        TimeSinceLastMeshUpdate = 0.f;
    }
}

void URealSenseComponent::ProcessDepthFrame(const rs2::frameset& Frames)
{
    auto DepthFrame = Frames.get_depth_frame();
    if (!DepthFrame) return;

    const int CamW = DepthFrame.get_width();
    const int CamH = DepthFrame.get_height();

    const int ResX = FMath::Max(MeshResX, 2);
    const int ResY = FMath::Max(MeshResY, 2);

    RawDepthData.SetNumUninitialized(ResX * ResY);

    // Downsample: for each grid cell, sample the corresponding camera pixel
    for (int GY = 0; GY < ResY; ++GY)
    {
        for (int GX = 0; GX < ResX; ++GX)
        {
            const int PX = FMath::Clamp((int)((GX + 0.5f) / ResX * CamW), 0, CamW - 1);
            const int PY = FMath::Clamp((int)((GY + 0.5f) / ResY * CamH), 0, CamH - 1);

            float D = DepthFrame.get_distance(PX, PY);

            // Zero or out-of-range reads fall back to MaxDepth (flat background)
            if (D < MinDepth || D <= 0.f)
                D = MaxDepth;
            else if (D > MaxDepth)
                D = MaxDepth;

            RawDepthData[GY * ResX + GX] = D;
        }
    }
}

void URealSenseComponent::RebuildMesh()
{
    if (!MeshComponent) return;

    const int ResX = FMath::Max(MeshResX, 2);
    const int ResY = FMath::Max(MeshResY, 2);

    const int VertCount = ResX * ResY;
    TArray<FVector> Vertices;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<int32> Triangles;

    Vertices.SetNumUninitialized(VertCount);
    Normals.SetNumUninitialized(VertCount);
    UVs.SetNumUninitialized(VertCount);

    const float StepX = MeshWorldWidth  / (ResX - 1);
    const float StepY = MeshWorldHeight / (ResY - 1);

    // Build vertex positions
    // X/Y spread across the mesh plane, Z = depth displacement
    // Closer objects (lower depth) produce a higher Z, pushing toward the fish
    for (int GY = 0; GY < ResY; ++GY)
    {
        for (int GX = 0; GX < ResX; ++GX)
        {
            const float D = RawDepthData[GY * ResX + GX];
            const float Z = (MaxDepth - D) * DepthToUnrealScale;

            Vertices[GY * ResX + GX] = FVector(
                GX * StepX - MeshWorldWidth  * 0.5f,
                GY * StepY - MeshWorldHeight * 0.5f,
                Z
            );

            UVs[GY * ResX + GX] = FVector2D(
                (float)GX / (ResX - 1),
                (float)GY / (ResY - 1)
            );
        }
    }

    // Build triangles (two triangles per quad)
    Triangles.Reserve((ResX - 1) * (ResY - 1) * 6);
    for (int GY = 0; GY < ResY - 1; ++GY)
    {
        for (int GX = 0; GX < ResX - 1; ++GX)
        {
            const int I00 =  GY      * ResX + GX;
            const int I10 =  GY      * ResX + GX + 1;
            const int I01 = (GY + 1) * ResX + GX;
            const int I11 = (GY + 1) * ResX + GX + 1;

            Triangles.Add(I00); Triangles.Add(I01); Triangles.Add(I10);
            Triangles.Add(I10); Triangles.Add(I01); Triangles.Add(I11);
        }
    }

    // Compute per-vertex normals via central differences
    for (int GY = 0; GY < ResY; ++GY)
    {
        for (int GX = 0; GX < ResX; ++GX)
        {
            const FVector& VL = Vertices[GY * ResX + FMath::Max(GX - 1, 0)];
            const FVector& VR = Vertices[GY * ResX + FMath::Min(GX + 1, ResX - 1)];
            const FVector& VD = Vertices[FMath::Max(GY - 1, 0) * ResX + GX];
            const FVector& VU = Vertices[FMath::Min(GY + 1, ResY - 1) * ResX + GX];

            const FVector Tangent = (VR - VL).GetSafeNormal();
            const FVector Bitangent = (VU - VD).GetSafeNormal();
            Normals[GY * ResX + GX] = FVector::CrossProduct(Tangent, Bitangent).GetSafeNormal();
        }
    }

    if (!bMeshInitialized)
    {
        MeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals,
            UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), /*bCreateCollision=*/true);
        bMeshInitialized = true;
    }
    else
    {
        MeshComponent->UpdateMeshSection(0, Vertices, Normals,
            UVs, TArray<FColor>(), TArray<FProcMeshTangent>());
    }
}
