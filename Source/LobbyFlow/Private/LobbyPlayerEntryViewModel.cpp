// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyPlayerEntryViewModel.h"

#include "Internationalization/Text.h"
#include "LobbyPlayerState.h"

void ULobbyPlayerEntryViewModel::BeginDestroy()
{
	Deinitialize();
	Super::BeginDestroy();
}

bool ULobbyPlayerEntryViewModel::Initialize(
	ALobbyPlayerState* InPlayerState,
	const bool bInIsLocalPlayer)
{
	if (!IsValid(InPlayerState))
	{
		Deinitialize();
		return false;
	}

	if (PlayerState.Get() != InPlayerState)
	{
		Deinitialize();
		PlayerState = InPlayerState;
		InPlayerState->OnReadyStateChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleReadyStateChanged);
		InPlayerState->OnPlayerNameChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandlePlayerNameChanged);
	}

	SetDisplayName(InPlayerState->GetPlayerName());
	SetReady(InPlayerState->IsReady());
	SetIsLocalPlayer(bInIsLocalPlayer);
	return true;
}

void ULobbyPlayerEntryViewModel::Deinitialize()
{
	if (ALobbyPlayerState* BoundPlayerState = PlayerState.Get())
	{
		BoundPlayerState->OnReadyStateChanged.RemoveDynamic(
			this,
			&ThisClass::HandleReadyStateChanged);
		BoundPlayerState->OnPlayerNameChanged.RemoveDynamic(
			this,
			&ThisClass::HandlePlayerNameChanged);
	}

	PlayerState.Reset();
}

FText ULobbyPlayerEntryViewModel::GetReadyStatusText() const
{
	return bIsReady
		? NSLOCTEXT("LobbyFlow", "LobbyPlayerReady", "Ready")
		: NSLOCTEXT("LobbyFlow", "LobbyPlayerNotReady", "Not Ready");
}

void ULobbyPlayerEntryViewModel::HandleReadyStateChanged(
	ALobbyPlayerState* InPlayerState,
	const bool bNewReady)
{
	if (InPlayerState == PlayerState.Get())
	{
		SetReady(bNewReady);
	}
}

void ULobbyPlayerEntryViewModel::HandlePlayerNameChanged(
	ALobbyPlayerState* InPlayerState,
	const FString& NewPlayerName)
{
	if (InPlayerState == PlayerState.Get())
	{
		SetDisplayName(NewPlayerName);
	}
}

void ULobbyPlayerEntryViewModel::SetDisplayName(const FString& NewPlayerName)
{
	const FText NewDisplayName = FText::AsCultureInvariant(NewPlayerName);
	if (DisplayName.IdenticalTo(NewDisplayName))
	{
		return;
	}

	DisplayName = NewDisplayName;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DisplayName);
}

void ULobbyPlayerEntryViewModel::SetReady(const bool bNewReady)
{
	if (bIsReady == bNewReady)
	{
		return;
	}

	bIsReady = bNewReady;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bIsReady);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetReadyStatusText);
}

void ULobbyPlayerEntryViewModel::SetIsLocalPlayer(const bool bNewIsLocalPlayer)
{
	UE_MVVM_SET_PROPERTY_VALUE(bIsLocalPlayer, bNewIsLocalPlayer);
}
