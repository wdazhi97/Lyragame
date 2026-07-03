// Copyright Epic Games, Inc. All Rights Reserved.

#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "Generated/external_ai.grpc.pb.h"

#include "ExternalAI/ExternalAITransport.h"
#include "HAL/Event.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "LyraLogChannels.h"
#include "Misc/App.h"
#include "Misc/ScopeLock.h"
#include "Containers/Queue.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <utility>

namespace ExternalAIProto = lyra::external_ai::v1;

class FExternalAIGrpcTransport;

class FExternalAIStreamReactor final
	: public grpc::ClientBidiReactor<ExternalAIProto::EnvironmentMessage, ExternalAIProto::ControllerMessage>
{
public:
	explicit FExternalAIStreamReactor(FExternalAIGrpcTransport& InOwner)
		: Owner(InOwner)
	{
	}

	void Start(ExternalAIProto::ExternalAIService::Stub& Stub);
	void EnqueueWorldState(const FExternalAIWorldState& WorldState);
	void StopExternalOperations();

private:
	void EnqueueMessage(ExternalAIProto::EnvironmentMessage&& Message);
	void ReleaseExternalHold();
	void OnReadInitialMetadataDone(bool bOk) override;
	void OnReadDone(bool bOk) override;
	void OnWriteDone(bool bOk) override;
	void OnDone(const grpc::Status& Status) override;

	FExternalAIGrpcTransport& Owner;
	grpc::ClientContext Context;
	ExternalAIProto::ControllerMessage ReadMessage;
	ExternalAIProto::EnvironmentMessage WriteMessage;
	TUniquePtr<ExternalAIProto::EnvironmentMessage> PendingMessage;
	std::mutex WriteMutex;
	std::atomic<bool> bAcceptingExternalWrites{true};
	std::atomic<bool> bExternalHoldReleased{false};
	bool bWriteInFlight = false;
};

class FExternalAIGrpcTransport final : public IExternalAITransport
{
public:
	FExternalAIGrpcTransport()
		: ReactorDoneEvent(FPlatformProcess::GetSynchEventFromPool(true))
	{
	}

	~FExternalAIGrpcTransport() override
	{
		Stop();
		FPlatformProcess::ReturnSynchEventToPool(ReactorDoneEvent);
	}

	bool Start(const FExternalAITransportConfig& Config, FString& OutError) override
	{
		check(IsInGameThread());

		if (Config.ServerAddress.IsEmpty())
		{
			OutError = TEXT("ServerAddress is empty.");
			return false;
		}

		ServerAddress = Config.ServerAddress;
		const std::string Address(TCHAR_TO_UTF8(*ServerAddress));
		Channel = grpc::CreateChannel(Address, grpc::InsecureChannelCredentials());
		Stub = ExternalAIProto::ExternalAIService::NewStub(Channel);
		bStopping.store(false);
		UE_LOG(LogLyraExternalAI, Log, TEXT("Starting external AI gRPC transport to %s"), *ServerAddress);
		BeginConnection();
		return true;
	}

	void Stop() override
	{
		FExternalAIStreamReactor* ReactorToStop = nullptr;
		{
			std::lock_guard<std::mutex> Guard(ReactorMutex);
			if (bStopping.exchange(true))
			{
				return;
			}

			ReactorToStop = ActiveReactor;
			if (ReactorToStop)
			{
				ReactorToStop->StopExternalOperations();
			}
		}

		if (ReactorToStop)
		{
			ReactorDoneEvent->Wait();
		}

		Stub.reset();
		Channel.reset();
		bConnected.store(false);

		FExternalAICommandBatch Discarded;
		while (ReceivedCommands.Dequeue(Discarded))
		{
		}
	}

	bool IsConnected() const override
	{
		return bConnected.load();
	}

	void SendWorldState(FExternalAIWorldState&& WorldState) override
	{
		check(IsInGameThread());

		if (!bConnected.load() && !bStopping.load() && FPlatformTime::Seconds() >= NextReconnectTime.load())
		{
			BeginConnection();
		}

		std::lock_guard<std::mutex> Guard(ReactorMutex);
		if (ActiveReactor)
		{
			ActiveReactor->EnqueueWorldState(WorldState);
		}
	}

