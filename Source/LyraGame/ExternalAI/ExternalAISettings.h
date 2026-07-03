// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"

#include "ExternalAISettings.generated.h"

/** Dedicated-server settings for the external AI environment bridge. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "External AI"))
class UExternalAISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = Connection)
	bool bEnabled = true;

	/** Address consumed by the gRPC transport, for example 127.0.0.1:50051. */
	UPROPERTY(Config, EditAnywhere, Category = Connection)
	FString ServerAddress = TEXT("127.0.0.1:50051");

	/** Frequency at which the subsystem collects and publishes world state. */
	UPROPERTY(Config, EditAnywhere, Category = State, meta = (ClampMin = "0.001", Units = "s"))
	float StatePublishIntervalSeconds = 0.1f;
};
