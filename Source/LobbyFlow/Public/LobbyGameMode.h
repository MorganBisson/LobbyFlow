// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"

#include "LobbyGameMode.generated.h"

class ALobbyPlayerController;
class ALobbyPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyMatchStartRequested);

/** Server-authoritative lobby rules, countdown and match-start request. */
UCLASS()
class LOBBYFLOW_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ALobbyGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** Applies a ready request for the requesting controller's own player state. */
	void SetPlayerReady(ALobbyPlayerController* PlayerController, bool bNewReady);

	/** Fired on the server after the countdown completes and the start conditions are revalidated. */
	UPROPERTY(BlueprintAssignable, Category = "Lobby Flow|Events")
	FOnLobbyMatchStartRequested OnMatchStartRequested;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby Flow|Rules", meta = (ClampMin = "1"))
	int32 MinimumPlayers = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby Flow|Rules", meta = (ClampMin = "0.0"))
	float CountdownDurationSeconds = 5.0f;

private:
	void RefreshLobbyState(const ALobbyPlayerState* IgnoredPlayerState = nullptr);
	void CountLobbyPlayers(
		const ALobbyPlayerState* IgnoredPlayerState,
		int32& OutConnectedPlayerCount,
		int32& OutReadyPlayerCount) const;
	bool AreStartConditionsMet(int32 ConnectedPlayerCount, int32 ReadyPlayerCount) const;
	void StartCountdown();
	void CancelCountdown();
	void HandleCountdownFinished();

	FTimerHandle CountdownTimerHandle;
};