	bool DequeueCommand(FExternalAICommandBatch& OutCommands) override
	{
		return ReceivedCommands.Dequeue(OutCommands);
	}

	void HandleControllerMessage(const ExternalAIProto::ControllerMessage& Message)
	{
		if (!Message.has_commands())
		{
			return;
		}

		FExternalAICommandBatch Batch;
		Batch.Commands.Reserve(Message.commands().commands_size());

		for (const ExternalAIProto::AgentCommand& ProtoCommand : Message.commands().commands())
		{
			FExternalAICommand& Command = Batch.Commands.AddDefaulted_GetRef();
			Command.AgentId = ProtoCommand.agent_id();
			Command.Sequence = ProtoCommand.sequence();
			Command.BasedOnSnapshotId = ProtoCommand.based_on_snapshot_id();
			Command.CommandType = FName(UTF8_TO_TCHAR(ProtoCommand.command_type().c_str()));

			Command.ContinuousValues.Reserve(ProtoCommand.continuous_values_size());
			for (const float Value : ProtoCommand.continuous_values())
			{
				Command.ContinuousValues.Add(Value);
			}

			Command.DiscreteValues.Reserve(ProtoCommand.discrete_values_size());
			for (const int32 Value : ProtoCommand.discrete_values())
			{
				Command.DiscreteValues.Add(Value);
			}
		}

		if (!Batch.Commands.IsEmpty())
		{
			ReceivedCommands.Enqueue(MoveTemp(Batch));
		}
	}

	void HandleReactorConnected(FExternalAIStreamReactor* Reactor)
	{
		std::lock_guard<std::mutex> Guard(ReactorMutex);
		if (ActiveReactor == Reactor)
		{
			bConnected.store(true);
			UE_LOG(LogLyraExternalAI, Log, TEXT("External AI gRPC stream connected to %s"), *ServerAddress);
		}
	}

	void HandleReactorDone(FExternalAIStreamReactor* Reactor, const grpc::Status& Status)
	{
		{
			std::lock_guard<std::mutex> Guard(ReactorMutex);
			if (ActiveReactor == Reactor)
			{
				ActiveReactor = nullptr;
				bConnected.store(false);
				NextReconnectTime.store(FPlatformTime::Seconds() + 1.0);
			}
		}

		if (!bStopping.load())
		{
			UE_LOG(LogLyraExternalAI, Warning,
				TEXT("External AI gRPC stream closed (%d): %s; retrying"),
				static_cast<int32>(Status.error_code()),
				UTF8_TO_TCHAR(Status.error_message().c_str()));
		}
		ReactorDoneEvent->Trigger();
	}

private:
	void BeginConnection()
	{
		if (bStopping.load() || !Stub)
		{
			return;
		}

		FExternalAIStreamReactor* NewReactor = nullptr;
		{
			std::lock_guard<std::mutex> Guard(ReactorMutex);
			if (ActiveReactor)
			{
				return;
			}

			ReactorDoneEvent->Reset();
			NewReactor = new FExternalAIStreamReactor(*this);
			ActiveReactor = NewReactor;
			bConnected.store(false);
		}

		NewReactor->Start(*Stub);
	}

	std::shared_ptr<grpc::Channel> Channel;
	std::unique_ptr<ExternalAIProto::ExternalAIService::Stub> Stub;
	TQueue<FExternalAICommandBatch, EQueueMode::Mpsc> ReceivedCommands;
	std::mutex ReactorMutex;
	FExternalAIStreamReactor* ActiveReactor = nullptr;
	FEvent* ReactorDoneEvent = nullptr;
	std::atomic<bool> bConnected{false};
	std::atomic<bool> bStopping{true};
	FString ServerAddress;
	std::atomic<double> NextReconnectTime{0.0};
};

void FExternalAIStreamReactor::Start(ExternalAIProto::ExternalAIService::Stub& Stub)
{
	AddHold();
	Stub.async()->Connect(&Context, this);

	StartRead(&ReadMessage);

	ExternalAIProto::EnvironmentMessage RegistrationMessage;
	RegistrationMessage.mutable_registration()->set_server_name(TCHAR_TO_UTF8(FApp::GetProjectName()));
	EnqueueMessage(MoveTemp(RegistrationMessage));

	StartCall();
}

