// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasternautPlayerState.h"

#include "Blasternaut/Character/BlasternautCharacter.h"
#include "Blasternaut/PlayerController/BlasternautController.h"
#include "Net/UnrealNetwork.h"

void ABlasternautPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasternautPlayerState, Defeats);
}

void ABlasternautPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	
	if (TrySetController())
	{
		Controller->SetHUDScore(GetScore());
	}
}

void ABlasternautPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	if (TrySetController()) 
	{
		Controller->SetHUDScore(GetScore());
	}
}

void ABlasternautPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;

	if (TrySetController())
	{
		Controller->SetHUDDefeats(Defeats);
	}
}

void ABlasternautPlayerState::OnRep_Defeats()
{
	if (TrySetController())
	{
		Controller->SetHUDDefeats(Defeats);
	}
}

bool ABlasternautPlayerState::TrySetController()
{
	Character = Character == nullptr ? Cast<ABlasternautCharacter>(GetPawn()) : Character;

	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasternautController>(Character->Controller) : Controller;

		return Controller != nullptr; 
	}
	return false;
}
