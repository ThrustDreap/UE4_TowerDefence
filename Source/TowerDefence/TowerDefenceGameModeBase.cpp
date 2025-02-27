// Fill out your copyright notice in the Description page of Project Settings.


#include "TowerDefenceGameModeBase.h"

ATowerDefenceGameModeBase::ATowerDefenceGameModeBase() {
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;

	PlayerControllerClass = ATDPlayerController::StaticClass();
	DefaultPawnClass = ATDSpectatorPawn::StaticClass();
	HUDClass = ATDHUD::StaticClass();

	SpawnArray.Add(TArray<int32>{3});
	SpawnArray.Add(TArray<int32>{5});
	SpawnArray.Add(TArray<int32>{5, 1});
	SpawnArray.Add(TArray<int32>{7, 2});
	SpawnArray.Add(TArray<int32>{7, 3, 1});
	SpawnArray.Add(TArray<int32>{9, 4, 2});
	SpawnArray.Add(TArray<int32>{9, 5, 3});

	SpawnLocation = FVector(1300.0f, 900.0f, 280.0f);

	SpawnRotation = FRotator(0.0f, 0.0f, 0.0f);
	
	CurrentGold = 50;

	MaxWave = 7;
	CurrentWave = 0;

	WaveCurrentTimer = WaveMaxTimer = 7.0f;

	SpawnMaxTimer = 2.5f;
	SpawnCurrentTimer = 0.0f;

	bWaveUpdated = false;

	bCanSpawn = true;

	bGameFinished = false;
}

void ATowerDefenceGameModeBase::BeginPlay() {	
	HUD = Cast<ATDHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
	if (HUD) {
		HUD->UpdateGold(CurrentGold);
		HUD->UpdateWave(CurrentWave);
	}

	TArray<FVector> WayPointsSpawnLocations;
	WayPointsSpawnLocations.Add(FVector(-800.0f, 900.0f, 240.0f));
	WayPointsSpawnLocations.Add(FVector(-800.0f, -900.0f, 240.0f));
	WayPointsSpawnLocations.Add(FVector(-3650.0, -900.0f, 240.0f));
	WayPointsSpawnLocations.Add(FVector(-3650.0, 550.0f, 240.0f));
	WayPointsSpawnLocations.Add(FVector(-6000.0f, 550.0f, 240.0f));
	for (auto& Itr : WayPointsSpawnLocations) {
		ATDWayPoint* Point = GetWorld()->SpawnActor<ATDWayPoint>(Itr, FRotator(0.0f, 0.0f, 0.0f));
		if (Point) {
			if (WayPoints.Num()) {
				WayPoints.Last()->SetNextPoint(Point);
			}
			WayPoints.Add(Point);
		}
	}

	PlayerController = Cast<ATDPlayerController>(GEngine->GetFirstLocalPlayerController(GetWorld()));
}

void ATowerDefenceGameModeBase::SetGamePaused() {
	if (PlayerController) {
		PlayerController->SetPause(!IsPaused());
	}
}

bool ATowerDefenceGameModeBase::ChangeGold(const unsigned& Value, bool bMinus) {
	bool bResult = false;
	if (bMinus){
		if (CurrentGold >= Value) {
			CurrentGold -= Value;
			bResult = true;
		}
	} else {
		CurrentGold += Value;
	}
	HUD->UpdateGold(CurrentGold);
	return bResult;
}

void ATowerDefenceGameModeBase::Restart(){
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), false);
}

void ATowerDefenceGameModeBase::CloseGame() {
	GetWorld()->GetFirstPlayerController()->ConsoleCommand("quit");
}

void ATowerDefenceGameModeBase::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (WaveCurrentTimer <= 0) {
		if (!bWaveUpdated) {
			bWaveUpdated = true;
			HUD->UpdateWave(++CurrentWave);
		}

		SpawnCurrentTimer -= DeltaTime;
		if (SpawnCurrentTimer <= 0 && bCanSpawn) {
			int32 CurrentType = FMath::RandRange(0, SpawnArray[CurrentWave - 1].Num() - 1);
			
			while (SpawnArray[CurrentWave - 1][CurrentType] == 0) {
				CurrentType = FMath::RandRange(0, SpawnArray[CurrentWave - 1].Num() - 1);
			}
			ATDDwarf* Dwarf = GetWorld()->SpawnActor<ATDDwarf>(SpawnLocation, SpawnRotation);
			if (Dwarf) {
				DwarfType Type = static_cast<DwarfType>(CurrentType);
				Dwarf->Initialize(WayPoints[0], Type);
				Dwarf->OnDestroyed.AddDynamic(this, &ATowerDefenceGameModeBase::OnDwarfDestroyed);
				++AliveDwarfCount;
			}
			SpawnArray[CurrentWave - 1][CurrentType]--;
			SpawnCurrentTimer = SpawnMaxTimer;		
			
			if (MaxNum() == 0) {
				bCanSpawn = false;
			}
		}
	}
	else {
		WaveCurrentTimer -= DeltaTime;
	}
}

void ATowerDefenceGameModeBase::BreweryDestroyed(){
	bGameFinished = true;
	HUD->ShowPauseMenu();
	HUD->ShowGameEndMessage(FString("You Lose"));
}

const bool& ATowerDefenceGameModeBase::GetGameFinished(){
	return bGameFinished;
}

void ATowerDefenceGameModeBase::OnDwarfDestroyed(AActor* Actor){
	if (Actor) {
		ATDDwarf* Dwarf = Cast<ATDDwarf>(Actor);
		if (Dwarf) {
			ChangeGold(Dwarf->GetAward());
		}
	}

	--AliveDwarfCount;
	if (AliveDwarfCount <= 0) {
		bWaveUpdated = false;
		bCanSpawn = true;
		if (CurrentWave == SpawnArray.Num()) {
			bGameFinished = true;
			HUD->ShowPauseMenu();
			HUD->ShowGameEndMessage(FString("Victory!"));
		}
	}
}

int32 ATowerDefenceGameModeBase::MaxNum(){
	int32 Max = SpawnArray[CurrentWave - 1][0];

	for (unsigned i = 1; i < static_cast<unsigned>(SpawnArray[CurrentWave - 1].Num()); ++i) {
		if (Max < SpawnArray[CurrentWave - 1][i]) {
			Max = SpawnArray[CurrentWave - 1][i];
		}
	}

	return Max;
}
