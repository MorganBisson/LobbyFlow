// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"

#include "LobbyPlayerEntryViewModel.generated.h"

class ALobbyPlayerState;

/** Event-driven UI projection of one replicated lobby player. */
UCLASS(BlueprintType, Transient)
class LOBBYFLOW_API ULobbyPlayerEntryViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;

	/** Binds this entry to one replicated lobby player. */
	bool Initialize(ALobbyPlayerState* InPlayerState, bool bInIsLocalPlayer);

	/** Releases the delegate bindings held on the replicated PlayerState. */
	void Deinitialize();

	ALobbyPlayerState* GetPlayerState() const
	{
		return PlayerState.Get();
	}

	const FText& GetDisplayName() const { return DisplayName; }
	bool IsReady() const { return bIsReady; }
	bool IsLocalPlayer() const { return bIsLocalPlayer; }

	/** Returns a localized, UI-ready label for the current ready state. */
	UFUNCTION(BlueprintPure, FieldNotify, Category = "Lobby Flow|View Model|Player")
	FText GetReadyStatusText() const;

private:
	UFUNCTION()
	void HandleReadyStateChanged(ALobbyPlayerState* InPlayerState, bool bNewReady);

	UFUNCTION()
	void HandlePlayerNameChanged(ALobbyPlayerState* InPlayerState, const FString& NewPlayerName);

	void SetDisplayName(const FString& NewPlayerName);
	void SetReady(bool bNewReady);
	void SetIsLocalPlayer(bool bNewIsLocalPlayer);

	TWeakObjectPtr<ALobbyPlayerState> PlayerState;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Lobby Flow|View Model|Player", meta = (AllowPrivateAccess = "true"))
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Lobby Flow|View Model|Player", meta = (AllowPrivateAccess = "true"))
	bool bIsReady = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Lobby Flow|View Model|Player", meta = (AllowPrivateAccess = "true"))
	bool bIsLocalPlayer = false;
};
