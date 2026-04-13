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
            
            // Save an index for each pixel point in the array
            RawDepthData[GY * ResX + GX] = D;
        }
    }

    SmoothDepthData();
}

TArray<FVector> URealSenseComponent::GetInteriorPoints(int32 Count) const
{
    TArray<FVector> Points;
    if (RawDepthData.Num() == 0 || Count <= 0) return Points;

    const int ResX     = FMath::Max(MeshResX, 2);
    const int ResY     = FMath::Max(MeshResY, 2);
    const float StepX  = MeshWorldWidth  / (ResX - 1);
    const float StepY  = MeshWorldHeight / (ResY - 1);
    const float BgCutoff = MaxDepth - FMath::Max(BackgroundDepthThreshold, 0.001f);

    // Collect all fully-foreground quad indices (top-left corner stored)
    TArray<int32> FGCells;
    FGCells.Reserve((ResX - 1) * (ResY - 1));
    for (int GY = 0; GY < ResY - 1; ++GY)
        for (int GX = 0; GX < ResX - 1; ++GX)
            if (RawDepthData[ GY      * ResX + GX    ] < BgCutoff &&
                RawDepthData[ GY      * ResX + GX + 1] < BgCutoff &&
                RawDepthData[(GY + 1) * ResX + GX    ] < BgCutoff &&
                RawDepthData[(GY + 1) * ResX + GX + 1] < BgCutoff)
                FGCells.Add(GY * ResX + GX);

    if (FGCells.Num() == 0) return Points;

    Points.Reserve(Count);
    for (int i = 0; i < Count; ++i)
    {
        // Pick a random foreground cell
        const int32 Cell = FGCells[FMath::RandRange(0, FGCells.Num() - 1)];
        const int GX = Cell % ResX;
        const int GY = Cell / ResX;

        // Random sub-cell position
        const float FracX = FMath::FRand();
        const float FracY = FMath::FRand();

        const float PX = (GX + FracX) * StepX - MeshWorldWidth  * 0.5f;
        const float PY = (GY + FracY) * StepY - MeshWorldHeight * 0.5f;

        // Bilinear-interpolate the surface depth at this XY to get the ceiling Z
        const float D00 = RawDepthData[ GY      * ResX + GX    ];
        const float D10 = RawDepthData[ GY      * ResX + GX + 1];
        const float D01 = RawDepthData[(GY + 1) * ResX + GX    ];
        const float D11 = RawDepthData[(GY + 1) * ResX + GX + 1];
        const float D   = FMath::BiLerp(D00, D10, D01, D11, FracX, FracY);

        // Interior Z is anywhere between the back cap (0) and the displaced surface
        const float MaxZ = (MaxDepth - D) * DepthToUnrealScale;
        const float Z    = FMath::FRandRange(0.f, MaxZ);

        Points.Add(FVector(PX, PY, Z));
    }

    return Points;
}

// Shared grid lookup: converts local XY to grid coords and bilinearly samples depth.
// Returns false if out of bounds or background.
static bool SampleDepthGrid(
    float LocalX, float LocalY,
    const TArray<float>& DepthData,
    int ResX, int ResY,
    float WorldWidth, float WorldHeight,
    float BgCutoff,
    float& OutDepth, float& OutFracX, float& OutFracY,
    int& OutGX, int& OutGY)
{
    const float StepX = WorldWidth  / (ResX - 1);
    const float StepY = WorldHeight / (ResY - 1);

    const float GridX = (LocalX + WorldWidth  * 0.5f) / StepX;
    const float GridY = (LocalY + WorldHeight * 0.5f) / StepY;

    OutGX = FMath::FloorToInt(GridX);
    OutGY = FMath::FloorToInt(GridY);

    if (OutGX < 0 || OutGX >= ResX - 1 || OutGY < 0 || OutGY >= ResY - 1)
        return false;

    OutFracX = GridX - OutGX;
    OutFracY = GridY - OutGY;

    const float D00 = DepthData[ OutGY      * ResX + OutGX    ];
    const float D10 = DepthData[ OutGY      * ResX + OutGX + 1];
    const float D01 = DepthData[(OutGY + 1) * ResX + OutGX    ];
    const float D11 = DepthData[(OutGY + 1) * ResX + OutGX + 1];

    if (D00 >= BgCutoff || D10 >= BgCutoff || D01 >= BgCutoff || D11 >= BgCutoff)
        return false;

    OutDepth = FMath::BiLerp(D00, D10, D01, D11, OutFracX, OutFracY);
    return true;
}

