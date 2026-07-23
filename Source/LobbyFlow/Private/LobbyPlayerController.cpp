// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyPlayerController.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "LobbyFlow.h"
#include "LobbyGameMode.h"
#include "LobbyPlayerState.h"
#include "LobbyViewModel.h"
#include "MVVMGameSubsystem.h"
#include "Types/MVVMViewModelContext.h"

namespace
{
	const FName LobbyViewModelContextName(TEXT("Lobby"));

	FMVVMViewModelContext MakeLobbyViewModelContext()
	{
		FMVVMViewModelContext Context;
		Context.ContextClass = ULobbyViewModel::StaticClass();
		Context.ContextName = LobbyViewModelContextName;
		return Context;
	}
}

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	EnsureLobbyViewModel();
}

void ALobbyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterLobbyViewModel();

	if (LobbyViewModel)
	{
		LobbyViewModel->Deinitialize();
		LobbyViewModel = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ALobbyPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	EnsureLobbyViewModel();
}

void ALobbyPlayerController::SetReady(const bool bNewReady)
{
	if (HasAuthority())
	{
		ApplyReadyRequest(bNewReady);
	}
	else if (IsLocalController())
	{
		ServerSetReady(bNewReady);
	}
}

void ALobbyPlayerController::ToggleReady()
{
	if (const ALobbyPlayerState* LobbyPlayerState = GetLobbyPlayerState())
	{
		SetReady(!LobbyPlayerState->IsReady());
	}
}

ALobbyPlayerState* ALobbyPlayerController::GetLobbyPlayerState() const
{
	return GetPlayerState<ALobbyPlayerState>();
}

void ALobbyPlayerController::EnsureLobbyViewModel()
{
	if (!IsLocalController())
	{
		return;
	}

	if (!LobbyViewModel)
	{
		LobbyViewModel = NewObject<ULobbyViewModel>(this);
	}

	if (LobbyViewModel->Initialize(this))
	{
		RegisterLobbyViewModel();
	}
	else
	{
		UnregisterLobbyViewModel();
	}
}

void ALobbyPlayerController::RegisterLobbyViewModel()
{
	if (!LobbyViewModel)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UMVVMGameSubsystem* ViewModelSubsystem = GameInstance
		? GameInstance->GetSubsystem<UMVVMGameSubsystem>()
		: nullptr;
	UMVVMViewModelCollectionObject* ViewModelCollection = ViewModelSubsystem
		? ViewModelSubsystem->GetViewModelCollection()
		: nullptr;
	if (!ViewModelCollection)
	{
		UE_LOG(LogLobbyFlow, Warning, TEXT("Unable to register Lobby ViewModel: global MVVM collection is unavailable."));
		return;
	}

	const FMVVMViewModelContext Context = MakeLobbyViewModelContext();
	UMVVMViewModelBase* RegisteredViewModel = ViewModelCollection->FindViewModelInstance(Context);
	if (RegisteredViewModel == LobbyViewModel.Get())
	{
		return;
	}

	if (RegisteredViewModel)
	{
		UE_LOG(LogLobbyFlow, Warning, TEXT("Replacing an existing Lobby ViewModel in the global MVVM collection."));
		ViewModelCollection->RemoveViewModel(Context);
	}

	if (!ViewModelCollection->AddViewModelInstance(Context, LobbyViewModel.Get()))
	{
		UE_LOG(LogLobbyFlow, Error, TEXT("Unable to register Lobby ViewModel in the global MVVM collection."));
	}
}

void ALobbyPlayerController::UnregisterLobbyViewModel()
{
	if (!LobbyViewModel)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UMVVMGameSubsystem* ViewModelSubsystem = GameInstance
		? GameInstance->GetSubsystem<UMVVMGameSubsystem>()
		: nullptr;
	UMVVMViewModelCollectionObject* ViewModelCollection = ViewModelSubsystem
		? ViewModelSubsystem->GetViewModelCollection()
		: nullptr;
	if (ViewModelCollection)
	{
		ViewModelCollection->RemoveAllViewModelInstance(LobbyViewModel.Get());
	}
}

void ALobbyPlayerController::ApplyReadyRequest(const bool bNewReady)
{
	UWorld* World = GetWorld();
	ALobbyGameMode* LobbyGameMode = World ? World->GetAuthGameMode<ALobbyGameMode>() : nullptr;
	if (LobbyGameMode)
	{
		LobbyGameMode->SetPlayerReady(this, bNewReady);
	}
}

void ALobbyPlayerController::ServerSetReady_Implementation(const bool bNewReady)
{
	ApplyReadyRequest(bNewReady);
}
