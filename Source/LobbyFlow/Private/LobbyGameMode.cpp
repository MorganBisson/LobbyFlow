// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"

#include "Engine/World.h"
#include "LobbyFlow.h"
#include "LobbyGameState.h"
#include "LobbyPlayerController.h"
#include "LobbyPlayerState.h"
#include "TimerManager.h"

ALobbyGameMode::ALobbyGameMode()
{
	GameStateClass = ALobbyGameState::StaticClass();
	PlayerControllerClass = ALobbyPlayerController::StaticClass();
	PlayerStateClass = ALobbyPlayerState::StaticClass();
	bDelayedStart = true;
}

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (ALobbyGameState* LobbyGameState = GetGameState<ALobbyGameState>())
	{
		LobbyGameState->SetLobbyRules(MinimumPlayers, CountdownDurationSeconds);
		LobbyGameState->SetLobbyPhase(ELobbyPhase::WaitingForPlayers);
	}

	RefreshLobbyState();
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (ALobbyPlayerState* LobbyPlayerState = NewPlayer
		? NewPlayer->GetPlayerState<ALobbyPlayerState>()
		: nullptr)
	{
		LobbyPlayerState->SetReadyState(false);
	}

	CancelCountdown();
	RefreshLobbyState();
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	const ALobbyPlayerState* ExitingPlayerState = Exiting
		? Exiting->GetPlayerState<ALobbyPlayerState>()
		: nullptr;

	CancelCountdown();
	Super::Logout(Exiting);
	RefreshLobbyState(ExitingPlayerState);
}

void ALobbyGameMode::SetPlayerReady(
	ALobbyPlayerController* PlayerController,
	const bool bNewReady)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	const ALobbyGameState* LobbyGameState = GetGameState<ALobbyGameState>();
	if (!LobbyGameState || LobbyGameState->GetLobbyState().Phase == ELobbyPhase::StartingGame)
	{
		return;
	}

	ALobbyPlayerState* LobbyPlayerState = PlayerController->GetLobbyPlayerState();
	if (LobbyPlayerState && LobbyPlayerState->SetReadyState(bNewReady))
	{
		UE_LOG(
			LogLobbyFlow,
			Log,
			TEXT("Player '%s' is now %s."),
			*LobbyPlayerState->GetPlayerName(),
			bNewReady ? TEXT("ready") : TEXT("not ready"));
		RefreshLobbyState();
	}
}

void ALobbyGameMode::RefreshLobbyState(const ALobbyPlayerState* IgnoredPlayerState)
{
	ALobbyGameState* LobbyGameState = GetGameState<ALobbyGameState>();
	if (!LobbyGameState || LobbyGameState->GetLobbyState().Phase == ELobbyPhase::StartingGame)
	{
		return;
	}

	int32 ConnectedPlayerCount = 0;
	int32 ReadyPlayerCount = 0;
	CountLobbyPlayers(IgnoredPlayerState, ConnectedPlayerCount, ReadyPlayerCount);
	LobbyGameState->SetPlayerCounts(ReadyPlayerCount, ConnectedPlayerCount);

	const bool bCanStart = AreStartConditionsMet(ConnectedPlayerCount, ReadyPlayerCount);
	const ELobbyPhase CurrentPhase = LobbyGameState->GetLobbyState().Phase;
	if (bCanStart && CurrentPhase == ELobbyPhase::WaitingForPlayers)
	{
		StartCountdown();
	}
	else if (!bCanStart && CurrentPhase == ELobbyPhase::Countdown)
	{
		CancelCountdown();
	}
}

void ALobbyGameMode::CountLobbyPlayers(
	const ALobbyPlayerState* IgnoredPlayerState,
	int32& OutConnectedPlayerCount,
	int32& OutReadyPlayerCount) const
{
	OutConnectedPlayerCount = 0;
	OutReadyPlayerCount = 0;

	const ALobbyGameState* LobbyGameState = GetGameState<ALobbyGameState>();
	if (!LobbyGameState)
	{
		return;
	}

	for (APlayerState* PlayerState : LobbyGameState->PlayerArray)
	{
		const ALobbyPlayerState* LobbyPlayerState = Cast<ALobbyPlayerState>(PlayerState);
		if (!IsValid(LobbyPlayerState)
			|| LobbyPlayerState == IgnoredPlayerState
			|| LobbyPlayerState->IsABot())
		{
			continue;
		}

		++OutConnectedPlayerCount;
		if (LobbyPlayerState->IsReady())
		{
			++OutReadyPlayerCount;
		}
	}
}

bool ALobbyGameMode::AreStartConditionsMet(
	const int32 ConnectedPlayerCount,
	const int32 ReadyPlayerCount) const
{
	return ConnectedPlayerCount >= FMath::Max(1, MinimumPlayers)
		&& ReadyPlayerCount == ConnectedPlayerCount;
}

void ALobbyGameMode::StartCountdown()
{
	ALobbyGameState* LobbyGameState = GetGameState<ALobbyGameState>();
	UWorld* World = GetWorld();
	if (!LobbyGameState || !World || LobbyGameState->GetLobbyState().Phase != ELobbyPhase::WaitingForPlayers)
	{
		return;
	}

	const float SafeDuration = FMath::Max(0.0f, CountdownDurationSeconds);
	const double ServerWorldTime = LobbyGameState->GetServerWorldTimeSeconds();
	LobbyGameState->SetLobbyPhase(
		ELobbyPhase::Countdown,
		ServerWorldTime + static_cast<double>(SafeDuration));
	UE_LOG(LogLobbyFlow, Log, TEXT("Lobby countdown started for %.1f seconds."), SafeDuration);

	if (SafeDuration <= 0.0f)
	{
		HandleCountdownFinished();
		return;
	}

	World->GetTimerManager().SetTimer(
		CountdownTimerHandle,
		this,
		&ALobbyGameMode::HandleCountdownFinished,
		SafeDuration,
		false);
}

void ALobbyGameMode::CancelCountdown()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(CountdownTimerHandle);
	}

	ALobbyGameState* LobbyGameState = GetGameState<ALobbyGameState>();
	if (LobbyGameState && LobbyGameState->GetLobbyState().Phase == ELobbyPhase::Countdown)
	{
		LobbyGameState->SetLobbyPhase(ELobbyPhase::WaitingForPlayers);
		UE_LOG(LogLobbyFlow, Log, TEXT("Lobby countdown cancelled."));
	}
}

void ALobbyGameMode::HandleCountdownFinished()
{
	ALobbyGameState* LobbyGameState = GetGameState<ALobbyGameState>();
	if (!LobbyGameState || LobbyGameState->GetLobbyState().Phase != ELobbyPhase::Countdown)
	{
		return;
	}

	int32 ConnectedPlayerCount = 0;
	int32 ReadyPlayerCount = 0;
	CountLobbyPlayers(nullptr, ConnectedPlayerCount, ReadyPlayerCount);
	LobbyGameState->SetPlayerCounts(ReadyPlayerCount, ConnectedPlayerCount);
	if (!AreStartConditionsMet(ConnectedPlayerCount, ReadyPlayerCount))
	{
		CancelCountdown();
		return;
	}

	if (!OnMatchStartRequested.IsBound())
	{
		UE_LOG(LogLobbyFlow, Error, TEXT("Cannot start the match: OnMatchStartRequested has no bound listener."));
		CancelCountdown();
		return;
	}

	LobbyGameState->SetLobbyPhase(ELobbyPhase::StartingGame);
	UE_LOG(LogLobbyFlow, Log, TEXT("Lobby is ready to start the match."));
	OnMatchStartRequested.Broadcast();
}