void FExternalAIStreamReactor::EnqueueWorldState(const FExternalAIWorldState& WorldState)
{
	if (!bAcceptingExternalWrites.load())
	{
		return;
	}

	ExternalAIProto::EnvironmentMessage Message;
	ExternalAIProto::WorldState* ProtoWorld = Message.mutable_world_state();
	ProtoWorld->set_snapshot_id(WorldState.SnapshotId);
	ProtoWorld->set_server_time_seconds(WorldState.ServerTimeSeconds);

	for (const FExternalAIAgentState& Agent : WorldState.Agents)
	{
		ExternalAIProto::AgentState* ProtoAgent = ProtoWorld->add_agents();
		ProtoAgent->set_agent_id(Agent.AgentId);
		ProtoAgent->set_team_id(Agent.TeamId);
		ProtoAgent->set_has_controlled_pawn(Agent.bHasControlledPawn);

		ProtoAgent->mutable_location()->set_x(Agent.Location.X);
		ProtoAgent->mutable_location()->set_y(Agent.Location.Y);
		ProtoAgent->mutable_location()->set_z(Agent.Location.Z);
		ProtoAgent->mutable_rotation()->set_pitch(Agent.Rotation.Pitch);
		ProtoAgent->mutable_rotation()->set_yaw(Agent.Rotation.Yaw);
		ProtoAgent->mutable_rotation()->set_roll(Agent.Rotation.Roll);
		ProtoAgent->mutable_velocity()->set_x(Agent.Velocity.X);
		ProtoAgent->mutable_velocity()->set_y(Agent.Velocity.Y);
		ProtoAgent->mutable_velocity()->set_z(Agent.Velocity.Z);
	}

	EnqueueMessage(MoveTemp(Message));
}

void FExternalAIStreamReactor::EnqueueMessage(ExternalAIProto::EnvironmentMessage&& Message)
{
	bool bStartWrite = false;
	{
		std::lock_guard<std::mutex> Guard(WriteMutex);
		if (!bAcceptingExternalWrites.load())
		{
			return;
		}

		if (bWriteInFlight)
		{
			// State is point-in-time data. Keep only the newest unsent snapshot.
			PendingMessage = MakeUnique<ExternalAIProto::EnvironmentMessage>(MoveTemp(Message));
			return;
		}

		WriteMessage = MoveTemp(Message);
		bWriteInFlight = true;
		bStartWrite = true;
	}

	if (bStartWrite)
	{
		StartWrite(&WriteMessage);
	}
}

void FExternalAIStreamReactor::StopExternalOperations()
{
	Context.TryCancel();
	ReleaseExternalHold();
}

void FExternalAIStreamReactor::ReleaseExternalHold()
{
	bAcceptingExternalWrites.store(false);
	if (!bExternalHoldReleased.exchange(true))
	{
		RemoveHold();
	}
}

void FExternalAIStreamReactor::OnReadInitialMetadataDone(bool bOk)
{
	if (bOk)
	{
		Owner.HandleReactorConnected(this);
	}
}

void FExternalAIStreamReactor::OnReadDone(bool bOk)
{
	if (!bOk || !bAcceptingExternalWrites.load())
	{
		ReleaseExternalHold();
		return;
	}

	Owner.HandleControllerMessage(ReadMessage);
	ReadMessage.Clear();
	StartRead(&ReadMessage);
}

void FExternalAIStreamReactor::OnWriteDone(bool bOk)
{
	if (!bOk)
	{
		ReleaseExternalHold();
		return;
	}

	bool bStartNextWrite = false;
	{
		std::lock_guard<std::mutex> Guard(WriteMutex);
		bWriteInFlight = false;
		if (PendingMessage && bAcceptingExternalWrites.load())
		{
			WriteMessage = MoveTemp(*PendingMessage);
			PendingMessage.Reset();
			bWriteInFlight = true;
			bStartNextWrite = true;
		}
	}

	if (bStartNextWrite)
	{
		StartWrite(&WriteMessage);
	}
}

void FExternalAIStreamReactor::OnDone(const grpc::Status& Status)
{
	Owner.HandleReactorDone(this, Status);
	delete this;
}

TUniquePtr<IExternalAITransport> CreateExternalAIGrpcTransport()
{
	return MakeUnique<FExternalAIGrpcTransport>();
}
