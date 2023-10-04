// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasternautGameState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTERNAUT_API ABlasternautGameState : public AGameState
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void UpdateTopScore(class ABlasternautPlayerState* ScoringPlayer);

	UPROPERTY(Replicated)
	TArray<ABlasternautPlayerState*> TopScoringPlayers;
	
private:
	float TopScore = 0.f;
};
