// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyViewModel.h"

#include "Engine/World.h"
#include "Internationalization/Text.h"
#include "LobbyFlow.h"
#include "LobbyPlayerController.h"
#include "LobbyPlayerEntryViewModel.h"
#include "LobbyPlayerState.h"
#include "TimerManager.h"

namespace
{
	constexpr float CountdownRefreshIntervalSeconds = 1.0f / 60.0f;
	constexpr double MillisecondsPerSecond = 1000.0;

	int64 GetCountdownDisplayMilliseconds(const double SecondsRemaining)
	{
		return FMath::CeilToInt(FMath::Max(0.0, SecondsRemaining) * MillisecondsPerSecond);
	}

	const FNumberFormattingOptions& GetCountdownNumberFormattingOptions()
	{
		static const FNumberFormattingOptions Options = []
		{
			FNumberFormattingOptions Result;
			Result.SetUseGrouping(false);
			Result.SetMinimumIntegralDigits(2);
			Result.SetMinimumFractionalDigits(3);
			Result.SetMaximumFractionalDigits(3);
			return Result;
		}();
		return Options;
	}
}

FText ULobbyViewModel::GetPhaseDisplayText() const
{
	switch (Phase)
	{
	case ELobbyPhase::WaitingForPlayers:
		return NSLOCTEXT("LobbyFlow", "LobbyPhaseWaitingForPlayers", "Waiting for players");

	case ELobbyPhase::Countdown:
		return NSLOCTEXT("LobbyFlow", "LobbyPhaseCountdown", "Countdown");

	case ELobbyPhase::StartingGame:
		return NSLOCTEXT("LobbyFlow", "LobbyPhaseStartingGame", "Starting game");

	default:
		return FText::GetEmpty();
	}
}

FText ULobbyViewModel::GetCountdownDisplayText() const
{
	const int64 DisplayMilliseconds = GetCountdownDisplayMilliseconds(CountdownSecondsRemaining);
	const double DisplaySeconds = static_cast<double>(DisplayMilliseconds) / MillisecondsPerSecond;
	return FText::AsNumber(DisplaySeconds, &GetCountdownNumberFormattingOptions());
}

void ULobbyViewModel::BeginDestroy()
{
	ReleaseBindings();
	Super::BeginDestroy();
}

bool ULobbyViewModel::Initialize(ALobbyPlayerController* InPlayerController)
{
	if (!IsValid(InPlayerController) || !InPlayerController->IsLocalController())
	{
		Deinitialize();
		UE_LOG(LogLobbyFlow, Warning, TEXT("Lobby ViewModel initialization requires a local LobbyPlayerController."));
		return false;
	}

	UWorld* World = InPlayerController->GetWorld();
	ALobbyGameState* NewLobbyGameState = World ? World->GetGameState<ALobbyGameState>() : nullptr;
	if (!NewLobbyGameState)
	{
		Deinitialize();
		UE_LOG(LogLobbyFlow, Warning, TEXT("Lobby ViewModel initialization failed: LobbyGameState is unavailable."));
		return false;
	}

	if (IsInitialized()
		&& PlayerController.Get() == InPlayerController
		&& LobbyGameState.Get() == NewLobbyGameState)
	{
		ApplyLobbyState(NewLobbyGameState->GetLobbyState());
		RefreshPlayers();
		return true;
	}

	Deinitialize();
	PlayerController = InPlayerController;
	LobbyGameState = NewLobbyGameState;

	NewLobbyGameState->OnLobbyStateChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleLobbyStateChanged);
	NewLobbyGameState->OnLobbyPlayersChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleLobbyPlayersChanged);

	ApplyLobbyState(NewLobbyGameState->GetLobbyState());
	RefreshPlayers();
	return true;
}

void ULobbyViewModel::Deinitialize()
{
	ReleaseBindings();
	ResetViewState();
}

void ULobbyViewModel::ReleaseBindings()
{
	ClearCountdownTimer();

	if (ALobbyGameState* GameState = LobbyGameState.Get())
	{
		GameState->OnLobbyStateChanged.RemoveDynamic(
			this,
			&ThisClass::HandleLobbyStateChanged);
		GameState->OnLobbyPlayersChanged.RemoveDynamic(
			this,
			&ThisClass::HandleLobbyPlayersChanged);
	}

	UnbindFromLocalPlayerState();
	for (ULobbyPlayerEntryViewModel* PlayerViewModel : Players)
	{
		if (IsValid(PlayerViewModel))
		{
			PlayerViewModel->Deinitialize();
		}
	}

	LobbyGameState.Reset();
	PlayerController.Reset();
}

void ULobbyViewModel::SetLocalPlayerReady(const bool bNewReady)
{
	if (!bCanChangeReadyState || bIsLocalPlayerReady == bNewReady)
	{
		return;
	}

	if (ALobbyPlayerController* Controller = PlayerController.Get())
	{
		Controller->SetReady(bNewReady);
	}
}

