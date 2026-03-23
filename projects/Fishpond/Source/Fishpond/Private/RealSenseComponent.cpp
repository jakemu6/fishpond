#include "RealSenseComponent.h"
#include "Kismet/GameplayStatics.h"

URealSenseComponent::URealSenseComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void URealSenseComponent::BeginPlay()
{
    Super::BeginPlay();
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
        Align.Emplace(RS2_STREAM_COLOR);

        // Enable depth and color streams
        Config.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
        Config.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 30);

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
        // Poll for frames without blocking
        rs2::frameset Frames;
        if (Pipeline.poll_for_frames(&Frames) != 0)
        {
            ProcessDepthFrame(Frames);
        }
    }
    catch (const rs2::error& e)
    {
        UE_LOG(LogTemp, Warning, TEXT("RealSense: Frame error: %s"),
            *FString(e.what()));
    }
}

void URealSenseComponent::ProcessDepthFrame(const rs2::frameset& Frames)
{
    // Align depth to color frame
    auto AlignedFrames = Align.GetValue().process(Frames);
    auto DepthFrame = AlignedFrames.get_depth_frame();

    if (!DepthFrame) return;

    int Width  = DepthFrame.get_width();
    int Height = DepthFrame.get_height();

    // Sample the center region of the frame for player detection
    int CenterX = Width  / 2;
    int CenterY = Height / 2;
    int RegionSize = 20;

    float ClosestDepth = MaxDepth;
    float SumX = 0.f, SumY = 0.f, SumZ = 0.f;
    int ValidSamples = 0;

    // Get depth intrinsics for deprojection
    auto DepthIntrinsics = DepthFrame.get_profile()
        .as<rs2::video_stream_profile>().get_intrinsics();

    for (int Y = CenterY - RegionSize; Y < CenterY + RegionSize; Y++)
    {
        for (int X = CenterX - RegionSize; X < CenterX + RegionSize; X++)
        {
            float Depth = DepthFrame.get_distance(X, Y);

            if (Depth > MinDepth && Depth < MaxDepth)
            {
                // Deproject pixel to 3D point
                float Pixel[2] = { (float)X, (float)Y };
                float Point[3];
                rs2_deproject_pixel_to_point(Point, &DepthIntrinsics, Pixel, Depth);

                SumX += Point[0];
                SumY += Point[1];
                SumZ += Point[2];
                ValidSamples++;

                if (Depth < ClosestDepth)
                    ClosestDepth = Depth;
            }
        }
    }

    bool bCurrentlyTracking = ValidSamples > 0;

    if (bCurrentlyTracking)
    {
        // Average the sampled points
        float AvgX = SumX / ValidSamples;
        float AvgY = SumY / ValidSamples;
        float AvgZ = SumZ / ValidSamples;

        TrackedPlayer.Position = RealSenseToUnreal(AvgX, AvgY, AvgZ);
        TrackedPlayer.Depth    = ClosestDepth;
        TrackedPlayer.bIsValid = true;

        OnPlayerDetected.Broadcast(TrackedPlayer);
    }
    else
    {
        TrackedPlayer.bIsValid = false;

        // Only fire lost event once when tracking is lost
        if (bWasTrackingLastFrame)
            OnPlayerLost.Broadcast();
    }

    bWasTrackingLastFrame = bCurrentlyTracking;
}

FVector URealSenseComponent::RealSenseToUnreal(float X, float Y, float Z) const
{
    // RealSense uses right-handed Y-up coordinate system
    // Unreal uses left-handed Z-up coordinate system
    return FVector(
         Z * DepthToUnrealScale,   // RealSense Z (forward) → Unreal X (forward)
        -X * DepthToUnrealScale,   // RealSense X (right)   → Unreal Y (left-handed flip)
        -Y * DepthToUnrealScale    // RealSense Y (down)    → Unreal Z (up, flipped)
    ) + GetOwner()->GetActorLocation();
}