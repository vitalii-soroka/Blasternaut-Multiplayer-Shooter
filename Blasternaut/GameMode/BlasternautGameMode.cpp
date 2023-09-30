// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasternautGameMode.h"
#include "Blasternaut/Character/BlasternautCharacter.h"
#include "Blasternaut/PlayerController/BlasternautController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Blasternaut/PlayerState/BlasternautPlayerState.h"

void ABlasternautGameMode::PlayerEliminated(ABlasternautCharacter* CharacterToEliminate, ABlasternautController* VictimController, ABlasternautController* AttackerController)
{
	auto* AttackerPlayerState = AttackerController ? Cast<ABlasternautPlayerState>(AttackerController->PlayerState) : nullptr;
	auto* VictimPlayerState = VictimController ? Cast<ABlasternautPlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
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
