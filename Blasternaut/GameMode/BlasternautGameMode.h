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
	ABlasternautGameMode();

	virtual void Tick(float DeltaTime) override;

	virtual void PlayerEliminated(
		ABlasternautCharacter* CharacterToEliminate, 
		ABlasternautController* VictimController,
		ABlasternautController* AttackerController
	);

	virtual void RequestRespawn(
		ACharacter* CharacterToEliminate,
		AController* ElimmedController
	);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	float LevelStartingTime = 0.f;

protected:
	virtual void BeginPlay() override;

	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0.f;
};
