// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExternalAI/ExternalAIWorldSubsystem.h"

#include "ExternalAI/ExternalAIAgentComponent.h"
#include "ExternalAI/ExternalAISettings.h"
#include "ExternalAI/ExternalAITransport.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "LyraLogChannels.h"
#include "Teams/LyraTeamSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExternalAIWorldSubsystem)

bool UExternalAIWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UExternalAIWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Transport = CreateExternalAITransport();
}

void UExternalAIWorldSubsystem::Deinitialize()
{
	StopTransport();
	RegisteredAgents.Reset();

	Super::Deinitialize();
}

void UExternalAIWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	bWorldHasBegunPlay = true;
	TimeUntilNextPublish = 0.0f;

	if (IsDedicatedServerWorld() && GetDefault<UExternalAISettings>()->bEnabled)
	{
		StartTransport();
	}
}

void UExternalAIWorldSubsystem::Tick(float DeltaTime)
{
	ProcessReceivedCommands();
	RemoveInvalidAgents();

	const UExternalAISettings* Settings = GetDefault<UExternalAISettings>();
	TimeUntilNextPublish -= DeltaTime;
	if (TimeUntilNextPublish <= 0.0f)
	{
		CollectAndSendWorldState();
		TimeUntilNextPublish = FMath::Max(Settings->StatePublishIntervalSeconds, UE_SMALL_NUMBER);
	}
}

TStatId UExternalAIWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UExternalAIWorldSubsystem, STATGROUP_Tickables);
}

bool UExternalAIWorldSubsystem::IsTickable() const
{
	const UExternalAISettings* Settings = GetDefault<UExternalAISettings>();
	return bWorldHasBegunPlay && IsDedicatedServerWorld() && Settings->bEnabled;
}

void UExternalAIWorldSubsystem::RegisterAgent(UExternalAIAgentComponent* Agent)
{
	if (!Agent || !IsDedicatedServerWorld())
	{
		return;
	}

	int64 AgentId = Agent->GetAgentId();
	if (AgentId < 0)
	{
		AgentId = AllocateAgentId();
		Agent->AssignAgentId(AgentId);
	}

	if (const FRegisteredAgent* Existing = RegisteredAgents.Find(AgentId))
	{
		if (Existing->Component.Get() != Agent)
		{
			UE_LOG(LogLyraExternalAI, Error,
				TEXT("Cannot register %s with external AI agent ID %lld; the ID is already used by %s"),
				*GetPathNameSafe(Agent), AgentId, *GetPathNameSafe(Existing->Component.Get()));
		}
		return;
	}

	FRegisteredAgent& Entry = RegisteredAgents.Add(AgentId);
	Entry.Component = Agent;
	NextAgentId = FMath::Max(NextAgentId, AgentId + 1);

	UE_LOG(LogLyraExternalAI, Verbose, TEXT("Registered external AI agent %lld: %s"), AgentId, *GetPathNameSafe(Agent));
}

void UExternalAIWorldSubsystem::UnregisterAgent(UExternalAIAgentComponent* Agent)
{
	if (!Agent)
	{
		return;
	}

	const int64 AgentId = Agent->GetAgentId();
	if (FRegisteredAgent* Entry = RegisteredAgents.Find(AgentId))
	{
		if (Entry->Component.Get() == Agent)
		{
			RegisteredAgents.Remove(AgentId);
			UE_LOG(LogLyraExternalAI, Verbose, TEXT("Unregistered external AI agent %lld"), AgentId);
		}
	}
}

void UExternalAIWorldSubsystem::SetTransport(TUniquePtr<IExternalAITransport> InTransport)
{
	StopTransport();
	Transport = MoveTemp(InTransport);

	if (bWorldHasBegunPlay && IsDedicatedServerWorld() && GetDefault<UExternalAISettings>()->bEnabled)
	{
		StartTransport();
	}
}

bool UExternalAIWorldSubsystem::IsDedicatedServerWorld() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() == NM_DedicatedServer;
}

void UExternalAIWorldSubsystem::StartTransport()
{
	if (bTransportStarted || !Transport)
	{
		if (!Transport)
		{
			UE_LOG(LogLyraExternalAI, Warning,
				TEXT("External AI is enabled, but no transport factory is registered."));
		}
		return;
	}

	const UExternalAISettings* Settings = GetDefault<UExternalAISettings>();
	FExternalAITransportConfig Config;
	Config.ServerAddress = Settings->ServerAddress;

	FString Error;
	bTransportStarted = Transport->Start(Config, Error);
	if (!bTransportStarted)
	{
		UE_LOG(LogLyraExternalAI, Error, TEXT("Failed to start external AI transport: %s"), *Error);
	}
}

