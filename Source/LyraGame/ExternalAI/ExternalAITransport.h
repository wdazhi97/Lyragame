// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ExternalAI/ExternalAITypes.h"
#include "Templates/Function.h"

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

	virtual void SendWorldState(FExternalAIWorldState&& WorldState) = 0;
	virtual bool DequeueCommand(FExternalAICommandBatch& OutCommands) = 0;
};

using FExternalAITransportFactory = TFunction<TUniquePtr<IExternalAITransport>()>;

/** Installed by the transport module during startup. */
LYRAGAME_API void RegisterExternalAITransportFactory(FExternalAITransportFactory InFactory);
LYRAGAME_API void UnregisterExternalAITransportFactory();
LYRAGAME_API TUniquePtr<IExternalAITransport> CreateExternalAITransport();
