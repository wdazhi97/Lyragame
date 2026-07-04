// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExternalAI/ExternalAITransport.h"

TUniquePtr<IExternalAITransport> CreateExternalAIGrpcTransport();

TUniquePtr<IExternalAITransport> CreateExternalAITransport()
{
	check(IsInGameThread());
	return CreateExternalAIGrpcTransport();
}