float URealSenseComponent::SampleSurfaceZ(float LocalX, float LocalY) const
{
    if (RawDepthData.Num() == 0) return -1.f;

    const int ResX = FMath::Max(MeshResX, 2);
    const int ResY = FMath::Max(MeshResY, 2);
    const float BgCutoff = MaxDepth - FMath::Max(BackgroundDepthThreshold, 0.001f);

    float Depth, FX, FY; int GX, GY;
    if (!SampleDepthGrid(LocalX, LocalY, RawDepthData, ResX, ResY,
                         MeshWorldWidth, MeshWorldHeight, BgCutoff,
                         Depth, FX, FY, GX, GY))
        return -1.f;

    return (MaxDepth - Depth) * DepthToUnrealScale;
}

bool URealSenseComponent::IsInsideSilhouette(FVector LocalPoint) const
{
    const float SurfZ = SampleSurfaceZ(LocalPoint.X, LocalPoint.Y);
    return SurfZ >= 0.f && LocalPoint.Z >= 0.f && LocalPoint.Z <= SurfZ;
}

void URealSenseComponent::SmoothDepthData()
{
    const int ResX = FMath::Max(MeshResX, 2);
    const int ResY = FMath::Max(MeshResY, 2);
    const int R    = SmoothingRadius;

    if (R <= 0) return;

    TArray<float> Smoothed;
    Smoothed.SetNumUninitialized(ResX * ResY);

    const float TwoSigmaSq = 2.f * (float)(R * R);

    for (int GY = 0; GY < ResY; ++GY)
    {
        for (int GX = 0; GX < ResX; ++GX)
        {
            float Sum = 0.f, WeightSum = 0.f;

            for (int KY = -R; KY <= R; ++KY)
            {
                for (int KX = -R; KX <= R; ++KX)
                {
                    const int NX = FMath::Clamp(GX + KX, 0, ResX - 1);
                    const int NY = FMath::Clamp(GY + KY, 0, ResY - 1);
                    const float W = FMath::Exp(-(KX*KX + KY*KY) / TwoSigmaSq);
                    Sum       += RawDepthData[NY * ResX + NX] * W;
                    WeightSum += W;
                }
            }

            Smoothed[GY * ResX + GX] = Sum / WeightSum;
        }
    }

    RawDepthData = MoveTemp(Smoothed);
}

