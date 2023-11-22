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

float ABlasternautGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	return BaseDamage;
}

void ABlasternautGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasternautController* BlasternautPlayer = Cast < ABlasternautController>(*It);
		if (BlasternautPlayer)
		{
			BlasternautPlayer->OnMatchStateSet(MatchState, bTeamsMatch);
		}
	}
}

void ABlasternautGameMode::PlayerEliminated(ABlasternautCharacter* CharacterToEliminate, ABlasternautController* VictimController, ABlasternautController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;

	auto* AttackerPlayerState = AttackerController ? Cast<ABlasternautPlayerState>(AttackerController->PlayerState) : nullptr;
	auto* VictimPlayerState = VictimController ? Cast<ABlasternautPlayerState>(VictimController->PlayerState) : nullptr;

	auto* BlasternautGameState = GetGameState<ABlasternautGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasternautGameState)
	{
		TArray<ABlasternautPlayerState*> PlayersCurrentlyLead;
		for (auto LeadPlayer : BlasternautGameState->TopScoringPlayers)
		{
			PlayersCurrentlyLead.Add(LeadPlayer);
		}

		AttackerPlayerState->AddToScore(1.f);
		BlasternautGameState->UpdateTopScore(AttackerPlayerState);

		// Gained Lead
		if (BlasternautGameState->TopScoringPlayers.Contains(AttackerPlayerState))
		{
			auto* Leader = Cast<ABlasternautCharacter>(AttackerPlayerState->GetPawn());
			if (Leader)
			{
				Leader->Multicast_GainedTheLead();
			}
		}

		// Losted Lead
		for (int32 i = 0; i < PlayersCurrentlyLead.Num(); ++i)
		{
			if (!BlasternautGameState->TopScoringPlayers.Contains(PlayersCurrentlyLead[i]))
			{
				auto* Loser = Cast<ABlasternautCharacter>(PlayersCurrentlyLead[i]->GetPawn());
				if (Loser)
				{
					Loser->Multicast_LostTheLead();
				}
			}
		}
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (CharacterToEliminate)
	{
		CharacterToEliminate->Elim(false);
	}

	// Elim message
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		auto* BlasternautPlayer = Cast<ABlasternautController>(*It);
		if (BlasternautPlayer && AttackerPlayerState && VictimPlayerState)
		{
			BlasternautPlayer->BroadCastElim(AttackerPlayerState, VictimPlayerState);
		}
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

void ABlasternautGameMode::PlayerLeftGame(ABlasternautPlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr) return;

	auto* BlasternautGameState = GetGameState<ABlasternautGameState>();
	
	if (BlasternautGameState && BlasternautGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		BlasternautGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}
	auto* CharacterLeaving = Cast<ABlasternautCharacter>(PlayerLeaving->GetPawn());

	if (CharacterLeaving)
	{
		CharacterLeaving->Elim(true);
	}
}
