// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExternalAI/ExternalAITransport.h"

namespace
{
	FExternalAITransportFactory GExternalAITransportFactory;
}

void RegisterExternalAITransportFactory(FExternalAITransportFactory InFactory)
{
	check(IsInGameThread());
	GExternalAITransportFactory = MoveTemp(InFactory);
}

void UnregisterExternalAITransportFactory()
{
	check(IsInGameThread());
	GExternalAITransportFactory = nullptr;
}

TUniquePtr<IExternalAITransport> CreateExternalAITransport()
{
	check(IsInGameThread());
	return GExternalAITransportFactory ? GExternalAITransportFactory() : nullptr;
}
