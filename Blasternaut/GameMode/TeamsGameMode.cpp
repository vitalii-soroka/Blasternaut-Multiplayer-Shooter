// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"
#include "Blasternaut/GameState/BlasternautGameState.h"
#include "Blasternaut/PlayerState/BlasternautPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Blasternaut/PlayerController/BlasternautController.h"

ATeamsGameMode::ATeamsGameMode()
{
	bTeamsMatch = true;
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	auto* BGameState = Cast<ABlasternautGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		auto* BPState = NewPlayer->GetPlayerState<ABlasternautPlayerState>();
		AddPlayerToTeam(BPState, BGameState);
	}
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	auto* BGameState = Cast<ABlasternautGameState>(UGameplayStatics::GetGameState(this));
	auto* BPState = Exiting->GetPlayerState<ABlasternautPlayerState>();
	if (BGameState && BPState)
	{
		if (BGameState->RedTeam.Contains(BPState)) BGameState->RedTeam.Remove(BPState);
		else if (BGameState->BlueTeam.Contains(BPState)) BGameState->BlueTeam.Remove(BPState);
	}
	Super::Logout(Exiting);
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	auto* BGameState = Cast<ABlasternautGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		for (auto PState : BGameState->PlayerArray)
		{
			auto* BPState = Cast<ABlasternautPlayerState>(PState.Get());
			if (BPState && BPState->GetTeam() == ETeam::ET_NoTeam)
			{
				if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
				{
					BGameState->RedTeam.AddUnique(BPState);
					BPState->SetTeam(ETeam::ET_RedTeam);
				}
				else
				{
					BGameState->BlueTeam.AddUnique(BPState);
					BPState->SetTeam(ETeam::ET_BlueTeam);
				}
			}
		}
	}
}

void ATeamsGameMode::AddPlayerToTeam(ABlasternautPlayerState* NewPlayerState, ABlasternautGameState* BGameState)
{
	if (NewPlayerState && NewPlayerState->GetTeam() == ETeam::ET_NoTeam && BGameState)
	{
		if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
		{
			BGameState->RedTeam.AddUnique(NewPlayerState);
			NewPlayerState->SetTeam(ETeam::ET_RedTeam);
		}
		else
		{
			BGameState->BlueTeam.AddUnique(NewPlayerState);
			NewPlayerState->SetTeam(ETeam::ET_BlueTeam);
		}
	}
}

float ATeamsGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	auto* AttackerPState = Attacker->GetPlayerState<ABlasternautPlayerState>();
	auto* VictimPState = Victim->GetPlayerState<ABlasternautPlayerState>();
	
	// 
	if (AttackerPState == nullptr || VictimPState == nullptr) return BaseDamage;
	//	self damage
	if (AttackerPState == VictimPState) return BaseDamage;		
	//
	if (AttackerPState->GetTeam() == VictimPState->GetTeam())
	{
		return 0.f;
	}
	return BaseDamage;
}

void ATeamsGameMode::PlayerEliminated(ABlasternautCharacter* CharacterToEliminate, ABlasternautController* VictimController, ABlasternautController* AttackerController)
{
	Super::PlayerEliminated(CharacterToEliminate, VictimController, AttackerController);

	auto* BGameState = Cast<ABlasternautGameState>(UGameplayStatics::GetGameState(this));

	auto* AttackerPlayerState = AttackerController ? Cast<ABlasternautPlayerState>(AttackerController->PlayerState) : nullptr;

	if (BGameState && AttackerPlayerState)
	{
		if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
		{
			BGameState->BlueTeamScores();
		}
		else if (AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam)
		{
			BGameState->RedTeamScores();
		}
	}
}