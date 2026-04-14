// Fill out your copyright notice in the Description page of Project Settings.


#include "UDPNetworkActor.h"


// Sets default values
AUDPNetworkActor::AUDPNetworkActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	RemoteEndpoint = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
}

// Called when the game starts or when spawned
void AUDPNetworkActor::BeginPlay()
{
	Super::BeginPlay();
	InitializeUDP();
}

void AUDPNetworkActor::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	ShutdownUDP();
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AUDPNetworkActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AUDPNetworkActor::InitializeUDP() {
	// Create socket
	UDPSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_DGram, TEXT("UDPSocket"), true);
	
	if (!UDPSocket) {
		UE_LOG(LogTemp, Error, TEXT("Failed to create UDP socket"));
		return;
	}

	FIPv4Address LocalAddr;
	FIPv4Address::Parse(TEXT("0.0.0.0"), LocalAddr);
	TSharedRef<FInternetAddr> LocalEndpoint = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	LocalEndpoint->SetIp(LocalAddr.Value);
	LocalEndpoint->SetPort(LocalPort);

	if (!UDPSocket->Bind(*LocalEndpoint)) {
		UE_LOG(LogTemp, Error, TEXT("Failed to create UDP socket at port %d"), LocalPort);
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(UDPSocket);
		UDPSocket = nullptr;
		return;
	}

	bool bIsValid = false;
	RemoteEndpoint->SetPort(RemotePort);
	RemoteEndpoint->SetIp(*RemoteIP, bIsValid);

	if (!bIsValid) {
		UE_LOG(LogTemp, Error, TEXT("Failed to create UDP socket at IP address: %s"), *RemoteIP);
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(UDPSocket);
		UDPSocket = nullptr;
		return;
	}

	UDPReceiver = new FUdpSocketReceiver(UDPSocket, FTimespan::FromMilliseconds(100), TEXT("SitaraUDPReceiver"));
	UDPReceiver->OnDataReceived().BindUObject(this, &AUDPNetworkActor::HandleUDPReceived);
	UDPReceiver->Start();
}

void AUDPNetworkActor::ShutdownUDP() {
	if (UDPReceiver) {
		UDPReceiver->Stop();
		delete UDPReceiver;
		UDPReceiver = nullptr;
	}

	if (UDPSocket) {
		UDPSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(UDPSocket);
		UDPSocket = nullptr;
	}
}

void AUDPNetworkActor::HandleUDPReceived(const FArrayReaderPtr& DataPtr, const FIPv4Endpoint& Endpoint) {
	if (!DataPtr.IsValid() || DataPtr->Num() <= 0) {
		return;
	}

	TArray<uint8> ReceivedData;
	ReceivedData.SetNumUninitialized(DataPtr->Num());
	FMemory::Memcpy(ReceivedData.GetData(), DataPtr->GetData(), DataPtr->Num());

	// Remove carriage return delimiter if present
	if (ReceivedData.Num() > 0 && ReceivedData.Last() == MESSAGE_DELIMITER) {
		ReceivedData.RemoveAt(ReceivedData.Num() - 1);
	}

	ProcessReceivedData(ReceivedData);

	if (bVerboseLogging) {
		FString SenderIP = Endpoint.Address.ToString();
		UE_LOG(LogTemp, Log, TEXT("Received %d bytes from %s:%d"), ReceivedData.Num(), *SenderIP, Endpoint.Port);
	}
}

bool AUDPNetworkActor::SendUDPMessage(const TArray<uint8>& Data) {
	if (!UDPSocket || !RemoteEndpoint.IsValid()) {
		return false;
	}

	int32 BytesSent = 0;
	bool  bSuccess = UDPSocket->SendTo(Data.GetData(), Data.Num(), BytesSent, *RemoteEndpoint);

	if (bVerboseLogging) {
		if (bSuccess) {
			UE_LOG(LogTemp, Log, TEXT("Sent %d bytes via UDP to %s:%d"), BytesSent, *RemoteIP, RemotePort);
		}
		else {
			UE_LOG(LogTemp, Log, TEXT("Failed to send data via UDP to %s:%d"), *RemoteIP, RemotePort);
		}
	}

	return bSuccess && (BytesSent == Data.Num());
}

void AUDPNetworkActor::ProcessReceivedData(const TArray<uint8>& Data) {
	if (Data.Num() == 0) {
		return;
	}

	// Treat as raw binary data
	if (IsInGameThread()) {
		OnBinaryMessageReceived.Broadcast(Data);
	}
	else {
		AsyncTask(ENamedThreads::GameThread, [this, Data]() {
			OnBinaryMessageReceived.Broadcast(Data);
			});
	}
}

bool AUDPNetworkActor::SendBinaryMessage(const TArray<uint8>& BinaryData) {
	return SendUDPMessage(BinaryData);
}