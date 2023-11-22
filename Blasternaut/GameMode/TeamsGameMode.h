// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasternautGameMode.h"
#include "TeamsGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTERNAUT_API ATeamsGameMode : public ABlasternautGameMode
{
	GENERATED_BODY()

public:
	ATeamsGameMode();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) override;
	virtual void PlayerEliminated(
		ABlasternautCharacter* CharacterToEliminate,
		ABlasternautController* VictimController,
		ABlasternautController* AttackerController
	) override;

protected:
	
	virtual void HandleMatchHasStarted() override;

private:

	void AddPlayerToTeam(class ABlasternautPlayerState* NewPlayerState, class ABlasternautGameState* BGameState);

};
