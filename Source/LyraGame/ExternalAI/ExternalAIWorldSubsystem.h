// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"

#include "ExternalAI/ExternalAITransport.h"
#include "ExternalAI/ExternalAITypes.h"

#include "ExternalAIWorldSubsystem.generated.h"

class UExternalAIAgentComponent;

/**
 * Dedicated-server bridge between the game world and an external AI process.
 * It owns state collection, transport lifecycle, receive polling, and command routing.
 */
UCLASS()
class UExternalAIWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

	void RegisterAgent(UExternalAIAgentComponent* Agent);
	void UnregisterAgent(UExternalAIAgentComponent* Agent);

	/**
	 * Replaces the factory-created transport. Intended for tests or an explicit
	 * runtime transport override.
	 */
	void SetTransport(TUniquePtr<IExternalAITransport> InTransport);

	const FExternalAIWorldState& GetLastCollectedWorldState() const { return LastCollectedWorldState; }

private:
	struct FRegisteredAgent
	{
		TWeakObjectPtr<UExternalAIAgentComponent> Component;
		int64 LastCommandSequence = 0;
	};

	bool IsDedicatedServerWorld() const;
	void StartTransport();
	void StopTransport();
	void CollectAndSendWorldState();
	void CollectWorldState(FExternalAIWorldState& OutWorldState);
	void ProcessReceivedCommands();
	void RouteCommand(const FExternalAICommand& Command);
	void RemoveInvalidAgents();
	int64 AllocateAgentId();

	TMap<int64, FRegisteredAgent> RegisteredAgents;
	TUniquePtr<IExternalAITransport> Transport;

	FExternalAIWorldState LastCollectedWorldState;

	int64 NextAgentId = 0;
	int64 NextSnapshotId = 1;
	float TimeUntilNextPublish = 0.0f;
	bool bWorldHasBegunPlay = false;
	bool bTransportStarted = false;
};
