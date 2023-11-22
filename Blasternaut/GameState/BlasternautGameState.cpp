// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasternautGameState.h"
#include "Net/UnrealNetwork.h"
#include "Blasternaut/PlayerState/BlasternautPlayerState.h"
#include "Blasternaut/PlayerController/BlasternautController.h"

void ABlasternautGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasternautGameState, TopScoringPlayers);
	DOREPLIFETIME(ABlasternautGameState, RedTeamScore);
	DOREPLIFETIME(ABlasternautGameState, BlueTeamScore);
}

void ABlasternautGameState::UpdateTopScore(ABlasternautPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() == TopScore)
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void ABlasternautGameState::OnRep_RedTeamScore()
{
	auto* World = GetWorld();
	if (World)
	{
		auto* BController = Cast<ABlasternautController>(World->GetFirstPlayerController());
		if (BController)
		{
			BController->SetHUDRedTeamScore(RedTeamScore);
		}
	}
}

void ABlasternautGameState::OnRep_BlueTeamScore()
{
	auto* World = GetWorld();
	if (World)
	{
		auto* BController = Cast<ABlasternautController>(World->GetFirstPlayerController());
		if (BController)
		{
			BController->SetHUDBlueTeamScore(BlueTeamScore);
		}
	}
}

void ABlasternautGameState::RedTeamScores()
{
	++RedTeamScore;

	auto* World = GetWorld();
	if (World)
	{
		auto* BController = Cast<ABlasternautController>(World->GetFirstPlayerController());
		if (BController)
		{
			BController->SetHUDRedTeamScore(RedTeamScore);
		}
	}
}

void ABlasternautGameState::BlueTeamScores()
{
	++BlueTeamScore;

	auto* World = GetWorld();
	if (World)
	{
		auto* BController = Cast<ABlasternautController>(World->GetFirstPlayerController());
		if (BController)
		{
			BController->SetHUDBlueTeamScore(BlueTeamScore);
		}
	}
}
