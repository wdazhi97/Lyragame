// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExternalAI/ExternalAITransport.h"
#include "Modules/ModuleManager.h"

TUniquePtr<IExternalAITransport> CreateExternalAIGrpcTransport();

class FExternalAIGrpcRuntimeModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		RegisterExternalAITransportFactory([]()
		{
			return CreateExternalAIGrpcTransport();
		});
	}

	virtual void ShutdownModule() override
	{
		UnregisterExternalAITransportFactory();
	}
};

IMPLEMENT_MODULE(FExternalAIGrpcRuntimeModule, ExternalAIGrpcRuntime)
