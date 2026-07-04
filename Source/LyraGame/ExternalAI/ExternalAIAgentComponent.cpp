// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExternalAI/ExternalAIAgentComponent.h"

#include "ExternalAI/ExternalAIWorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExternalAIAgentComponent)

UExternalAIAgentComponent::UExternalAIAgentComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UExternalAIAgentComponent::BeginPlay()
{
	Super::BeginPlay();

	const AActor* Owner = GetOwner();
	if (Owner && Owner->HasAuthority())
	{
		if (UExternalAIWorldSubsystem* Subsystem = GetWorld()->GetSubsystem<UExternalAIWorldSubsystem>())
		{
			Subsystem->RegisterAgent(this);
		}
	}
}

void UExternalAIAgentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UExternalAIWorldSubsystem* Subsystem = World->GetSubsystem<UExternalAIWorldSubsystem>())
		{
			Subsystem->UnregisterAgent(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UExternalAIAgentComponent::HandleExternalCommand(const lyra::external_ai::v1::AgentCommand& Command)
{
	check(IsInGameThread());
	check(Command.agent_id() == AgentId);
}

void UExternalAIAgentComponent::AssignAgentId(int64 NewAgentId)
{
	check(NewAgentId >= 0);
	AgentId = NewAgentId;
}
