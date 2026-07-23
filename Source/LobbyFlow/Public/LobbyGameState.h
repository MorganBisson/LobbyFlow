// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"

#include "LobbyGameState.generated.h"

UENUM(BlueprintType)
enum class ELobbyPhase : uint8
{
	WaitingForPlayers,
	Countdown,
	StartingGame
};

/** Snapshot replicated by the server for lobby UI and flow decisions. */
USTRUCT(BlueprintType)
struct LOBBYFLOW_API FLobbyRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Lobby Flow")
	ELobbyPhase Phase = ELobbyPhase::WaitingForPlayers;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby Flow")
	int32 ConnectedPlayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby Flow")
	int32 ReadyPlayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby Flow")
	int32 MinimumPlayers = 2;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby Flow")
	float CountdownDurationSeconds = 5.0f;

	/** Server world time at which the countdown ends. Zero outside the countdown phase. */
	UPROPERTY(BlueprintReadOnly, Category = "Lobby Flow")
	double CountdownEndServerTime = 0.0;

	bool operator==(const FLobbyRuntimeState& Other) const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnLobbyRuntimeStateChanged,
	const FLobbyRuntimeState&,
	NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyPlayersChanged);

/** Replicated source of truth consumed by lobby UI on every peer. */
UCLASS()
class LOBBYFLOW_API ALobbyGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	UFUNCTION(BlueprintPure, Category = "Lobby Flow")
	FLobbyRuntimeState GetLobbyState() const
	{
		return LobbyState;
	}

	UFUNCTION(BlueprintPure, Category = "Lobby Flow")
	double GetCountdownRemaining() const;

	UPROPERTY(BlueprintAssignable, Category = "Lobby Flow")
	FOnLobbyRuntimeStateChanged OnLobbyStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lobby Flow")
	FOnLobbyPlayersChanged OnLobbyPlayersChanged;

	/** Authority-only updates used by ALobbyGameMode. */
	void SetLobbyRules(int32 InMinimumPlayers, float InCountdownDurationSeconds);
	void SetPlayerCounts(int32 InReadyPlayerCount, int32 InConnectedPlayerCount);
	void SetLobbyPhase(ELobbyPhase InPhase, double InCountdownEndServerTime = 0.0);

private:
	void ApplyLobbyState(const FLobbyRuntimeState& NewState);

	UFUNCTION()
	void OnRep_LobbyState();

	UPROPERTY(ReplicatedUsing = OnRep_LobbyState)
	FLobbyRuntimeState LobbyState;
};