void ULobbyViewModel::ToggleLocalPlayerReady()
{
	if (!bCanChangeReadyState)
	{
		return;
	}

	if (ALobbyPlayerController* Controller = PlayerController.Get())
	{
		Controller->ToggleReady();
	}
}

void ULobbyViewModel::HandleLobbyStateChanged(const FLobbyRuntimeState& NewState)
{
	ApplyLobbyState(NewState);
}

void ULobbyViewModel::HandleLobbyPlayersChanged()
{
	RefreshPlayers();
}

void ULobbyViewModel::HandleLocalPlayerReadyStateChanged(
	ALobbyPlayerState* InPlayerState,
	const bool bNewReady)
{
	if (InPlayerState == BoundLocalPlayerState.Get())
	{
		UE_MVVM_SET_PROPERTY_VALUE(bIsLocalPlayerReady, bNewReady);
	}
}

void ULobbyViewModel::ApplyLobbyState(const FLobbyRuntimeState& NewState)
{
	const bool bPhaseChanged = Phase != NewState.Phase;
	const bool bCountdownChanged = bPhaseChanged
		|| !FMath::IsNearlyEqual(CountdownEndServerTime, NewState.CountdownEndServerTime);

	UE_MVVM_SET_PROPERTY_VALUE(Phase, NewState.Phase);
	if (bPhaseChanged)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetPhaseDisplayText);
	}

	UE_MVVM_SET_PROPERTY_VALUE(ConnectedPlayerCount, NewState.ConnectedPlayerCount);
	UE_MVVM_SET_PROPERTY_VALUE(ReadyPlayerCount, NewState.ReadyPlayerCount);
	UE_MVVM_SET_PROPERTY_VALUE(MinimumPlayers, NewState.MinimumPlayers);
	UE_MVVM_SET_PROPERTY_VALUE(CountdownDurationSeconds, NewState.CountdownDurationSeconds);
	UE_MVVM_SET_PROPERTY_VALUE(bIsCountdownActive, NewState.Phase == ELobbyPhase::Countdown);

	CountdownEndServerTime = NewState.CountdownEndServerTime;
	UpdateCanChangeReadyState();

	if (bCountdownChanged)
	{
		RefreshCountdownTimer();
	}
}

void ULobbyViewModel::RefreshPlayers()
{
	ALobbyPlayerState* LocalPlayerState = PlayerController.IsValid()
		? PlayerController->GetLobbyPlayerState()
		: nullptr;
	BindToLocalPlayerState(LocalPlayerState);

	TMap<ALobbyPlayerState*, ULobbyPlayerEntryViewModel*> ExistingPlayerViewModels;
	ExistingPlayerViewModels.Reserve(Players.Num());
	for (ULobbyPlayerEntryViewModel* PlayerViewModel : Players)
	{
		if (!IsValid(PlayerViewModel))
		{
			continue;
		}

		if (ALobbyPlayerState* PlayerState = PlayerViewModel->GetPlayerState())
		{
			ExistingPlayerViewModels.Add(PlayerState, PlayerViewModel);
		}
		else
		{
			PlayerViewModel->Deinitialize();
		}
	}

	TArray<TObjectPtr<ULobbyPlayerEntryViewModel>> NewPlayers;

	if (ALobbyGameState* GameState = LobbyGameState.Get())
	{
		NewPlayers.Reserve(GameState->PlayerArray.Num());

		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			ALobbyPlayerState* LobbyPlayerState = Cast<ALobbyPlayerState>(PlayerState);
			if (!IsValid(LobbyPlayerState) || LobbyPlayerState->IsABot())
			{
				continue;
			}

			ULobbyPlayerEntryViewModel* PlayerViewModel =
				ExistingPlayerViewModels.FindRef(LobbyPlayerState);
			if (PlayerViewModel)
			{
				ExistingPlayerViewModels.Remove(LobbyPlayerState);
			}
			else
			{
				PlayerViewModel = NewObject<ULobbyPlayerEntryViewModel>(this);
			}

			if (PlayerViewModel->Initialize(
				LobbyPlayerState,
				LobbyPlayerState == LocalPlayerState))
			{
				NewPlayers.Add(PlayerViewModel);
			}
		}
	}

	for (const TPair<ALobbyPlayerState*, ULobbyPlayerEntryViewModel*>& RemovedPlayer : ExistingPlayerViewModels)
	{
		if (IsValid(RemovedPlayer.Value))
		{
			RemovedPlayer.Value->Deinitialize();
		}
	}

	SetPlayers(MoveTemp(NewPlayers));
	UE_MVVM_SET_PROPERTY_VALUE(
		bIsLocalPlayerReady,
		IsValid(LocalPlayerState) && LocalPlayerState->IsReady());
	UpdateCanChangeReadyState();
}

void ULobbyViewModel::BindToLocalPlayerState(ALobbyPlayerState* NewLocalPlayerState)
{
	if (BoundLocalPlayerState.Get() == NewLocalPlayerState)
	{
		return;
	}

	UnbindFromLocalPlayerState();
	if (IsValid(NewLocalPlayerState))
	{
		BoundLocalPlayerState = NewLocalPlayerState;
		NewLocalPlayerState->OnReadyStateChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleLocalPlayerReadyStateChanged);
	}
}

