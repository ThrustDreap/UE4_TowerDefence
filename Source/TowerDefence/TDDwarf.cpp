// Fill out your copyright notice in the Description page of Project Settings.


#include "TDDwarf.h"

// Sets default values
ATDDwarf::ATDDwarf()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = GetCapsuleComponent();
	GetCapsuleComponent()->SetCapsuleHalfHeight(0.44f, true);

	static ConstructorHelpers::FObjectFinder<UAnimSequence> AnimationDeathAsset(TEXT("AnimSequence'/Game/Characters/DwarfGrunt/Anims/Death2.Death2'"));
	if (AnimationDeathAsset.Succeeded()) {
		AnimationDeath = AnimationDeathAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> AnimationMoveFrdAsset(TEXT("AnimSequence'/Game/Characters/DwarfGrunt/Anims/WalkFwd2.WalkFwd2'"));
	if (AnimationMoveFrdAsset.Succeeded()) {
		AnimationMoveFrd = AnimationMoveFrdAsset.Object;
	}
	
	SkeletalComponent = GetMesh();
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshDwarf(TEXT("SkeletalMesh'/Game/Characters/DwarfGrunt/SkelMesh/DwarfGrunt_R_new.DwarfGrunt_R_new'"));
	if (MeshDwarf.Succeeded()) {
		SkeletalComponent->SetSkeletalMesh(MeshDwarf.Object);
		SkeletalComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	}

	ParticleComponentBurn = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleBurn"));
	if (ParticleComponentBurn) {
		static ConstructorHelpers::FObjectFinder<UParticleSystem> ParticleAssetBurn(TEXT("ParticleSystem'/Game/Environment/Effects/particles/P_body_burn_01.P_body_burn_01'"));
		if (ParticleAssetBurn.Succeeded()) {
			ParticleComponentBurn->SetTemplate(ParticleAssetBurn.Object);
		}
		ParticleComponentBurn->SetupAttachment(SkeletalComponent);		
		ParticleComponentBurn->bAutoActivate = false;
		ParticleComponentBurn->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
		ParticleComponentBurn->SetWorldScale3D(FVector(1.5f, 1.5f, 1.0f));
	}
	

	ParticleComponentBlood = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleBlood")); {
		static ConstructorHelpers::FObjectFinder<UParticleSystem> ParticleAssetBlood(TEXT("ParticleSystem'/Game/Environment/Effects/particles/P_blood_splash_02.P_blood_splash_02'"));
		if (ParticleAssetBlood.Succeeded()) {
			ParticleComponentBlood->SetTemplate(ParticleAssetBlood.Object);
		}
		ParticleComponentBlood->SetupAttachment(SkeletalComponent);		
		ParticleComponentBlood->bAutoActivate = false;
	}
	
	HealthPoints = 30;	
	MovementSpeed = 50.0f;

	DestroyingTimer = 5.0f;

	bIsAlive = true;

	ContiniusDamageTimer = 0.0f;
	ContiniusDamage = 0.0f;
}

// Called when the game starts or when spawned
void ATDDwarf::BeginPlay(){
	Super::BeginPlay();
}

// Called every frame
void ATDDwarf::Tick(float DeltaTime){
	Super::Tick(DeltaTime);

	if (HealthPoints <= 0 ) {
		if (bIsAlive) {
			bIsAlive = false;	
			SkeletalComponent->PlayAnimation(AnimationDeath, false);
		}
		if (!bIsAlive) {
			if (DestroyingTimer > 0) {
				DestroyingTimer -= DeltaTime;
			} else {
				Destroy();
			}
		}
	} else {
		if (CurrentTarget) {			
			FVector NewLocation = GetActorLocation();

			FVector Direction = GetActorLocation() - CurrentTarget->GetActorLocation();
			FRotator Rotation = FRotationMatrix::MakeFromX(Direction).Rotator();
			Rotation.Yaw += 90.0f;
			SetActorRotation(Rotation);

			Direction = Rotation.Vector();
			switch (int(Direction.X)) {
			case 0:
				Direction.X = Direction.Y;
				Direction.Y = 0.0f;

				NewLocation -= Direction * MovementSpeed * DeltaTime;
				break;
			default:
				Direction.Y = Direction.X;
				Direction.X = 0.0f;

				NewLocation += Direction * MovementSpeed * DeltaTime;
				break;
			}
			SetActorLocation(NewLocation);

			if (GEngine) {
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Rotation.Vector().ToString());
			}

		}
		
		if (ContiniusDamageTimer > 0) {			
			HealthPoints -= ContiniusDamage * DeltaTime;
			
			if (!bIsUnderFlame) {
				ContiniusDamageTimer -= DeltaTime;
				if (ContiniusDamageTimer <= 0) {
					ParticleComponentBurn->Deactivate();
				}
			}
		}
	}	
}

float ATDDwarf::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser){
	FString DamageTypeName;
	DamageEvent.DamageTypeClass.Get()->GetName(DamageTypeName);
	if (DamageTypeName == "TDDamageTypeFlame") {
		if (DamageAmount > 0) {
			bIsUnderFlame = true;
			if (ContiniusDamageTimer == 0) {
				UTDDamageTypeFlame* DamageType = Cast<UTDDamageTypeFlame>(DamageEvent.DamageTypeClass.GetDefaultObject());
				if (DamageType) {
					DamageType->InitDamage(ContiniusDamageTimer);
					ContiniusDamage = DamageAmount;
				}
				if (!ParticleComponentBurn->bIsActive) {
					ParticleComponentBurn->Activate();
				}
			}			
		} else {
			bIsUnderFlame = false;	
			DamageAmount = -DamageAmount;
		} 	
	} 
	if (DamageTypeName == "TDDamageTypeCannon") {
		if (!ParticleComponentBlood->bIsActive) {
			ParticleComponentBlood->Activate();
		}
	}

	HealthPoints -= DamageAmount;

	return 0.0;
}

void ATDDwarf::Initialize(TArray<ATargetPoint*>* WayPointsArray){
	auto Iter = WayPointsArray->CreateIterator();
	
	CurrentTarget = (*Iter);
}

const bool& ATDDwarf::IsAlive(){
	return bIsAlive;
}

