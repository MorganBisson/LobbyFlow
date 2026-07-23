// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "LobbyPlayerController.generated.h"

class ALobbyPlayerState;
class ULobbyViewModel;

/** Client-owned entry point for changing the local player's ready state. */
UCLASS()
class LOBBYFLOW_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Lobby Flow|Player")
	void SetReady(bool bNewReady);

	UFUNCTION(BlueprintCallable, Category = "Lobby Flow|Player")
	void ToggleReady();

	UFUNCTION(BlueprintPure, Category = "Lobby Flow|Player")
	ALobbyPlayerState* GetLobbyPlayerState() const;

	/** Local ViewModel used as the UMG MVVM source for this lobby. */
	UFUNCTION(BlueprintPure, Category = "Lobby Flow|View Model")
	ULobbyViewModel* GetLobbyViewModel() const
	{
		return LobbyViewModel;
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_PlayerState() override;

private:
	void EnsureLobbyViewModel();
	void RegisterLobbyViewModel();
	void UnregisterLobbyViewModel();
	void ApplyReadyRequest(bool bNewReady);

	UFUNCTION(Server, Reliable)
	void ServerSetReady(bool bNewReady);

	UPROPERTY(Transient)
	TObjectPtr<ULobbyViewModel> LobbyViewModel;
};