void ULobbyViewModel::UnbindFromLocalPlayerState()
{
	if (ALobbyPlayerState* LocalPlayerState = BoundLocalPlayerState.Get())
	{
		LocalPlayerState->OnReadyStateChanged.RemoveDynamic(
			this,
			&ThisClass::HandleLocalPlayerReadyStateChanged);
	}

	BoundLocalPlayerState.Reset();
}

void ULobbyViewModel::SetPlayers(
	TArray<TObjectPtr<ULobbyPlayerEntryViewModel>>&& NewPlayers)
{
	if (Players == NewPlayers)
	{
		return;
	}

	Players = MoveTemp(NewPlayers);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Players);
}

void ULobbyViewModel::UpdateCanChangeReadyState()
{
	const bool bHasLocalPlayerState = PlayerController.IsValid()
		&& IsValid(PlayerController->GetLobbyPlayerState());
	const bool bNewCanChangeReadyState = IsInitialized()
		&& bHasLocalPlayerState
		&& Phase != ELobbyPhase::StartingGame;
	UE_MVVM_SET_PROPERTY_VALUE(bCanChangeReadyState, bNewCanChangeReadyState);
}

void ULobbyViewModel::SetCountdownSecondsRemaining(const double NewSecondsRemaining)
{
	const double ClampedSecondsRemaining = FMath::Max(0.0, NewSecondsRemaining);
	const bool bDisplayTextChanged =
		GetCountdownDisplayMilliseconds(CountdownSecondsRemaining)
		!= GetCountdownDisplayMilliseconds(ClampedSecondsRemaining);

	UE_MVVM_SET_PROPERTY_VALUE(CountdownSecondsRemaining, ClampedSecondsRemaining);
	if (bDisplayTextChanged)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCountdownDisplayText);
	}
}

void ULobbyViewModel::RefreshCountdownTimer()
{
	ClearCountdownTimer();

	if (!bIsCountdownActive)
	{
		SetCountdownSecondsRemaining(0.0);
		return;
	}

	UpdateCountdown();
	if (CountdownSecondsRemaining <= 0.0)
	{
		return;
	}

	ALobbyPlayerController* Controller = PlayerController.Get();
	UWorld* World = Controller ? Controller->GetWorld() : nullptr;
	if (World)
	{
		FTimerManagerTimerParameters TimerParameters;
		TimerParameters.bLoop = true;
		TimerParameters.bMaxOncePerFrame = true;
		World->GetTimerManager().SetTimer(
			CountdownTimerHandle,
			this,
			&ThisClass::UpdateCountdown,
			CountdownRefreshIntervalSeconds,
			TimerParameters);
	}
}

void ULobbyViewModel::UpdateCountdown()
{
	const ALobbyGameState* GameState = LobbyGameState.Get();
	const double CountdownRemaining = GameState && bIsCountdownActive
		? GameState->GetCountdownRemaining()
		: 0.0;
	SetCountdownSecondsRemaining(CountdownRemaining);

	if (CountdownRemaining <= 0.0)
	{
		ClearCountdownTimer();
		return;
	}
}

void ULobbyViewModel::ClearCountdownTimer()
{
	ALobbyPlayerController* Controller = PlayerController.Get();
	UWorld* World = Controller ? Controller->GetWorld() : nullptr;
	if (World)
	{
		World->GetTimerManager().ClearTimer(CountdownTimerHandle);
	}
	else
	{
		CountdownTimerHandle.Invalidate();
	}
}

void ULobbyViewModel::ResetViewState()
{
	const bool bPhaseChanged = Phase != ELobbyPhase::WaitingForPlayers;
	UE_MVVM_SET_PROPERTY_VALUE(Phase, ELobbyPhase::WaitingForPlayers);
	if (bPhaseChanged)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetPhaseDisplayText);
	}

	UE_MVVM_SET_PROPERTY_VALUE(ConnectedPlayerCount, 0);
	UE_MVVM_SET_PROPERTY_VALUE(ReadyPlayerCount, 0);
	UE_MVVM_SET_PROPERTY_VALUE(MinimumPlayers, 0);
	UE_MVVM_SET_PROPERTY_VALUE(CountdownDurationSeconds, 0.0f);
	UE_MVVM_SET_PROPERTY_VALUE(bIsCountdownActive, false);
	SetCountdownSecondsRemaining(0.0);
	UE_MVVM_SET_PROPERTY_VALUE(bIsLocalPlayerReady, false);
	UE_MVVM_SET_PROPERTY_VALUE(bCanChangeReadyState, false);

	CountdownEndServerTime = 0.0;
	TArray<TObjectPtr<ULobbyPlayerEntryViewModel>> EmptyPlayers;
	SetPlayers(MoveTemp(EmptyPlayers));
}
