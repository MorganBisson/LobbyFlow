// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyPlayerState.h"

#include "Net/UnrealNetwork.h"

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyPlayerState, bIsReady);
}

bool ALobbyPlayerState::SetReadyState(const bool bNewReady)
{
	if (!HasAuthority() || bIsReady == bNewReady)
	{
		return false;
	}

	bIsReady = bNewReady;
	ForceNetUpdate();
	OnReadyStateChanged.Broadcast(this, bIsReady);
	return true;
}

void ALobbyPlayerState::OnRep_IsReady()
{
	OnReadyStateChanged.Broadcast(this, bIsReady);
}

void ALobbyPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();
	OnPlayerNameChanged.Broadcast(this, GetPlayerName());
}
