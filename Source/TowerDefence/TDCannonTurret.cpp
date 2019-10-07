// Fill out your copyright notice in the Description page of Project Settings.


#include "TDCannonTurret.h"

// Sets default values
ATDCannonTurret::ATDCannonTurret()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StaticMeshFoundation = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshFoundation"));
	if (StaticMeshFoundation) {
		SetRootComponent(StaticMeshFoundation);
		static ConstructorHelpers::FObjectFinder<UStaticMesh> FoundationMesh(TEXT("StaticMesh'/Game/Assets/Meshes/CannonTower_StaticElement.CannonTower_StaticElement'"));
		if (FoundationMesh.Succeeded()) {
			StaticMeshFoundation->SetStaticMesh(FoundationMesh.Object);
		}
	}
	
	SkeletalMeshCannon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshCannon"));
	if (SkeletalMeshCannon) {
		SkeletalMeshCannon->AttachTo(RootComponent);
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> CannonMesh(TEXT("SkeletalMesh'/Game/Assets/Meshes/Cannon_Tower_SkelMesh.Cannon_Tower_SkelMesh'"));
		if (CannonMesh.Succeeded()) {
			SkeletalMeshCannon->SetSkeletalMesh(CannonMesh.Object);
			SkeletalMeshCannon->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));
			SkeletalMeshCannon->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
		}
	}

	ConstructionBarWidget = CreateWidget<UTDConstructionBarWidget>(GetWorld(), UTDConstructionBarWidget::StaticClass());

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	if (CollisionComponent) {
		CollisionComponent->AttachTo(RootComponent);
		CollisionComponent->SetRelativeLocation(FVector(0.0f, 550.0f, 0.0f));
		CollisionComponent->SetWorldScale3D(FVector(10.0f, 10.0f, 1.0f));
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ATDCannonTurret::OnCollisionBeginOverlap);
		CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &ATDCannonTurret::OnCollisionEndOverlap);
		CollisionComponent->SetGenerateOverlapEvents(false);
	}

	ParticleComponentCreation = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleCreation"));
	if (ParticleComponentCreation) {
		static ConstructorHelpers::FObjectFinder<UParticleSystem> ParticleAsset(TEXT("ParticleSystem'/Game/Environment/Effects/particles/P_building_creation_01.P_building_creation_01'"));
		if (ParticleAsset.Succeeded()) {
			ParticleComponentCreation->SetTemplate(ParticleAsset.Object);
			ParticleComponentCreation->AttachTo(RootComponent);
			ParticleComponentCreation->bAutoActivate = true;
		}
	}

	CurrentTarget = nullptr;

	Damage = 10.0f;

	ReloadMaxTime = 3.0f;

	ReloadCurrentTime = 0.0f;

	Targets = TArray<AActor*>();

	bConstructed = false;
}

void ATDCannonTurret::Initiliaze(const float& Timer) {
	ConstructionBarWidget->AddToViewport();
	ConstructionBarWidget->SetPositionAndTime(GetActorLocation(), Timer, &bConstructed);
}

// Called when the game starts or when spawned
void ATDCannonTurret::BeginPlay(){
	Super::BeginPlay();	
}

// Called every frame
void ATDCannonTurret::Tick(float DeltaTime){
	Super::Tick(DeltaTime);

	if (bConstructed) {
		if (ConstructionBarWidget->IsInViewport()) {
			ConstructionBarWidget->RemoveFromViewport();
			CollisionComponent->SetGenerateOverlapEvents(true);
		}

		ReloadCurrentTime -= DeltaTime;

		while (Targets.Num() && CurrentTarget == nullptr) {
			CurrentTarget = Cast<ATDDwarf>(Targets.Pop());
		}

		if (CurrentTarget) {
			FVector Direction = SkeletalMeshCannon->GetComponentLocation() - CurrentTarget->GetActorLocation();
			FRotator Rot = FRotationMatrix::MakeFromX(Direction).Rotator();
			Rot.Yaw += 180.0f;
			SkeletalMeshCannon->SetRelativeRotation(Rot);



			if (ReloadCurrentTime <= 0) {
				if (CurrentTarget->IsAlive()) {
					FActorSpawnParameters SpawnParams;
					FTransform SpawnPlace = GetTransform();
					SpawnPlace.AddToTranslation(Direction - SpawnPlace.GetLocation());
					ATDCannonBall* Ball = GetWorld()->SpawnActor<ATDCannonBall>(ATDCannonBall::StaticClass(), SpawnPlace, SpawnParams);
					Ball->Initialize(CurrentTarget->GetActorLocation(), Damage);
					ReloadCurrentTime = ReloadMaxTime;
				} else {
					CurrentTarget = nullptr;
				}
			}
		}		
	}	
}

void ATDCannonTurret::OnCollisionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult){

	if (OtherActor != nullptr && OtherActor->GetClass()->GetName() == "TDDwarf") {
		Targets.Add(OtherActor);			
	}
}

void ATDCannonTurret::OnCollisionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex){
	Targets.Remove(OtherActor);
	if (CurrentTarget == OtherActor) {
		CurrentTarget = nullptr;		
	}
}


