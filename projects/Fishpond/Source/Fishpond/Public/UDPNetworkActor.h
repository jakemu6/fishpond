// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Common/UdpSocketReceiver.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "UDPNetworkActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBinaryMessageReceived, const TArray<uint8>&, BinaryData);

UCLASS()
class FISHPOND_API AUDPNetworkActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AUDPNetworkActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	// Network Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings")
	FString RemoteIP = "127.0.0.1";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings")
	int32 RemotePort = 5008;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings")
	int32 LocalPort = 55555;

	UFUNCTION(BlueprintCallable, Category = "Network|Messages")
	bool SendBinaryMessage(const TArray<uint8>& BinaryData);

	UPROPERTY(BlueprintAssignable, Category = "Network|Events")
	FOnBinaryMessageReceived OnBinaryMessageReceived;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Debug")
	bool bVerboseLogging = false;

protected:

	void InitializeUDP();
	void ShutdownUDP();
	void HandleUDPReceived(const FArrayReaderPtr& DataPtr, const FIPv4Endpoint& Endpoint);
	bool SendUDPMessage(const TArray<uint8>& Data);

	// Message Processing Helper
	void ProcessReceivedData(const TArray<uint8>& Data);

private:
	FSocket* UDPSocket;
	FUdpSocketReceiver* UDPReceiver;
	TSharedPtr<FInternetAddr> RemoteEndpoint;

	static const uint8 MESSAGE_DELIMITER = '\r'; // Carriage return delimiter
};
