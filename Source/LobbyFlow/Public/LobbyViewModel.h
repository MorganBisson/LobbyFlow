// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LobbyGameState.h"
#include "MVVMViewModelBase.h"

#include "LobbyViewModel.generated.h"

class ALobbyGameState;
class ALobbyPlayerController;
class ALobbyPlayerState;
class ULobbyPlayerEntryViewModel;

/** Local, event-driven projection of the replicated lobby state for UMG. */
UCLASS(BlueprintType, Blueprintable, Transient)
class LOBBYFLOW_API ULobbyViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;

	/** Binds this ViewModel to the local controller and its replicated lobby state. */
	UFUNCTION(BlueprintCallable, Category = "Lobby Flow|View Model")
	bool Initialize(ALobbyPlayerController* InPlayerController);

	/** Releases all gameplay delegates and the local countdown timer. */
	UFUNCTION(BlueprintCallable, Category = "Lobby Flow|View Model")
	void Deinitialize();

	/** Forwards a ready request through the local PlayerController. */
	UFUNCTION(BlueprintCallable, Category = "Lobby Flow|View Model")
	void SetLocalPlayerReady(bool bNewReady);

	/** Toggles the authoritative ready state through the local PlayerController. */
	UFUNCTION(BlueprintCallable, Category = "Lobby Flow|View Model")
	void ToggleLocalPlayerReady();

	bool IsInitialized() const
	{
		return PlayerController.IsValid() && LobbyGameState.IsValid();
	}
	ELobbyPhase GetPhase() const { return Phase; }

	/** Returns the localized, UI-ready label for the current lobby phase. */
	UFUNCTION(BlueprintPure, FieldNotify, Category = "Lobby Flow|View Model")
	FText GetPhaseDisplayText() const;

	int32 GetConnectedPlayerCount() const { return ConnectedPlayerCount; }
	int32 GetReadyPlayerCount() const { return ReadyPlayerCount; }
	int32 GetMinimumPlayers() const { return MinimumPlayers; }
	float GetCountdownDurationSeconds() const { return CountdownDurationSeconds; }
	double GetCountdownSecondsRemaining() const { return CountdownSecondsRemaining; }

	/** Returns the remaining countdown time formatted to milliseconds for direct UMG binding. */
	UFUNCTION(BlueprintPure, FieldNotify, Category = "Lobby Flow|View Model")
	FText GetCountdownDisplayText() const;

	bool IsCountdownActive() const { return bIsCountdownActive; }
	bool IsLocalPlayerReady() const { return bIsLocalPlayerReady; }
	bool CanChangeReadyState() const { return bCanChangeReadyState; }
	const TArray<TObjectPtr<ULobbyPlayerEntryViewModel>>& GetPlayers() const { return Players; }

private:
	UFUNCTION()
	void HandleLobbyStateChanged(const FLobbyRuntimeState& NewState);

	UFUNCTION()
	void HandleLobbyPlayersChanged();

	UFUNCTION()
	void HandleLocalPlayerReadyStateChanged(ALobbyPlayerState* InPlayerState, bool bNewReady);

	void ApplyLobbyState(const FLobbyRuntimeState& NewState);
	void RefreshPlayers();
	void BindToLocalPlayerState(ALobbyPlayerState* NewLocalPlayerState);
	void UnbindFromLocalPlayerState();
	void SetPlayers(TArray<TObjectPtr<ULobbyPlayerEntryViewModel>>&& NewPlayers);
	void UpdateCanChangeReadyState();
	void SetCountdownSecondsRemaining(double NewSecondsRemaining);
	void RefreshCountdownTimer();
	void UpdateCountdown();
	void ClearCountdownTimer();
	void ReleaseBindings();
	void ResetViewState();

	TWeakObjectPtr<ALobbyPlayerController> PlayerController;
	TWeakObjectPtr<ALobbyGameState> LobbyGameState;
	TWeakObjectPtr<ALobbyPlayerState> BoundLocalPlayerState;
	FTimerHandle CountdownTimerHandle;
	double CountdownEndServerTime = 0.0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Lobby Flow|View Model", meta = (AllowPrivateAccess = "true"))
	ELobbyPhase Phase = ELobbyPhase::WaitingForPlayers;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Lobby Flow|View Model", meta = (AllowPrivateAccess = "true"))
	int32 ConnectedPlayerCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Lobby Flow|View Model", meta = (AllowPrivateAccess = "true"))
	int32 ReadyPlayerCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Lobby Flow|View Model", meta = (AllowPrivateAccess = "true"))
	int32 MinimumPlayers = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Lobby Flow|View Model", meta = (AllowPrivateAccess = "true"))
	float CountdownDurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Lobby Flow|View Model", meta = (AllowPrivateAccess = "true"))
	double CountdownSecondsRemaining = 0.0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Lobby Flow|View Model", meta = (AllowPrivateAccess = "true"))
	bool bIsCountdownActive = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Lobby Flow|View Model", meta = (AllowPrivateAccess = "true"))
	bool bIsLocalPlayerReady = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Lobby Flow|View Model", meta = (AllowPrivateAccess = "true"))
	bool bCanChangeReadyState = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Lobby Flow|View Model", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<ULobbyPlayerEntryViewModel>> Players;
};
