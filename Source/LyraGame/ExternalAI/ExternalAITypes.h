// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ExternalAITypes.generated.h"

/** Transport-only state for one externally controlled agent. */
USTRUCT(BlueprintType)
struct FExternalAIAgentState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	int64 AgentId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	bool bHasControlledPawn = false;
};

/** A point-in-time snapshot produced by the dedicated server. */
USTRUCT(BlueprintType)
struct FExternalAIWorldState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	int64 SnapshotId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	double ServerTimeSeconds = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	TArray<FExternalAIAgentState> Agents;
};

/** Generic command addressed to one AI agent. */
USTRUCT(BlueprintType)
struct FExternalAICommand
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	int64 AgentId = INDEX_NONE;

	/** Monotonically increasing per-agent sequence. Zero disables stale-command filtering. */
	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	int64 Sequence = 0;

	/** Optional snapshot on which the external controller based this command. */
	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	int64 BasedOnSnapshotId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	FName CommandType = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	TArray<float> ContinuousValues;

	UPROPERTY(BlueprintReadOnly, Category = "External AI")
	TArray<int32> DiscreteValues;
};

/** Commands received together from the external controller. */
USTRUCT()
struct FExternalAICommandBatch
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FExternalAICommand> Commands;
};
