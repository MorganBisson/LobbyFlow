// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameState.h"

#include "LobbyPlayerState.h"
#include "Net/UnrealNetwork.h"

bool FLobbyRuntimeState::operator==(const FLobbyRuntimeState& Other) const
{
	return Phase == Other.Phase
		&& ConnectedPlayerCount == Other.ConnectedPlayerCount
		&& ReadyPlayerCount == Other.ReadyPlayerCount
		&& MinimumPlayers == Other.MinimumPlayers
		&& FMath::IsNearlyEqual(CountdownDurationSeconds, Other.CountdownDurationSeconds)
		&& FMath::IsNearlyEqual(CountdownEndServerTime, Other.CountdownEndServerTime);
}

void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyGameState, LobbyState);
}

void ALobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	if (PlayerState && PlayerState->IsA<ALobbyPlayerState>())
	{
		OnLobbyPlayersChanged.Broadcast();
	}
}

void ALobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	const bool bWasLobbyPlayer = PlayerState && PlayerState->IsA<ALobbyPlayerState>();
	Super::RemovePlayerState(PlayerState);

	if (bWasLobbyPlayer)
	{
		OnLobbyPlayersChanged.Broadcast();
	}
}

double ALobbyGameState::GetCountdownRemaining() const
{
	if (LobbyState.Phase != ELobbyPhase::Countdown)
	{
		return 0.0;
	}

	const double ServerWorldTime = GetServerWorldTimeSeconds();
	return FMath::Max(0.0, LobbyState.CountdownEndServerTime - ServerWorldTime);
}

void ALobbyGameState::SetLobbyRules(
	const int32 InMinimumPlayers,
	const float InCountdownDurationSeconds)
{
	FLobbyRuntimeState NewState = LobbyState;
	NewState.MinimumPlayers = FMath::Max(1, InMinimumPlayers);
	NewState.CountdownDurationSeconds = FMath::Max(0.0f, InCountdownDurationSeconds);
	ApplyLobbyState(NewState);
}

void ALobbyGameState::SetPlayerCounts(
	const int32 InReadyPlayerCount,
	const int32 InConnectedPlayerCount)
{
	FLobbyRuntimeState NewState = LobbyState;
	NewState.ConnectedPlayerCount = FMath::Max(0, InConnectedPlayerCount);
	NewState.ReadyPlayerCount = FMath::Clamp(
		InReadyPlayerCount,
		0,
		NewState.ConnectedPlayerCount);
	ApplyLobbyState(NewState);
}

void ALobbyGameState::SetLobbyPhase(
	const ELobbyPhase InPhase,
	const double InCountdownEndServerTime)
{
	FLobbyRuntimeState NewState = LobbyState;
	NewState.Phase = InPhase;
	NewState.CountdownEndServerTime = InPhase == ELobbyPhase::Countdown
		? FMath::Max(0.0, InCountdownEndServerTime)
		: 0.0;
	ApplyLobbyState(NewState);
}

void ALobbyGameState::ApplyLobbyState(const FLobbyRuntimeState& NewState)
{
	if (!HasAuthority() || LobbyState == NewState)
	{
		return;
	}

	LobbyState = NewState;
	ForceNetUpdate();
	OnLobbyStateChanged.Broadcast(LobbyState);
}

void ALobbyGameState::OnRep_LobbyState()
{
	OnLobbyStateChanged.Broadcast(LobbyState);
}
