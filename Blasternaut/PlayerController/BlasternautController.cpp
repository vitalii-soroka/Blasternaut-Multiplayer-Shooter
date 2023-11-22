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
#include "Components/Image.h"
#include "Blasternaut/HUD/ReturnToMainMenu.h"
#include "Blasternaut/BlasternautTypes/Announcement.h"

void ABlasternautController::BeginPlay()
{
	Super::BeginPlay();

	BlasternautHUD = Cast<ABlasternautHUD>(GetHUD());
	ServerCheckMatchState();
}

void ABlasternautController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindAction("Quit", IE_Pressed, this, &ABlasternautController::ShowReturnToMainMenu);
	}
}

void ABlasternautController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasternautController, MatchState);
	DOREPLIFETIME(ABlasternautController, bShowTeamScores);
}

void ABlasternautController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	auto* BlasternautCharacter = Cast<ABlasternautCharacter>(InPawn);
	if (BlasternautCharacter)
	{
		SetHUDHealth(BlasternautCharacter->GetHealth(), BlasternautCharacter->GetMaxHealth());
		SetHUDShield(BlasternautCharacter->GetShield(), BlasternautCharacter->GetMaxShield());
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
	CheckHighPing(DeltaTime);
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
	SingleTripTime = (0.5f * RoundTripTime);
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

// -------------- Match State --------------
void ABlasternautController::OnMatchStateSet(FName State, bool bTeamsMatch)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted(bTeamsMatch);
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

void ABlasternautController::HandleMatchHasStarted(bool bTeamsMatch)
{
	// Team Score shows on server
	// clients rely on replication
	if (HasAuthority())
	{
		bShowTeamScores = bTeamsMatch;
		if (bTeamsMatch) InitTeamScores();
		else HideTeamScores();
	}
	
	// Announcement and overlay
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
			
			BlasternautHUD->Announcement->AnnouncementText->SetText(FText::FromString(Announcement::NewMatchStartsIn));
			
			auto* BlasternautGameState = Cast<ABlasternautGameState>(UGameplayStatics::GetGameState(this));
			auto* BlasternautPlayerState = GetPlayerState<ABlasternautPlayerState>();

			if (BlasternautGameState && BlasternautPlayerState)
			{ 
				auto TopPlayers = BlasternautGameState->TopScoringPlayers;
				FString InfoTextString = bShowTeamScores ? GetTeamsInfoText(BlasternautGameState) : GetInfoText(TopPlayers);

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

FString ABlasternautController::GetInfoText(const TArray<class ABlasternautPlayerState*>& Players) const
{
	FString InfoTextString;

	auto* BlasternautPlayerState = GetPlayerState<ABlasternautPlayerState>();
	if (BlasternautPlayerState == nullptr) return InfoTextString;

	if (Players.Num() == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (Players.Num() == 1 && Players[0] == BlasternautPlayerState)
	{
		InfoTextString = Announcement::YouAreTheWinner;
	}
	else if (Players.Num() == 1)
	{
		InfoTextString = Announcement::Winner;
		InfoTextString.Append(FString::Printf(TEXT("\n%s"), *Players[0]->GetPlayerName()));
	}
	else if (Players.Num() > 1)
	{
		InfoTextString = Announcement::PlayersTiedForTheWin;

		for (auto TiedPlayers : Players)
		{
			InfoTextString.Append(FString::Printf(TEXT("\n%s\n"), *TiedPlayers->GetPlayerName()));
		}
	}

	return InfoTextString;
}

FString ABlasternautController::GetTeamsInfoText(ABlasternautGameState* BlasternautGameState) const
{
	// TODO : Add top player

	FString InfoTextString;
	if (BlasternautGameState == nullptr) return InfoTextString;

	const int32 RedTeamScore = BlasternautGameState->RedTeamScore;
	const int32 BlueTeamScore = BlasternautGameState->BlueTeamScore;

	if (RedTeamScore == 0 && BlueTeamScore == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (RedTeamScore == BlueTeamScore)
	{
		InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::TeamsTiedForTheWin);
		InfoTextString.Append(Announcement::RedTeam);
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(Announcement::BlueTeam);
		InfoTextString.Append(TEXT("\n"));
	}
	else if (RedTeamScore > BlueTeamScore)
	{
		InfoTextString = Announcement::RedTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d"), *Announcement::RedTeam, RedTeamScore));
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d"), *Announcement::BlueTeam, BlueTeamScore));
	}
	else if (BlueTeamScore > RedTeamScore)
	{
		InfoTextString = Announcement::BlueTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d"), *Announcement::BlueTeam, BlueTeamScore));
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s: %d"), *Announcement::RedTeam, RedTeamScore));
	}

	return InfoTextString;
}

//
void ABlasternautController::StartHighPingWarning()
{
	TrySetHUD();

	if (IsOverlayValid() &&
		BlasternautHUD->CharacterOverlay->HighPingImage &&
		BlasternautHUD->CharacterOverlay->HighPingAnimation)
	{
		BlasternautHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		BlasternautHUD->CharacterOverlay->PlayAnimation(
			BlasternautHUD->CharacterOverlay->HighPingAnimation,
			0.f,
			5
		);
	}
}

void ABlasternautController::StopHighPingWarning()
{
	TrySetHUD();

	if (IsOverlayValid() &&
		BlasternautHUD->CharacterOverlay->HighPingImage &&
		BlasternautHUD->CharacterOverlay->HighPingAnimation)
	{
		BlasternautHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (BlasternautHUD->CharacterOverlay->IsAnimationPlaying(BlasternautHUD->CharacterOverlay->HighPingAnimation))
		{
			BlasternautHUD->CharacterOverlay->StopAnimation(BlasternautHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void ABlasternautController::CheckHighPing(float DeltaTime)
{
	if (HasAuthority()) return;

	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
		if (PlayerState)
		{
			//UE_LOG(LogTemp, Warning, TEXT("PlayerState->CompressedPing() * 4: %d"), PlayerState->GetCompressedPing() * 4);

			if (PlayerState->GetCompressedPing() * 4 > HighPingThreshold)
			{
				StartHighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}

	bool bPingWarningActive = IsOverlayValid() && 
		BlasternautHUD->CharacterOverlay->HighPingAnimation &&
		BlasternautHUD->CharacterOverlay->IsAnimationPlaying(BlasternautHUD->CharacterOverlay->HighPingAnimation);

	if (bPingWarningActive)
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void ABlasternautController::ShowReturnToMainMenu()
{
	// TODO show the Return to Main Menu Widget
	if (ReturnToMainMenuWidget == nullptr) return;

	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}

	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}
}

void ABlasternautController::BroadCastElim(APlayerState* Attacker, APlayerState* Victim)
{
	ClientElimAnnouncement(Attacker, Victim);
}

void ABlasternautController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	if (Attacker, Victim, Self)
	{
		if (TrySetHUD())
		{
			if (Attacker == Self && Victim != Self)
			{
				BlasternautHUD->AddElimAnnouncement("You", Victim->GetPlayerName());
			}
			else if (Victim == Self && Attacker != Self)
			{
				BlasternautHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "you");
			}
			else if (Attacker == Victim && Attacker == Self)
			{
				BlasternautHUD->AddElimAnnouncement("You", "yourself");
			}
			else if (Attacker == Victim && Attacker != Self)
			{
				BlasternautHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "themselves");
			}
			else
			{
				BlasternautHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
			}
		}
	}
}

// is ping too high?
void ABlasternautController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void ABlasternautController::OnRep_ShowTeamScores()
{
	if (bShowTeamScores)
	{
		InitTeamScores();
	}
	else
	{
		HideTeamScores();
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
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasternautController::SetHUDShield(float Shield, float MaxShield)
{
	TrySetHUD();

	if (IsOverlayValid() &&
		BlasternautHUD->CharacterOverlay->ShieldBar &&
		BlasternautHUD->CharacterOverlay->ShieldText)
	{
		const float ShieldPercent = Shield / MaxShield;
		BlasternautHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);

		FString ShieldText = FString::Printf(TEXT("%d/%d"),
			FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));

		BlasternautHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
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
		bInitializeScore = true;
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
		bInitializeDefeats = true;
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
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
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
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
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

void ABlasternautController::SetHUDGrenades(int32 Grenades)
{
	TrySetHUD();

	if (IsOverlayValid() && BlasternautHUD->CharacterOverlay->GrenadesText)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		BlasternautHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ABlasternautController::HideTeamScores()
{
	TrySetHUD();
	if (IsOverlayValid() &&
		BlasternautHUD->CharacterOverlay->RedTeamScore &&
		BlasternautHUD->CharacterOverlay->BlueTeamScore &&
		BlasternautHUD->CharacterOverlay->ScoreSpacerText)
	{
		BlasternautHUD->CharacterOverlay->RedTeamScore->SetText(FText());
		BlasternautHUD->CharacterOverlay->BlueTeamScore->SetText(FText());
		BlasternautHUD->CharacterOverlay->ScoreSpacerText->SetText(FText());
	}
}

void ABlasternautController::InitTeamScores()
{
	TrySetHUD();
	if (IsOverlayValid() &&
		BlasternautHUD->CharacterOverlay->RedTeamScore &&
		BlasternautHUD->CharacterOverlay->BlueTeamScore &&
		BlasternautHUD->CharacterOverlay->ScoreSpacerText)
	{
		FString Zero("0");
		FString Spacer("|");
		BlasternautHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(Zero));
		BlasternautHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(Zero));
		BlasternautHUD->CharacterOverlay->ScoreSpacerText->SetText(FText::FromString(Spacer));
	}
}

void ABlasternautController::SetHUDRedTeamScore(int32 RedScore)
{
	TrySetHUD();
	if (IsOverlayValid() &&
		BlasternautHUD->CharacterOverlay->RedTeamScore)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), RedScore);
		BlasternautHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(ScoreText));
	}
}

void ABlasternautController::SetHUDBlueTeamScore(int32 BlueScore)
{
	TrySetHUD();
	if (IsOverlayValid() &&
		BlasternautHUD->CharacterOverlay->BlueTeamScore)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), BlueScore);
		BlasternautHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(ScoreText));
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
	if (CharacterOverlay) return;

	if (BlasternautHUD && BlasternautHUD->CharacterOverlay)
	{
		CharacterOverlay = BlasternautHUD->CharacterOverlay;

		if (CharacterOverlay)
		{
			if (bInitializeHealth)  SetHUDHealth(HUDHealth, HUDMaxHealth);
			if (bInitializeShield)  SetHUDShield(HUDShield, HUDMaxShield);
			if (bInitializeScore)   SetHUDScore(HUDScore);
			if (bInitializeDefeats) SetHUDDefeats(HUDDefeats);
			if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
			if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);

			auto* BlasternautCharacter = Cast<ABlasternautCharacter>(GetPawn());
			if (bInitializeGrenades && BlasternautCharacter && BlasternautCharacter->GetCombat())
			{
				SetHUDGrenades(BlasternautCharacter->GetCombat()->GetGrenades());
			}
		}
	}
}

// --------------  --------------
bool ABlasternautController::IsOverlayValid() const
{
	return BlasternautHUD && BlasternautHUD->CharacterOverlay;
}

