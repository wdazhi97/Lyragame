// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "ExternalAI/ExternalAIProto.h"

#include "ExternalAIAgentComponent.generated.h"

class UExternalAIWorldSubsystem;

/**
 * Endpoint for commands routed to an AI controller by the external AI subsystem.
 * This component does not own transport or world-state collection.
 */
UCLASS(Blueprintable, ClassGroup = AI, meta = (BlueprintSpawnableComponent))
class UExternalAIAgentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UExternalAIAgentComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "External AI")
	int64 GetAgentId() const { return AgentId; }

	/** Called by the subsystem on the game thread after routing and freshness checks. */
	virtual void HandleExternalCommand(const lyra::external_ai::v1::AgentCommand& Command);

private:
	friend UExternalAIWorldSubsystem;

	void AssignAgentId(int64 NewAgentId);

	/** Set a stable non-negative identity, or leave at -1 for automatic assignment. */
	UPROPERTY(EditDefaultsOnly, Category = "External AI")
	int64 AgentId = INDEX_NONE;
};
