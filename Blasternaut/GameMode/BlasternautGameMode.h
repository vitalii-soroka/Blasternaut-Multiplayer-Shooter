// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasternautGameMode.generated.h"

class ABlasternautCharacter;
class ABlasternautController;
/**
 * 
 */
UCLASS()
class BLASTERNAUT_API ABlasternautGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	virtual void PlayerEliminated(
		ABlasternautCharacter* CharacterToEliminate, 
		ABlasternautController* VictimController,
		ABlasternautController* AttackerController
	);

	virtual void RequestRespawn(
		ACharacter* CharacterToEliminate,
		AController* ElimmedController
	);
};
