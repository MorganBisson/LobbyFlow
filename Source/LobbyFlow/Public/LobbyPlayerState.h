// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "LobbyPlayerState.generated.h"

class ALobbyPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnLobbyPlayerReadyStateChanged,
	ALobbyPlayerState*,
	PlayerState,
	bool,
	bIsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnLobbyPlayerNameChanged,
	ALobbyPlayerState*,
	PlayerState,
	const FString&,
	PlayerName);

/** Replicated per-player state used while a player is in the lobby. */
UCLASS()
class LOBBYFLOW_API ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_PlayerName() override;

	UFUNCTION(BlueprintPure, Category = "Lobby Flow|Player")
	bool IsReady() const
	{
		return bIsReady;
	}

	UPROPERTY(BlueprintAssignable, Category = "Lobby Flow|Player")
	FOnLobbyPlayerReadyStateChanged OnReadyStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lobby Flow|Player")
	FOnLobbyPlayerNameChanged OnPlayerNameChanged;

	/** Authority-only mutation used by the lobby game mode. */
	bool SetReadyState(bool bNewReady);

private:
	UFUNCTION()
	void OnRep_IsReady();

	UPROPERTY(ReplicatedUsing = OnRep_IsReady)
	bool bIsReady = false;
};