void UExternalAIWorldSubsystem::StopTransport()
{
	if (Transport && bTransportStarted)
	{
		Transport->Stop();
	}

	bTransportStarted = false;
}

void UExternalAIWorldSubsystem::CollectAndSendWorldState()
{
	lyra::external_ai::v1::WorldState NewState;
	CollectWorldState(NewState);
	LastCollectedWorldState = NewState;

	if (Transport && bTransportStarted)
	{
		Transport->SendWorldState(MoveTemp(NewState));
	}
}

void UExternalAIWorldSubsystem::CollectWorldState(lyra::external_ai::v1::WorldState& OutWorldState)
{
	check(IsInGameThread());

	OutWorldState.set_snapshot_id(NextSnapshotId++);
	OutWorldState.set_server_time_seconds(GetWorld()->GetTimeSeconds());
	OutWorldState.mutable_agents()->Reserve(RegisteredAgents.Num());

	const ULyraTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<ULyraTeamSubsystem>();

	for (const TPair<int64, FRegisteredAgent>& Pair : RegisteredAgents)
	{
		const UExternalAIAgentComponent* AgentComponent = Pair.Value.Component.Get();
		const AController* Controller = AgentComponent ? Cast<AController>(AgentComponent->GetOwner()) : nullptr;
		if (!Controller)
		{
			continue;
		}

		lyra::external_ai::v1::AgentState* AgentState = OutWorldState.add_agents();
		AgentState->set_agent_id(Pair.Key);
		AgentState->set_team_id(TeamSubsystem ? TeamSubsystem->FindTeamFromObject(Controller) : INDEX_NONE);

		if (const APawn* Pawn = Controller->GetPawn())
		{
			AgentState->set_has_controlled_pawn(true);

			const FVector Location = Pawn->GetActorLocation();
			AgentState->mutable_location()->set_x(Location.X);
			AgentState->mutable_location()->set_y(Location.Y);
			AgentState->mutable_location()->set_z(Location.Z);

			const FRotator Rotation = Pawn->GetActorRotation();
			AgentState->mutable_rotation()->set_pitch(Rotation.Pitch);
			AgentState->mutable_rotation()->set_yaw(Rotation.Yaw);
			AgentState->mutable_rotation()->set_roll(Rotation.Roll);

			const FVector Velocity = Pawn->GetVelocity();
			AgentState->mutable_velocity()->set_x(Velocity.X);
			AgentState->mutable_velocity()->set_y(Velocity.Y);
			AgentState->mutable_velocity()->set_z(Velocity.Z);
		}
		else
		{
			const FRotator Rotation = Controller->GetControlRotation();
			AgentState->mutable_rotation()->set_pitch(Rotation.Pitch);
			AgentState->mutable_rotation()->set_yaw(Rotation.Yaw);
			AgentState->mutable_rotation()->set_roll(Rotation.Roll);
		}
	}
}

void UExternalAIWorldSubsystem::ProcessReceivedCommands()
{
	if (!Transport || !bTransportStarted)
	{
		return;
	}

	lyra::external_ai::v1::AgentCommandBatch Batch;
	while (Transport->DequeueCommand(Batch))
	{
		for (const lyra::external_ai::v1::AgentCommand& Command : Batch.commands())
		{
			RouteCommand(Command);
		}
		Batch.Clear();
	}
}

void UExternalAIWorldSubsystem::RouteCommand(const lyra::external_ai::v1::AgentCommand& Command)
{
	check(IsInGameThread());

	const int64 AgentId = Command.agent_id();
	const int64 Sequence = Command.sequence();
	FRegisteredAgent* Entry = RegisteredAgents.Find(AgentId);
	if (!Entry)
	{
		UE_LOG(LogLyraExternalAI, Verbose, TEXT("Dropped command for unknown agent %lld"), AgentId);
		return;
	}

	if (Sequence != 0 && Sequence <= Entry->LastCommandSequence)
	{
		UE_LOG(LogLyraExternalAI, VeryVerbose,
			TEXT("Dropped stale command %lld for agent %lld (last %lld)"),
			Sequence, AgentId, Entry->LastCommandSequence);
		return;
	}

	UExternalAIAgentComponent* Component = Entry->Component.Get();
	if (!Component)
	{
		RegisteredAgents.Remove(AgentId);
		return;
	}

	if (Sequence != 0)
	{
		Entry->LastCommandSequence = Sequence;
	}

	Component->HandleExternalCommand(Command);
}

void UExternalAIWorldSubsystem::RemoveInvalidAgents()
{
	for (auto It = RegisteredAgents.CreateIterator(); It; ++It)
	{
		if (!It.Value().Component.IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

int64 UExternalAIWorldSubsystem::AllocateAgentId()
{
	while (RegisteredAgents.Contains(NextAgentId))
	{
		++NextAgentId;
	}

	return NextAgentId++;
}
