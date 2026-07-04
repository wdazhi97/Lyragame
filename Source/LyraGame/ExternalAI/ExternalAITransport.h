// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/UnrealString.h"
#include "ExternalAI/ExternalAIProto.h"
#include "Templates/UniquePtr.h"

struct FExternalAITransportConfig
{
	FString ServerAddress;
};

/**
 * Transport boundary for the environment bridge.
 *
 * Implementations may perform I/O on worker threads, but these methods are
 * called on the game thread. I/O threads must never access UObjects.
 */
class LYRAGAME_API IExternalAITransport
{
public:
	virtual ~IExternalAITransport() = default;

	virtual bool Start(const FExternalAITransportConfig& Config, FString& OutError) = 0;
	virtual void Stop() = 0;
	virtual bool IsConnected() const = 0;

	virtual void SendWorldState(lyra::external_ai::v1::WorldState&& WorldState) = 0;
	virtual bool DequeueCommand(lyra::external_ai::v1::AgentCommandBatch& OutCommands) = 0;
};

LYRAGAME_API TUniquePtr<IExternalAITransport> CreateExternalAITransport();