void URealSenseComponent::RebuildMesh()
{
    if (!MeshComponent) return;

    const int ResX = FMath::Max(MeshResX, 2);
    const int ResY = FMath::Max(MeshResY, 2);
    const int N    = ResX * ResY;

    const float StepX    = MeshWorldWidth  / (ResX - 1);
    const float StepY    = MeshWorldHeight / (ResY - 1);
    // Depth values at or above this cutoff are treated as background and excluded
    const float BgCutoff = MaxDepth - FMath::Max(BackgroundDepthThreshold, 0.001f);

    // Per-vertex foreground mask
    TArray<bool> bFG;
    bFG.SetNumUninitialized(N);
    for (int i = 0; i < N; ++i)
        bFG[i] = (RawDepthData[i] < BgCutoff);

    // True only when all four corners of a quad are foreground
    auto QuadFG = [&](int GX, int GY) -> bool
    {
        return bFG[ GY      * ResX + GX    ] &&
               bFG[ GY      * ResX + GX + 1] &&
               bFG[(GY + 1) * ResX + GX    ] &&
               bFG[(GY + 1) * ResX + GX + 1];
    };

    // Two vertex layers stored flat:
    //   [0 .. N-1]   — top surface (depth-displaced)
    //   [N .. 2N-1]  — bottom cap  (Z = 0, same XY)
    TArray<FVector>   Vertices;
    TArray<FVector2D> UVs;
    Vertices.SetNumUninitialized(2 * N);
    UVs.SetNumUninitialized(2 * N);

    for (int GY = 0; GY < ResY; ++GY)
    {
        for (int GX = 0; GX < ResX; ++GX)
        {
            const int   Idx = GY * ResX + GX;
            const float D   = RawDepthData[Idx];
            const float Z   = (MaxDepth - D) * DepthToUnrealScale;
            const float PX  = GX * StepX - MeshWorldWidth  * 0.5f;
            const float PY  = GY * StepY - MeshWorldHeight * 0.5f;
            const FVector2D UV((float)GX / (ResX - 1), (float)GY / (ResY - 1));

            Vertices[Idx]     = FVector(PX, PY, Z);   // top surface
            Vertices[N + Idx] = FVector(PX, PY, 0.f); // flat back cap
            UVs[Idx]          = UV;
            UVs[N + Idx]      = UV;
        }
    }

    TArray<int32> Triangles;
    Triangles.Reserve(N * 8);

    for (int GY = 0; GY < ResY - 1; ++GY)
    {
        for (int GX = 0; GX < ResX - 1; ++GX)
        {
            if (!QuadFG(GX, GY)) continue;

            const int I00 =  GY      * ResX + GX;
            const int I10 =  GY      * ResX + GX + 1;
            const int I01 = (GY + 1) * ResX + GX;
            const int I11 = (GY + 1) * ResX + GX + 1;

            // Top face (outward normal faces camera)
            Triangles.Add(I00); Triangles.Add(I01); Triangles.Add(I10);
            Triangles.Add(I10); Triangles.Add(I01); Triangles.Add(I11);

            // Bottom cap (reversed winding so normal faces away from camera)
            Triangles.Add(N+I00); Triangles.Add(N+I10); Triangles.Add(N+I01);
            Triangles.Add(N+I10); Triangles.Add(N+I11); Triangles.Add(N+I01);

            // Side walls — added only where this quad borders a background quad or the mesh edge.
            // This seals the solid and keeps the background out of the mesh.

            // Left wall
            if (GX == 0 || !QuadFG(GX - 1, GY))
            {
                Triangles.Add(I00); Triangles.Add(N+I00); Triangles.Add(N+I01);
                Triangles.Add(I00); Triangles.Add(N+I01); Triangles.Add(I01);
            }
            // Right wall
            if (GX == ResX - 2 || !QuadFG(GX + 1, GY))
            {
                Triangles.Add(I10); Triangles.Add(I11); Triangles.Add(N+I11);
                Triangles.Add(I10); Triangles.Add(N+I11); Triangles.Add(N+I10);
            }
            // Top wall (low-GY edge)
            if (GY == 0 || !QuadFG(GX, GY - 1))
            {
                Triangles.Add(I00); Triangles.Add(I10); Triangles.Add(N+I10);
                Triangles.Add(I00); Triangles.Add(N+I10); Triangles.Add(N+I00);
            }
            // Bottom wall (high-GY edge)
            if (GY == ResY - 2 || !QuadFG(GX, GY + 1))
            {
                Triangles.Add(I01); Triangles.Add(N+I01); Triangles.Add(N+I11);
                Triangles.Add(I01); Triangles.Add(N+I11); Triangles.Add(I11);
            }
        }
    }

    // Nothing visible — clear the section and bail out
    if (Triangles.Num() == 0)
    {
        MeshComponent->ClearMeshSection(0);
        return;
    }

    // Accumulate face normals into each vertex for smooth shading
    TArray<FVector> Normals;
    Normals.SetNumZeroed(2 * N);

    for (int t = 0; t + 2 < Triangles.Num(); t += 3)
    {
        const int A = Triangles[t], B = Triangles[t + 1], C = Triangles[t + 2];
        const FVector FaceNormal = FVector::CrossProduct(
            Vertices[B] - Vertices[A],
            Vertices[C] - Vertices[A]
        ).GetSafeNormal();
        Normals[A] += FaceNormal;
        Normals[B] += FaceNormal;
        Normals[C] += FaceNormal;
    }
    for (int i = 0; i < 2 * N; ++i)
        Normals[i] = Normals[i].GetSafeNormal();

    // CreateMeshSection replaces any existing section, so no bMeshInitialized needed
    MeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals,
        UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), /*bCreateCollision=*/false);
}
