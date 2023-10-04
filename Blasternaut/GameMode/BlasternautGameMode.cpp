// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasternautGameMode.h"

#include "Blasternaut/Character/BlasternautCharacter.h"
#include "Blasternaut/GameState/BlasternautGameState.h"
#include "Blasternaut/PlayerController/BlasternautController.h"
#include "Blasternaut/PlayerState/BlasternautPlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasternautGameMode::ABlasternautGameMode()
{
	bDelayedStart = true;
}

void ABlasternautGameMode::BeginPlay()
{
	Super::BeginPlay();
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasternautGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = LevelStartingTime + WarmupTime - GetWorld()->GetTimeSeconds();

		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds();

		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void ABlasternautGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasternautController* BlasternautPlayer = Cast < ABlasternautController>(*It);
		if (BlasternautPlayer)
		{
			BlasternautPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void ABlasternautGameMode::PlayerEliminated(ABlasternautCharacter* CharacterToEliminate, ABlasternautController* VictimController, ABlasternautController* AttackerController)
{
	auto* AttackerPlayerState = AttackerController ? Cast<ABlasternautPlayerState>(AttackerController->PlayerState) : nullptr;
	auto* VictimPlayerState = VictimController ? Cast<ABlasternautPlayerState>(VictimController->PlayerState) : nullptr;

	auto* BlasternautGameState = GetGameState<ABlasternautGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasternautGameState)
	{
		AttackerPlayerState->AddToScore(1.f);
		BlasternautGameState->UpdateTopScore(AttackerPlayerState);
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (CharacterToEliminate)
	{
		CharacterToEliminate->Elim();
	}
}

void ABlasternautGameMode::RequestRespawn(ACharacter* CharacterToEliminate, AController* ElimmedController)
{
	if (CharacterToEliminate) 
	{
		CharacterToEliminate->Reset();
		CharacterToEliminate->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}
