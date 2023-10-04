// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasternautController.h"

#include "Blasternaut/Character/BlasternautCharacter.h"
#include "Blasternaut/CharacterComponents/CombatComponent.h"
#include "Blasternaut/GameMode/BlasternautGameMode.h"
#include "Blasternaut/HUD/Announcement.h"
#include "Blasternaut/HUD/BlasternautHUD.h"
#include "Blasternaut/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Blasternaut/GameState/BlasternautGameState.h"
#include "Blasternaut/PlayerState/BlasternautPlayerState.h"

void ABlasternautController::BeginPlay()
{
	Super::BeginPlay();

	BlasternautHUD = Cast<ABlasternautHUD>(GetHUD());
	ServerCheckMatchState();
}

void ABlasternautController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasternautController, MatchState);
}

void ABlasternautController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	auto* BlasternautCharacter = Cast<ABlasternautCharacter>(InPawn);
	if (BlasternautCharacter)
	{
		SetHUDHealth(BlasternautCharacter->GetHealth(), BlasternautCharacter->GetMaxHealth());
	}
}

void ABlasternautController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasternautController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
}

// -------------- Server Time Sync --------------
float ABlasternautController::GetServerTime()
{
	//return GetWorld()->GetTimeSeconds() + (HasAuthority() ? 0 : ClientServerDelta);
	
	if (HasAuthority())
		return GetWorld()->GetTimeSeconds();
	else
		return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasternautController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;

	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasternautController::ServerCheckMatchState_Implementation()
{
	auto* GameMode = Cast<ABlasternautGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		MatchState = GameMode->GetMatchState();
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void ABlasternautController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	MatchState = StateOfMatch;
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;

	OnMatchStateSet(MatchState);

	if (BlasternautHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasternautHUD->AddAnnouncement();
	}
}

void ABlasternautController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasternautController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

// -------------- Match State --------------
void ABlasternautController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasternautController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasternautController::HandleMatchHasStarted()
{
	if (TrySetHUD())
	{
		if (BlasternautHUD->CharacterOverlay == nullptr) BlasternautHUD->AddCharacterOverlay();

		if (BlasternautHUD->Announcement)
		{
			BlasternautHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ABlasternautController::HandleCooldown()
{
	if (TrySetHUD())
	{
		BlasternautHUD->CharacterOverlay->RemoveFromParent();

		if (BlasternautHUD->Announcement && 
			BlasternautHUD->Announcement->AnnouncementText &&
			BlasternautHUD->Announcement->InfoText)
		{
			BlasternautHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Starts In:");
			BlasternautHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
			
			auto* BlasternautGameState = Cast<ABlasternautGameState>(UGameplayStatics::GetGameState(this));
			auto* BlasternautPlayerState = GetPlayerState<ABlasternautPlayerState>();

			if (BlasternautGameState && BlasternautPlayerState)
			{ 
				auto TopPlayers = BlasternautGameState->TopScoringPlayers;
				FString InfoTextString;

				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("There is no winner");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasternautPlayerState)
				{
					InfoTextString = FString("You are the winner");
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Players tied for the win:\n");
					for (auto TiedPlayers : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayers->GetPlayerName()));
					}
				}

				BlasternautHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}

	auto* BlasternautCharacter = Cast<ABlasternautCharacter>(GetPawn());
	if (BlasternautCharacter && BlasternautCharacter->GetCombat())
	{
		BlasternautCharacter->bDisableGameplay = true;
		BlasternautCharacter->GetCombat()->FireButtonPressed(false);
	}
}

// -------------- Set HUD Elements --------------
bool ABlasternautController::TrySetHUD()
{
	BlasternautHUD = BlasternautHUD == nullptr ? Cast<ABlasternautHUD>(GetHUD()) : BlasternautHUD;
	return BlasternautHUD != nullptr;
}

void ABlasternautController::SetHUDHealth(float Health, float MaxHealth)
{
	TrySetHUD();

	if (IsOverlayValid() &&
		BlasternautHUD->CharacterOverlay->HealthBar &&
		BlasternautHUD->CharacterOverlay->HealthText)
	{
		const float HealthPercent = Health / MaxHealth;
		BlasternautHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		
		 FString HealthText = FString::Printf(TEXT("%d/%d"), 
			FMath::CeilToInt(Health),FMath::CeilToInt(MaxHealth));

		BlasternautHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasternautController::SetHUDScore(float Score)
{
	TrySetHUD();

	if (IsOverlayValid() && BlasternautHUD->CharacterOverlay->ScoreAmount)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasternautHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDScore = Score;
	}
}

void ABlasternautController::SetHUDDefeats(int32 Defeats)
{
	TrySetHUD();

	if (IsOverlayValid() && BlasternautHUD->CharacterOverlay->DefeatsAmount)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasternautHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void ABlasternautController::SetHUDWeaponAmmo(int32 Ammo)
{
	TrySetHUD();

	if (IsOverlayValid() && BlasternautHUD->CharacterOverlay->WeaponAmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasternautHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ABlasternautController::SetHUDCarriedAmmo(int32 Ammo)
{
	TrySetHUD();

	if (IsOverlayValid() && BlasternautHUD->CharacterOverlay->CarriedAmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasternautHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ABlasternautController::SetHUDMatchCountdown(float CountdownTime)
{
	TrySetHUD();

	if (IsOverlayValid() && BlasternautHUD->CharacterOverlay->MatchCountdownText)
	{
		if (CountdownTime < 0.f)
		{
			BlasternautHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f );
		int32 Seconds = CountdownTime - 60 * Minutes;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasternautHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABlasternautController::SetHUDAnnouncementCountDown(float CountdownTime)
{
	if (TrySetHUD() && BlasternautHUD->Announcement && BlasternautHUD->Announcement->WarmupTime)
	{
		if (CountdownTime < 0.f)
		{
			BlasternautHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - 60 * Minutes;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasternautHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ABlasternautController::SetHUDTime()
{
	float TimeLeft = 0.f;

	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	//
	if (HasAuthority())
	{
		BlasternautGameMode = BlasternautGameMode == nullptr ? Cast<ABlasternautGameMode>(UGameplayStatics::GetGameMode(this)) : BlasternautGameMode;
		if (BlasternautGameMode)
		{
			SecondsLeft = FMath::CeilToInt(BlasternautGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}
	
	// Update HUD Timer every second
	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountDown(TimeLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
		else if (MatchState == MatchState::Cooldown)
		{

		}
	}

	CountdownInt = SecondsLeft;
}

void ABlasternautController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (BlasternautHUD && BlasternautHUD->CharacterOverlay)
		{
			CharacterOverlay = BlasternautHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}
}

// --------------  --------------
bool ABlasternautController::IsOverlayValid() const
{
	return BlasternautHUD && BlasternautHUD->CharacterOverlay;
}

