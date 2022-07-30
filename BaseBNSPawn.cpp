// Fill out your copyright notice in the Description page of Project Settings.
// Fill out your copyright notice in the Description page of Project Settings.
// Fill out your copyright notice in the Description page of Project Settings.
// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseBNSPawn.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include <BoneNSoul\Public\DamageWidget.h>
#include <BoneNSoul/BoneNSoulGameInstance.h>
#include <BoneNSoul/BoneNSoulGameSession.h>
#include <BoneNSoul/UI/BNSCamera.h>
#include <BoneNSoul/UI/PlayerHUDWidget.h>
#include <BNSGameState.h>
#include "../UI/IngameMenu.h"
#include <BoneNSoul/UI/StatsWidget.h>
#include "Runtime/Engine/Classes/Particles/ParticleSystemComponent.h"
#include <BoneNSoul/SkeletonPawn.h>
#include <BoneNSoul/Public/GhostPawn.h>
#include "Materials/MaterialInstanceDynamic.h"
#include <BoneNSoul/Pathfinding/APlatform.h>
#include <BoneNSoul/Pathfinding/Pathfinder.h>
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>
#include <BoneNSoul/Events/EventController.h>
#include <BoneNSoul/Triggers/ARoomSwitcher.h>
#include "../UI/IngameMenuWidget.h"


// Sets default values
ABaseBNSPawn::ABaseBNSPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bGenerateOverlapEventsDuringLevelStreaming = true;
	Speed = 1000.0f;
	InterpolationN = 0;
	InterpolationTime = 0;
	SendTimeCorrected = 0;
	PlatformID = -1;
	TimesPerSecond = 20;
	PSendTime = 1.0f / TimesPerSecond;
	Grounded = false;
	WannaJump = false;
	IsAttacking = false;
	IsCrouching = false;
	OnSemiPlatform = false;
	InEvent = false;
	PingSent = false;
	PreJumpTime = 0;
	SendTime = 0;
	T = 0.2	;
	AttackCooldown = 2;
	SelectedTier = 1;
	Tier1Charges = 1;
	PingID = 0;
	BaseHealth = 300.0f;
	MaxHealth = 300.0f;
	Health = 299.0f;

	SelectedTier1Spell = -1;
	SelectedTier2Spell = -1;
	SelectedTier3Spell = -1;
	SelectedTier4Spell = -1;


//	FlipbookDelegate.BindUFunction(this, "OnAttackEnd");


	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(RootComponent);


	//create box
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
	BoxComponent->SetupAttachment(Arrow);
	BoxComponent->SetBoxExtent(FVector(20.0f, 10.0f, 45.0f));
	BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Overlap);
	BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &ABaseBNSPawn::BoxOverlap);
	BoxComponent->OnComponentEndOverlap.AddDynamic(this, &ABaseBNSPawn::BoxOverlapEnd);

	
	Pathfinder = CreateDefaultSubobject<UPathfinder>(TEXT("Pathfinder"));



	// Use only Yaw from the controller and ignore the rest of the rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	
	FLightingChannels Channels;

	Channels.bChannel0 = false;
	Channels.bChannel1 = true;
	Channels.bChannel2 = false;


	FlipBook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Animation"));
	FlipBook->SetupAttachment(BoxComponent);
	FlipBook->LightingChannels = Channels;
	FlipBook->SetMaterial(0, LoadObject<UMaterial>(NULL, TEXT("/Game/Sprites/TransparentSprite.TransparentSprite"), NULL, LOAD_None, NULL));
	//FlipBook->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.25f));
	//FlipBook->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
	//FlipBook->SetFlipbook(IdleAnimation);
	


	//CameraHitbox = NewObject<UBoxComponent>(this, TEXT("CameraHitbox"));

	//CameraHitbox->SetupAttachment(Arrow);
	//CameraHitbox->SetBoxExtent(FVector(10.0f, 10.0f, 10.0f));
	//CameraHitbox->OnComponentBeginOverlap.AddDynamic(this, &ABaseBNSPawn::CameraHit);
	//CameraHitbox->SetWorldLocation(FVector(GetActorLocation().X, 500.0f, GetActorLocation().Z));

	
	
	DebuffParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("DebuffPSystem"));
	DebuffParticles->SetupAttachment(Arrow);
	DebuffParticles->SetTemplate(LoadObject<UParticleSystem>(NULL, TEXT("/Game/Sprites/Spells/Particles/Burning.Burning"), NULL, LOAD_None, NULL));
	
	EventController = CreateDefaultSubobject<UEventController>(TEXT("EventController"));

	
}

// Called when the game starts or when spawned
void ABaseBNSPawn::BeginPlay()
{
	Super::BeginPlay();
	EventController->RegisterComponent();
	DebuffParticles->Deactivate();
	OldPosition = GetActorLocation();
	LastPosition.Add(FVector(0.0f, 0.0f, 0.0f));
	LastPosition.Add(FVector(0.0f, 0.0f, 0.0f));
	SpellEffect = new SpellEffects();
	ItemEffect = new ItemEffects();
	DynamicMaterial = UMaterialInstanceDynamic::Create(FlipBook->GetMaterial(0), this);
	BNSHUD = Cast<ABoneNSoulHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
	FlipBook->SetMaterial(0, DynamicMaterial);
	Rotation = BoxComponent->GetComponentRotation().Yaw;
}

void ABaseBNSPawn::Possess() {



	GameInstance = GetWorld()->GetGameInstance<UBoneNSoulGameInstance>();
	GameSession = GameInstance->GetGameSession();
	BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel8, ECollisionResponse::ECR_Overlap);
	GameInstance->SetMyPawn(this, OtherPawn);
	GetWorld()->GetFirstPlayerController()->Possess(this);
	if (!BNSCamera) {
		BNSCamera = GetWorld()->SpawnActor<ABNSCamera>(FVector(GetActorLocation().X, 800.0f, GetActorLocation().Z + 100.0f), FRotator(0, -90, 0));
		BNSCamera->GetCameraComponent()->ProjectionMode = ECameraProjectionMode::Orthographic;
		BNSCamera->GetCameraComponent()->SetOrthoWidth(1520);
	}
	BNSCamera->SetPlayerPawn(this);
	
	APlayerController* FPC = GetWorld()->GetFirstPlayerController();
	FPC->SetViewTarget(BNSCamera);
	IsPossessed = true;
	ABNSGameState* const GameState = Cast<ABNSGameState>(GetWorld()->GetGameState());

	if (GameState) {

		GameState->SetCManager(BNSCamera);
	}

	if (IngameMenu) {

		

	}
	else {
		
		BNSHUD->MenuDelegate.AddDynamic(this, &ABaseBNSPawn::UpdateMenu);
	}

	BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel8, ECollisionResponse::ECR_Block);

	//Vectors.Add(FVector(GetActorLocation().X + 123.4, 0.0f, GetActorLocation().Z + 100));
	//InterpolatePosition();

}

void ABaseBNSPawn::GhostPossess(class AGhostPawn* GP) {

	FlipBook->SetSpriteColor(FLinearColor(0.3f, 0.69f, 1.0f, 1.0f));
	IsAIControlled = false;
	IsLocallyControlled = false;
	IsPossessed = true;
	Speed = PossessedMS;
	TypeOfPawn = PossessedEnemy;
	PossessingPawn = GP;
	BoxComponent->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel8, ECollisionResponse::ECR_Block);
	IngameMenu = BNSHUD->GetIngameMenu();
	GameInstance = Cast<UBoneNSoulGameInstance>(GetGameInstance());
	BNSCamera = GP->BNSCamera;
}

void ABaseBNSPawn::NetworkGhostPossess(class AGhostPawn* GP) {

	FlipBook->SetSpriteColor(FLinearColor(0.3f, 0.69f, 1.0f, 1.0f));
	IsAIControlled = false;
	PossessingPawn = GP;
	BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel8, ECollisionResponse::ECR_Block);
	IngameMenu = BNSHUD->GetIngameMenu();
}

void ABaseBNSPawn::UpdateMenu() {
	
	//IngameMenu = BNSHUD->GetIngameMenu();


	

}

void ABaseBNSPawn::SetIngameMenu(UIngameMenuWidget* _IngameMenu, UPlayerHUDWidget* _PlayerHUD){

	IngameMenu = _IngameMenu;
	PlayerHUDWidget = _PlayerHUD;


}



// Called every frame
void ABaseBNSPawn::Tick(float Seconds)
{
	Super::Tick(Seconds);
	if (!IsStunned) {
		if (FirstPosition) {
			LastPosition[0] = GetActorLocation();
			FirstPosition = false;
		}
		else {
			LastPosition[1] = GetActorLocation();
			FirstPosition = true;
		}
		DeltaTime = Seconds;
		if (IsJumping) {
			if (JumpingTime > 0.25f) {
				IsJumping = false;
				JumpingTime = 0;
			}
			JumpingTime += Seconds;
		}

	

		if (WannaJump) {


			PreJumpTime += Seconds;
			if (PreJumpTime < 0.02f) {
				if (Grounded) {
					Jump();
				}

			}
			else {

				PreJumpTime = 0;
				WannaJump = false;
			}

		}
	}


	if (IsPossessed) {

		if (LastMonsterNotificationSent < 11.0f) {
			LastMonsterNotificationSent += Seconds;
		}

		if (!IsCasting) {
			GravitySimulation();
		}
		if (GameInstance->GetGameMode() == EGameMode::Multiplayer) {
			if (!IsRezzing && !InEvent) {
				if (SendTime >= PSendTime) {
					SendPosition(false);
					SendTime -= PSendTime;
				}
				else {

					SendTime += Seconds;

				}
			}
		}



	}
	else {

		if (GameInstance) {
			if (GameInstance->GetGameMode() == EGameMode::Singleplayer && !InEvent) {
				Follow();
				if (!IsCasting) {
					GravitySimulation();
				}
			}

		}
		if (InEvent) {
			GravitySimulation();
		}
		if (Interpolating && !IsRezzing && !InEvent) {
			InterpolatePosition(Seconds);
		}
	}

	if (CurrentDebuffs.Num() > 0) {

		CycleDebuffs(Seconds);

	}

	if (IsStatsVisible) {
		if (FPSIterator < 5) {
			FPSIterator++;
			FPS += Seconds;
		}
		else {
			BNSHUD->GetStatsMenu()->UpdateFPS(5.0f / FPS);
			FPS = 0;
			FPSIterator = 0;
		}
		
		if (GameInstance->GetGameMode() == EGameMode::Multiplayer) {
				if (PingCooldown > 3.0f) {
					if (!PingSent) {
						if (PingID < 50) {
							PingID++;
						}
						else {
							PingID = 0;
						}
						GameSession->Ping(PingID);
						//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Ping Sent %i"), PingID));
						PingSent = true;
						
					}

					else {
						if (PingTime <= 10.0f) {
							PingTime += Seconds;
						}
						else {
							PingTime = 0;
							PingSent = false;
							PingCooldown = 0;
						}
					}
					PingIterator++;
				}
				else {

					PingCooldown += Seconds;
				}

		
		}

	}

	
}

void ABaseBNSPawn::ProcessPong(int _PingID) {

	if (_PingID == PingID) {
		PingSent = false;
		BNSHUD->GetStatsMenu()->UpdatePing(PingTime * 1000.0f);
		PingTime = 0;
		PingCooldown = 0;
	}


}

UEventController* ABaseBNSPawn::GetEventController(){
	return EventController;
}

//HALF SIZE
FVector ABaseBNSPawn::GetHitboxSize()
{
	return BoxComponent->GetScaledBoxExtent();
}

int ABaseBNSPawn::GetMaxHealth()
{
	return MaxHealth;
}

int ABaseBNSPawn::GetAttack()
{
	return Damage;
}

int ABaseBNSPawn::GetDefense()
{
	return Defense;
}

void ABaseBNSPawn::InterpolatePosition(float _DeltaTime) {
	InterpolationTime += _DeltaTime;


	//correct for backlog of packets
	//more packets = faster interpolation
	if (PSendTime * ceil(Vectors.Num() / 10.0f) > SendTimeCorrected && Vectors.Num() < 3) {
	//	SendTimeCorrected = FMath::Clamp((PSendTime * ceil(Vectors.Num() / 10.0f)) - 0.1f, 0.05f, 100.0f);
		SendTimeCorrected = PSendTime * ceil(Vectors.Num() / 5.0f);
	}


	if (Vectors.Num() > 0) {
		/*GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Position1 %s"), *OldPosition.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Position2 %s"), *Vectors[0].ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Position3 %s"), *GetActorLocation().ToString()));*/
		
			if (InterpolationTime >= SendTimeCorrected) {
				SetActorLocation(Vectors[0]);
				OldPosition = GetActorLocation();
				Vectors.RemoveAt(0, 1, false);
				Animations.RemoveAt(0, 1, false);
				WasRight.RemoveAt(0, 1, false);
				WasHurting.RemoveAt(0, 1, false);
				InterpolationTime = 0;
			}
			else {

				if (GetActorLocation() == Vectors[0]) {
					OldPosition = GetActorLocation();
					if (!IsDead) {
						CheckAnimation(Animations[0], WasRight[0]);
					}
					Vectors.RemoveAt(0, 1, false);
					Animations.RemoveAt(0, 1, false);
					WasRight.RemoveAt(0, 1, false);
					WasHurting.RemoveAt(0, 1, false);
					InterpolationTime = 0;
					InterpolatePosition(_DeltaTime);

				}
				else {

				  //interpolate at deltatime / packet send time to sync with everyone's fps
					bool WasCrouching = IsCrouching;

					if (WasRight[0]) {
						if (!IsDead) {
							CheckAnimation(Animations[0], true);
						}
					}
					else {
						if (!IsDead) {
							CheckAnimation(Animations[0], false);
						}
					}

					if (WasHurting[0]) {
						IsHurting = true;
					}
					else {
						IsHurting = false;
					}

					if (Vectors[0].Y != 0) {
						uint8_t RightOrLeft;
						if (Rotation == 0) {
							RightOrLeft = 1;
						}
						else {
							RightOrLeft = 0;
						}
						NetworkAttack(FVector(Vectors[0].X, 0.0f, Vectors[0].Z), Animations[0], RightOrLeft);
						Vectors[0].Y = 0;
					}
					if(WasCrouching == IsCrouching){
						SetActorLocation(FVector(GetActorLocation().X + (((Vectors[0].X - OldPosition.X) * FMath::Clamp(_DeltaTime / SendTimeCorrected, 0.0f, 1.0f))), GetActorLocation().Y, GetActorLocation().Z + ((Vectors[0].Z - OldPosition.Z) * FMath::Clamp(_DeltaTime / SendTimeCorrected, 0.0f, 1.0f))), false);
					}
					else {
						SetActorLocation(FVector(Vectors[0]), false);
					}
			//		InterpolationN++;
				}
			}

		}
	
	else {
		Interpolating = false;
	}
}


void ABaseBNSPawn::SetNewPosition(float x, float z, unsigned char _Animation, int PCounter, bool Attacking, uint8_t RightAndHurting) {
	if (!Attacking) {
		Vectors.Add(FVector(x, 0, z));		
		
	}
	else {

		Vectors.Add(FVector(x, PCounter, z));

	}
	Animations.Add(_Animation);
	switch (RightAndHurting) {
		case 0:
			WasRight.Add(false);
			WasHurting.Add(false);
			break;
		case 1:
			WasRight.Add(true);
			WasHurting.Add(false);
			break;
		case 2:
			WasRight.Add(false);
			WasHurting.Add(true);
			break;
		case 3:
			WasRight.Add(true);
			WasHurting.Add(true);
			break;

	}
		
	Interpolating = true;
}

void ABaseBNSPawn::NetworkAttack(FVector Location, unsigned char _Animation, uint8_t RightAndHurting) {


}

void ABaseBNSPawn::NetworkCast(FVector Location, unsigned char _Animation, uint16_t SpellID, uint8_t RightAndHurting) {


}


void ABaseBNSPawn::NetworkResurrect(int Position) {



}

void ABaseBNSPawn::NetworkPlayRezzAnimation(int Position) {


}


void ABaseBNSPawn::NetworkResurrectEnd(float _Health) {


}

void ABaseBNSPawn::SendMonsterPosition(bool Attacking){
	bool IsLookingRIght;
	if (Rotation == 0) {
		IsLookingRIght = true;
	}
	else {
		IsLookingRIght = false;
	}
	GameInstance->SendMonsterPosition(GetActorLocation(), Animation, NetworkID, MonsterPCounter, IsLookingRIght, Attacking, IsHurting);


	MonsterPCounter++;

	LastPositionSent = GetActorLocation();


}


void ABaseBNSPawn::SetNewMonsterPosition(float x, float z, char _Animation, bool _IsAttacking, uint8_t IsRightAndHurting, uint16_t _MonsterPCounter) {
	
	MonsterPCounter = _MonsterPCounter;
	if (!_IsAttacking) {
			Vectors.Add(FVector(x, 0, z));
		
		}
	else {

			Vectors.Add(FVector(x, 1, z));
	


	}
	Animations.Add(_Animation);
	switch (IsRightAndHurting) {
	case 0:
		WasRight.Add(false);
		WasHurting.Add(false);
		break;
	case 1:
		WasRight.Add(true);
		WasHurting.Add(false);
		break;
	case 2:
		WasRight.Add(false);
		WasHurting.Add(true);
		break;
	case 3:
		WasRight.Add(true);
		WasHurting.Add(true);
		break;

	}
	InterpolatingMonster = true;





}


void ABaseBNSPawn::InterpolateMonsterPosition(float _DeltaTime) {

	InterpolationTime += _DeltaTime;


	//correct for backlog of packets
	//more packets = faster interpolation
	if (PSendTime * ceil(Vectors.Num() / 10.0f) > SendTimeCorrected && Vectors.Num() < 3) {
		//	SendTimeCorrected = FMath::Clamp((PSendTime * ceil(Vectors.Num() / 10.0f)) - 0.1f, 0.05f, 100.0f);
		SendTimeCorrected = PSendTime * ceil(Vectors.Num() / 10.0f);
	}


	if (Vectors.Num() > 0) {


		if (InterpolationTime >= SendTimeCorrected) {
			SetActorLocation(Vectors[0]);
			OldPosition = GetActorLocation();
			Vectors.RemoveAt(0, 1, false);
			Animations.RemoveAt(0, 1, false);
			WasRight.RemoveAt(0, 1, false);
			WasHurting.RemoveAt(0, 1, false);
			InterpolationTime = 0;
		}
		else {

			if (GetActorLocation() == Vectors[0]) {
				OldPosition = GetActorLocation();
				Vectors.RemoveAt(0, 1, false);
				Animations.RemoveAt(0, 1, false);
				WasRight.RemoveAt(0, 1, false);
				WasHurting.RemoveAt(0, 1, false);
				InterpolationTime = 0;
				InterpolateMonsterPosition(_DeltaTime);

			}
			else {

				//interpolate at deltatime / packet send time to sync with everyone's fps
				
				if (WasRight[0]) {
					CheckAnimation(Animations[0], true);
				}
				else {
					CheckAnimation(Animations[0], false);
				}

				if (WasHurting[0]) {
					IsHurting = true;
				}
				else {
					IsHurting = false;
				}


	
				SetActorLocation(FVector(GetActorLocation().X + (((Vectors[0].X - OldPosition.X) * FMath::Clamp(_DeltaTime / SendTimeCorrected, 0.0f, 1.0f))), 0.0f, GetActorLocation().Z + ((Vectors[0].Z - OldPosition.Z) * FMath::Clamp(_DeltaTime / SendTimeCorrected, 0.0f, 1.0f))), false);
				if (Vectors[0].Y != 0) {

					Attack();

				}
			
			}
		}

	}

	else {
		InterpolatingMonster = false;
	}


}


// Called to bind functionality to input
void ABaseBNSPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	//Super::SetupPlayerInputComponent(PlayerInputComponent);
	IC = PlayerInputComponent;
	PlayerInputComponent->BindAxis("MoveRight", this, &ABaseBNSPawn::Move);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABaseBNSPawn::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ABaseBNSPawn::StopJump);
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ABaseBNSPawn::Attack);
	PlayerInputComponent->BindAction("OpenMenu", IE_Pressed, this, &ABaseBNSPawn::OpenMenu);
	PlayerInputComponent->BindAction("ShowStats", IE_Pressed, this, &ABaseBNSPawn::ShowStats);
	PlayerInputComponent->BindAction("MenuChange", IE_Pressed, this, &ABaseBNSPawn::MenuChange);
	PlayerInputComponent->BindAction("ChangeCharacter", IE_Pressed, this, &ABaseBNSPawn::ChangeCharacter);

	
}


void ABaseBNSPawn::GravitySimulation() {

	//check if grounded


	FCollisionShape Shape = FCollisionShape::MakeBox(FVector(BoxComponent->GetUnscaledBoxExtent().X , BoxComponent->GetUnscaledBoxExtent().Y, BoxComponent->GetUnscaledBoxExtent().Z));
	FCollisionShape ShapeCeiling = FCollisionShape::MakeBox(FVector(BoxComponent->GetUnscaledBoxExtent().X, BoxComponent->GetUnscaledBoxExtent().Y, BoxComponent->GetUnscaledBoxExtent().Z / 2));
	FVector LocationNow = BoxComponent->GetComponentLocation();
	TArray<FHitResult> HitResults;
	FHitResult TraceResult;
	FHitResult TraceResult2;
	if (!Grounded) {
		if (!IsStunned && !IsDead) {
			SetAnimation(Jumping);
		}
	}

	if (Gravity >= 0) {

		if (GetWorld()->SweepSingleByChannel(TraceResult, LocationNow, LocationNow - FVector(0.0f, 0.0f, 20.0f), FQuat::Identity, ECollisionChannel::ECC_GameTraceChannel2, Shape)) {


			
			SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, TraceResult.ImpactPoint.Z + BoxComponent->GetUnscaledBoxExtent().Z + 0.1f), true);
			
			Gravity = 0;
			Grounded = true;
			OnSemiPlatform = false;
			
				if (GameInstance->GetGameMode() == EGameMode::Singleplayer) {
					if (TraceResult.Actor->GetClass() == APlatform::StaticClass()) {

						APlatform* Act = Cast<APlatform>(TraceResult.Actor);

						if (Act->GetPlatformID() != PlatformID) {
							PlatformID = Act->PlatformID;
							if (IsPossessed) {
								OtherPawn->PlayerChangedPlatforms(PlatformID);
							}
						}

					}
				}

				if (NewPath) {
					FindPath();

				}
			
		}

		else if (GetWorld()->SweepSingleByChannel(TraceResult, LocationNow, LocationNow - FVector(0.0f, 0.0f, 20.0f), FQuat::Identity, ECollisionChannel::ECC_GameTraceChannel4, Shape) && (TraceResult.Component->GetComponentLocation().Z < (GetActorLocation().Z - BoxComponent->GetScaledBoxExtent().Z))) {
		
			SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, TraceResult.ImpactPoint.Z + BoxComponent->GetUnscaledBoxExtent().Z + 0.1f), true);
			
			Gravity = 0;
			Grounded = true;
			OnSemiPlatform = true;

			if (IsPossessed) {
				if (GameInstance->GetGameMode() == EGameMode::Singleplayer) {
					if (TraceResult.Actor->GetClass() == APlatform::StaticClass()) {

						APlatform* Act = Cast<APlatform>(TraceResult.Actor);
						if (Act->GetPlatformID() != PlatformID) {
							PlatformID = Act->PlatformID;
							if (IsPossessed) {
								OtherPawn->PlayerChangedPlatforms(PlatformID);
							}
						}

					}
				}
			}
		}




		else {



			if (GetWorld()->SweepSingleByChannel(TraceResult, LocationNow, LocationNow - FVector(0.0f, 0.0f, Gravity * FMath::Clamp(DeltaTime, 0.0f, 0.3f)), FQuat::Identity, ECollisionChannel::ECC_GameTraceChannel2, Shape)) {

				
			//	if (MaxDistance > 0) {
				
					SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, TraceResult.ImpactPoint.Z + BoxComponent->GetUnscaledBoxExtent().Z + 0.2f), true);
					Gravity = 0;
					

					//			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("gravity hit")));
					Grounded = true;
					OnSemiPlatform = false;

					if (IsPossessed) {
						if (GameInstance->GetGameMode() == EGameMode::Singleplayer) {
							if (TraceResult.Actor->GetClass() == APlatform::StaticClass()) {

								APlatform* Act = Cast<APlatform>(TraceResult.Actor);

								if (Act->GetPlatformID() != PlatformID) {
									PlatformID = Act->PlatformID;
									if (IsPossessed) {
										OtherPawn->PlayerChangedPlatforms(PlatformID);
									}
								}

							}
						}
					}
			//	}
			}


			else if (GetWorld()->SweepSingleByChannel(TraceResult, LocationNow, LocationNow - FVector(0.0f, 0.0f, 20.0f), FQuat::Identity, ECollisionChannel::ECC_GameTraceChannel4, Shape) && (TraceResult.Component->GetComponentLocation().Z < (GetActorLocation().Z - BoxComponent->GetScaledBoxExtent().Z))) {

				SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, TraceResult.ImpactPoint.Z + BoxComponent->GetUnscaledBoxExtent().Z + 0.2f), true);
				Gravity = 0;
				

				//			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("gravity hit")));
				Grounded = true;
				OnSemiPlatform = true;

				if (IsPossessed) {
					if (GameInstance->GetGameMode() == EGameMode::Singleplayer) {
						if (TraceResult.Actor->GetClass() == APlatform::StaticClass()) {

							APlatform* Act = Cast<APlatform>(TraceResult.Actor);

							if (Act->GetPlatformID() != PlatformID) {
								PlatformID = Act->PlatformID;
								if (IsPossessed) {
									OtherPawn->PlayerChangedPlatforms(PlatformID);
								}
							}

						}
					}
				}



			}


			else {
				SetActorLocation(LocationNow - FVector(0.0f, 0.0f, Gravity * FMath::Clamp(DeltaTime, 0.0f, 0.3f)), true);
				Grounded = false;

				if (IsJumping) {
				//	Gravity = -1000;
					 
				}
				else {


					if (Gravity < 1000) {

						Gravity += (((abs(Gravity) * 10.0f) + 1000) / 2) * FMath::Clamp(DeltaTime, 0.0f, 0.3f);
					
					}

					else {

						Gravity = 1000;
					}
				}
				


				//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Gravity plus	 hit")));

			}
		}


	}

	else {

		if (GetWorld()->SweepSingleByChannel(TraceResult, LocationNow + FVector (0.0f, 0.0f, BoxComponent->GetUnscaledBoxExtent().Z / 2), LocationNow + FVector(0.0f, 0.0f, BoxComponent->GetUnscaledBoxExtent().Z / 2) - FVector(0.0f, 0.0f, Gravity * FMath::Clamp(DeltaTime, 0.0f, 0.3f)), FQuat::Identity, ECollisionChannel::ECC_GameTraceChannel2, ShapeCeiling)) {
			float MaxDistance = 0;

					MaxDistance = TraceResult.Distance;


			if (MaxDistance > 0) {
				//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Distance gravity %f"), MaxDistance));
				SetActorLocation(GetActorLocation() + FVector(0.0f, 0.0f, MaxDistance - 0.1f), true);
			
				//			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("gravity hit")));
				

			}
			
			Grounded = false;
			Gravity = 0;
			IsJumping = false;
			JumpingTime = 0;
			PreJumpTime = 0;


		}

		else {

			SetActorLocation(GetActorLocation() - FVector(0.0f, 0.0f, Gravity * FMath::Clamp(DeltaTime, 0.0f, 0.3f)), true);
			if (IsJumping) {
			//	Gravity = -1000;

			}
			else {


				if (Gravity < 1000) {
					Gravity += (((abs(Gravity) * 20.0f) + 1000) / 2) * FMath::Clamp(DeltaTime, 0.0f, 1.0f);
					
				}

				else {

					Gravity = 1000;
				}
			}
			
			//SetActorLocation(GetActorLocation() - FVector(0.0f, 0.0f, Gravity * 0.005f), true);
			Grounded = false;


//			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Gravity %f"), Gravity));

		}

	}
	
	//	DeltaTime = 0;

}

void ABaseBNSPawn::SetOtherPawn(ABaseBNSPawn* _OtherPawn)
{
	if (!IsPossessed) {
		OldSpeed = Speed;
		Speed = _OtherPawn->Speed;
	}
	OtherPawn = _OtherPawn;
}


void ABaseBNSPawn::ForceMove(float Value)
{
	
}

void ABaseBNSPawn::Move(float Value)
{
	
		/*UpdateChar();*/
		FCollisionShape Shape = FCollisionShape::MakeBox(FVector(BoxComponent->GetUnscaledBoxExtent().X , BoxComponent->GetUnscaledBoxExtent().Y, BoxComponent->GetUnscaledBoxExtent().Z));
		FVector LocationNow = BoxComponent->GetComponentLocation();
		TArray<FHitResult> HitResults;
		FHitResult TraceResult;
		FVector ImpactLocation;
		FVector SurfaceNormal;
		FVector RotationVector;
		FVector DirectionNormal;
		float MaxDistance = 0.0f;
		float Modifier = 0;

		if (!IsCrouching) {

			if (Value != 0) {
				if (Value > 0) {
					Rotation = 0.0f;
					Modifier = 1;


				}
				else {
					Rotation = 180.0f;
					Modifier = -1;

				}


				BoxComponent->SetWorldRotation(FRotator(0.0f, Rotation, 0.0f));

				if (Grounded) {
					if (!IsDead) {
						SetAnimation(Running);
					}
				}
				//check upslope or wall
				if (GetWorld()->SweepSingleByChannel(TraceResult, LocationNow, LocationNow + FVector(Speed * FMath::Clamp(DeltaTime, 0.0f, 1.0f) * Modifier, 0.0f, 0.0f), FQuat::Identity, ECollisionChannel::ECC_GameTraceChannel3, Shape)) {


					/*GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("S1 %s"), *TraceResult.GetActor()->GetFName().ToString()));
					GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("S2 %s"), *LocationNow.ToString()));
					GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("S3 %f"), Shape.Box.HalfExtentZ));
					GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("S4 %s"), *TraceResult.ImpactPoint.ToString()));*/

					MaxDistance = TraceResult.Distance;
					ImpactLocation = TraceResult.ImpactPoint;
					SurfaceNormal = TraceResult.ImpactNormal;

					//	SurfaceNormal.Normalize();






					if (FVector(-1 * Modifier, 0, 0).Equals(SurfaceNormal)) {

						//			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("S2 %s"), *SurfaceNormal.ToString()));
					//	SetActorLocation(FVector((ImpactLocation.X - (BoxSize.X)) - 0.1f, GetActorLocation().Y, GetActorLocation().Z), true);
						SetActorLocation(GetActorLocation() + FVector((MaxDistance - 0.1f) * Modifier, 0.0f, 0.0f), true);

					}
					else {
						RotationVector = FVector(SurfaceNormal.Z, 0, -SurfaceNormal.X);
						SetActorLocation(GetActorLocation() + FVector((Speed * FMath::Clamp(DeltaTime, 0.0f, 0.3f)) * RotationVector.X * Modifier, 0.0f, (Speed * FMath::Clamp(DeltaTime, 0.0f, 1.0f)) * RotationVector.Z * Modifier), true);
						//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("slope %s"), *SurfaceNormal.ToString()));
					}



				}



				else {

					SetActorLocation(GetActorLocation() + FVector(Speed * FMath::Clamp(DeltaTime, 0.0f, 0.3f) * Modifier, 0.0f, 0.0f), true);


				}







			}






			else {


				if (Grounded) {
					if (!IsDead) {
						SetAnimation(Idle);
					}

				}



			}

			if (!Grounded) {
				if (!IsDead) {
					SetAnimation(Jumping);
				}

			}
		}
	

}

void ABaseBNSPawn::Stand(){
	SetAnimation(Idle);
	BoxComponent->SetBoxExtent(StandingHitboxExtent);
//	SetActorLocation(GetActorLocation() + FVector(0.0f, 0.0f, 35.f), false);
	FlipBook->SetRelativeLocation(SpriteStandingPosition);
	IsCrouching = false;


}

void ABaseBNSPawn::Crouch(){
	SetAnimation(Crouching);
	BoxComponent->SetBoxExtent(CrouchingHitboxExtent);
//	SetActorLocation(GetActorLocation() - FVector(0.0f, 0.0f, 35.0f), false);
	FlipBook->SetRelativeLocation(SpriteCrouchingPosition);
	IsCrouching = true;
}
void ABaseBNSPawn::EquipSpell(TSharedPtr<FSpellData> Spell){
	TArray<ESpellID> Tier1SpellsBuffer;
	TArray<ESpellID> Tier2SpellsBuffer;
	TArray<ESpellID> Tier3SpellsBuffer;
	TArray<ESpellID> Tier4SpellsBuffer;

	switch (Spell->SpellTier) {
		case 1:
			if (EquippedTier1Spells.Num() < Tier1Slots) {
				EquippedTier1Spells.Add(Spell);

				if (EquippedTier1Spells.Num() == 1) {

					SelectedTier1Spell = 0;
					if (PlayerHUDWidget) {
						PlayerHUDWidget->UpdateSpellIcon(Spell->SpellTier, Spell->SpellIcon);

					}

				}
			}

			for (auto EquippedSpell : EquippedTier1Spells) {
					Tier1SpellsBuffer.Add(EquippedSpell->SpellID);
					GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Equipped %i"), static_cast<int>(EquippedSpell->SpellID)));
			}
			
			IngameMenu->UpdateEquippedSpells(1, Tier1SpellsBuffer);
		
			break;
		case 2:
			if (EquippedTier2Spells.Num() < Tier2Slots) {
				EquippedTier2Spells.Add(Spell);

				if (EquippedTier2Spells.Num() == 1) {

					SelectedTier2Spell = 0;
					if (PlayerHUDWidget) {
						PlayerHUDWidget->UpdateSpellIcon(Spell->SpellTier, Spell->SpellIcon);

					}

				}
			}
			for (auto EquippedSpell : EquippedTier2Spells) {
				Tier2SpellsBuffer.Add(EquippedSpell->SpellID);
			}

			IngameMenu->UpdateEquippedSpells(2, Tier2SpellsBuffer);
			break;
		case 3:
			if (EquippedTier3Spells.Num() < Tier3Slots) {
				EquippedTier3Spells.Add(Spell);
				if (EquippedTier2Spells.Num() == 1) {

					SelectedTier3Spell = 0;
					if (PlayerHUDWidget) {
						PlayerHUDWidget->UpdateSpellIcon(Spell->SpellTier, Spell->SpellIcon);

					}

				}

			}
			for (auto EquippedSpell : EquippedTier3Spells) {
				Tier3SpellsBuffer.Add(EquippedSpell->SpellID);
			}

			IngameMenu->UpdateEquippedSpells(3, Tier3SpellsBuffer);

			break;
		case 4:
			if (EquippedTier4Spells.Num() < Tier4Slots) {

				EquippedTier4Spells.Add(Spell);
				if (EquippedTier2Spells.Num() == 1) {

					SelectedTier4Spell = 0;
					if (PlayerHUDWidget) {
						PlayerHUDWidget->UpdateSpellIcon(Spell->SpellTier, Spell->SpellIcon);

					}

				}

			}
			for (auto EquippedSpell : EquippedTier4Spells) {
				Tier4SpellsBuffer.Add(EquippedSpell->SpellID);
			}

			IngameMenu->UpdateEquippedSpells(4, Tier4SpellsBuffer);

			break;




	}
	

}

void ABaseBNSPawn::UnequipSpell(TSharedPtr<FSpellData> Spell){
	TArray<ESpellID> Tier1SpellsBuffer;
	TArray<ESpellID> Tier2SpellsBuffer;
	TArray<ESpellID> Tier3SpellsBuffer;
	TArray<ESpellID> Tier4SpellsBuffer;

	switch (Spell->SpellTier) {
		case 1:
			EquippedTier1Spells.Remove(Spell);
			if (EquippedTier1Spells.Num() > 0) {
				SelectedTier1Spell = 0;
				if (PlayerHUDWidget) {
					PlayerHUDWidget->UpdateSpellIcon(1, EquippedTier1Spells[SelectedTier1Spell]->SpellIcon);
				}
			}
			else {
				if (PlayerHUDWidget) {
					PlayerHUDWidget->UpdateCharges(1, -1);
				}
				SelectedTier1Spell = -1;
			}


			for (auto EquippedSpell : EquippedTier1Spells) {
				Tier1SpellsBuffer.Add(EquippedSpell->SpellID);
			}

			IngameMenu->UpdateEquippedSpells(1, Tier1SpellsBuffer);

			break;
		case 2:
			EquippedTier2Spells.Remove(Spell);

			for (auto EquippedSpell : EquippedTier2Spells) {
				Tier2SpellsBuffer.Add(EquippedSpell->SpellID);
			}

			IngameMenu->UpdateEquippedSpells(2, Tier2SpellsBuffer);
			break;
		case 3:
			EquippedTier3Spells.Remove(Spell);

			for (auto EquippedSpell : EquippedTier3Spells) {
				Tier3SpellsBuffer.Add(EquippedSpell->SpellID);
			}

			IngameMenu->UpdateEquippedSpells(3, Tier3SpellsBuffer);
			break;
		case 4:
			EquippedTier4Spells.Remove(Spell);

			for (auto EquippedSpell : EquippedTier4Spells) {
				Tier4SpellsBuffer.Add(EquippedSpell->SpellID);
			}

			IngameMenu->UpdateEquippedSpells(4, Tier4SpellsBuffer);
		break;




	}



}


void ABaseBNSPawn::Jump() {
	if (!IsStunned) {
		if (!IsInMenu) {
			if (Grounded) {
				//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Grounded")));
				//if (IsCrouching) {
				//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("IsCrouching")));
				//}
				//if (OnSemiPlatform) {
				//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("IsOnPlatform")));
				//}

				if (IsCrouching && OnSemiPlatform) {
					Stand();
					SetActorLocation(GetActorLocation() - FVector(0.0f, 0.0f, 20.0f), true);
					Grounded = false;
				}

				else {
					
					IsJumping = true;
					Gravity = -1000;
					
					SetActorLocation(GetActorLocation() + FVector(0.0f, 0.0f, 1.0f), true);
					Grounded = false;
					PreJumpTime = 0;
					WannaJump = false;
					JumpingTime = 0;

					if (GameInstance->GetGameMode() == EGameMode::Singleplayer) {
						OtherPawn->FollowJumpingPosition = GetActorLocation();
						OtherPawn->FollowJumpSpeed = Speed;
					}
				}
			}

			else {

				WannaJump = true;
				PreJumpTime = 0;

			}
		}
	
	}

}

void ABaseBNSPawn::StopJump() {

	if (GameInstance->GetGameMode() == EGameMode::Singleplayer) {
		OtherPawn->FollowJumpingTime = JumpingTime;
	}
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("STOPJUMP")));
	IsJumping = false;
	JumpingTime = 0;


}

void ABaseBNSPawn::Attack() {

	//if (AttackCooldown >= 0.1f) {
	//	IsAttacking = true;
	//	AttackCooldown = 0;
	//}


}

void ABaseBNSPawn::OnAttackEnd() {
	

	//IsAttacking = false;
	//FlipBook->OnFinishedPlaying.Clear();

}

void ABaseBNSPawn::TakingDamage(DamageInfo* Source) {

	//if ((!IsHurting && IsPossessed)) {
		
		int32 TotalDamage = FMath::Clamp(ceilf(Source->Damage - (Source->Damage * (Defense/100.0f))), 0.0f, 99999.0f);
		if (Health - TotalDamage < 0) {
			Health = 0;
		}
		else {
			Health -= TotalDamage;
		}

		if (GameInstance->GetMyPawn() == this) {
			BNSHUD->DrawDamage(TotalDamage, FVector(GetActorLocation().X, 0, GetActorLocation().Z - 20.0f), EColor::RED);
		}
		else if (GameInstance->GetOtherPawn() == this) {

			BNSHUD->DrawDamage(TotalDamage, FVector(GetActorLocation().X, 0, GetActorLocation().Z - 20.0f), EColor::PURPLE);
		}
		else {

			BNSHUD->DrawDamage(TotalDamage, FVector(GetActorLocation().X, 0, GetActorLocation().Z - 20.0f), EColor::WHITE);
		}
//	}
	if (!Source->SourceName.Equals("Network") && Health > 0) {
		PlayHurtAnimation();
	}
	
	delete(Source);
}

void ABaseBNSPawn::Invincibility(bool On) {
	if (On) {
		BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else {
		BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	}
}

void ABaseBNSPawn::Healing(float Healing) {

	uint32_t IHealing = Healing;
	Health += IHealing;
	if (GameSession) {
		uint8_t ClassID;
		if (IsPossessed) {
			ClassID = 0;
		}
		else {
			ClassID = 1;
		}
		GameSession->SendAppliedSpellEffect(0, ESpellEffect::NETWORKHEAL, ClassID, IHealing);
	}



}

void ABaseBNSPawn::NetworkHealing(float Healing) {

	uint32_t IHealing = Healing;
	Health += IHealing;


}

void ABaseBNSPawn::CycleDebuffs(float Seconds) {

	
	for (int i = 0; i < CurrentDebuffs.Num(); i++) {
		if (CurrentDebuffs[i].CurrentTimer > CurrentDebuffs[i].FDuration) {
			if (CurrentDebuffs[i].DebuffID == BURNING) {
				DebuffParticles->Deactivate();
			}
			CurrentDebuffs.RemoveAt(i, 1, true);
			
		}
		else {
			
			switch (CurrentDebuffs[i].DebuffID) {
				//No debuff
			case 0:
				break;
				//Burning
			case 1:
				
				if ((CurrentDebuffs[i].CurrentTimer / CurrentDebuffs[i].FInterval) >= (CurrentDebuffs[i].ITicks + 1.0f) && !CurrentDebuffs[i].Networked) {
				
					DamageInfo* Source = new DamageInfo();
					Source->SourceName = CurrentDebuffs[i].DebuffName;
					Source->Damage = CurrentDebuffs[i].Intensity;

					TakingDamage(Source);
					CurrentDebuffs[i].ITicks++;
				}

			}
			CurrentDebuffs[i].CurrentTimer += Seconds;
		}

		
		
	}

}

int ABaseBNSPawn::PlayRezzAnimation()
{
	return 0;
}

void ABaseBNSPawn::RezzAnimationFinish() {

	IsDead = false;
	IsStunned = false;

}


void ABaseBNSPawn::StopRezzing(float Position)
{
}

void ABaseBNSPawn::SetAnimation(int animation) {

	
}

void ABaseBNSPawn::PlayHurtAnimation(){

	//FlipBook->GetMaterial()->set
	MaterialColor = 1.0f;
	DynamicMaterial->SetVectorParameterValue("White", FLinearColor(MaterialColor, MaterialColor, MaterialColor, MaterialColor));
	HurtAnimationUpdate();

}

void ABaseBNSPawn::HurtAnimationUpdate(){


	if (MaterialColor > 0.0f) {
		MaterialColor -= 0.1f;
		DynamicMaterial->SetVectorParameterValue("White", FLinearColor(MaterialColor, MaterialColor, MaterialColor, 1));
		//FlipBook->SetSpriteColor(FLinearColor(1.0f, FlipBook->GetSpriteColor().G + 0.05f, FlipBook->GetSpriteColor().B + 0.05f, 1.0f));
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ABaseBNSPawn::HurtAnimationUpdate);
	}
	else {
		MaterialColor = 0.0f;
		DynamicMaterial->SetVectorParameterValue("White", FLinearColor(0, 0, 0, 1));
	//	FlipBook->SetSpriteColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
	}

}

void ABaseBNSPawn::CheckAnimation(unsigned char _Animation, bool Right) {
	



}

void ABaseBNSPawn::Rotate(uint16_t _Rotation)
{
	Rotation = _Rotation;
	BoxComponent->SetWorldRotation(FRotator(0.0f, _Rotation, 0.0f));
}

void ABaseBNSPawn::SaveRange(bool _CanSave)
{
	CanSave = _CanSave;

}

void ABaseBNSPawn::OpenSaveMenu()
{
	if (BNSHUD) {
		if (GameInstance) {
			if (GameInstance->GetMyPawn() == this) {

				BNSHUD->OpenSaveMenu(1);

			}
			else {

				BNSHUD->OpenSaveMenu(2);

			}
			IsStunned = true;
		}
		

	}


}

void ABaseBNSPawn::KillSelf() {

	Destroy();
}

void ABaseBNSPawn::HitOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	
	if (OtherActor->GetClass() == ASkeletonPawn::StaticClass() || OtherActor->GetClass() == AGhostPawn::StaticClass()) {
		
		if (OtherComp->GetCollisionObjectType() == ECollisionChannel::ECC_Pawn) {
			ABaseBNSPawn* Act = Cast<ABaseBNSPawn>(OtherActor);
			OverlappedPawns.Add(Act);

			DamageInfo* Source = new DamageInfo();
			Source->SourceName = FString("Body Damage");
			Source->Damage = BodyDamage;
			Source->DamagingComp = OverlappedComp;
			Source->DamagedComp = OtherComp;

			Act->TakingDamage(Source);

		}

	}





}

void ABaseBNSPawn::HitOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) {

	if (OtherActor->GetClass() == ASkeletonPawn::StaticClass() || OtherActor->GetClass() == AGhostPawn::StaticClass()) {

		ABaseBNSPawn* Act = Cast<ABaseBNSPawn>(OtherActor);
		OverlappedPawns.Remove(Act);

	}





}


void ABaseBNSPawn::OpenMenu(){

	
	IsInMenu = BNSHUD->OpenCloseIngameMenu();

}

void ABaseBNSPawn::ShowStats() {
	
	IsStatsVisible = BNSHUD->ShowStats();

}


void ABaseBNSPawn::DamageOverlappedPawns(){

	for (ABaseBNSPawn* Pawn : OverlappedPawns) {

		DamageInfo* Source = new DamageInfo();
		Source->SourceName = FString("Body Damage");
		Source->Damage = BodyDamage;
		Pawn->TakingDamage(Source);


	}

}

void ABaseBNSPawn::MenuChange() {
	if (IsInMenu) {

		
			BNSHUD->ChangeMenuSideOption();

		
		


	}


}

void ABaseBNSPawn::AddDebuff(ABaseBNSPawn* From, DebuffData Debuff) {
	bool Added = false;

	for (int i = 0; i < CurrentDebuffs.Num(); i++) {

		if (Debuff.DebuffID == CurrentDebuffs[i].DebuffID) {
			CurrentDebuffs[i].CurrentTimer -= CurrentDebuffs[i].ITicks * CurrentDebuffs[i].FInterval;
			CurrentDebuffs[i].ITicks = 0;
			Added = true;
		}


	}

	if (!Added) {
		CurrentDebuffs.Add(Debuff);
		if (Debuff.DebuffID == BURNING) {
			DebuffParticles->Activate();

		}
	}
}

void ABaseBNSPawn::AddSpell(FSpellData Spell) {
	TSharedPtr<FSpellData> SpellBuffer = MakeShareable(new FSpellData());
	SpellBuffer->DamageType = Spell.DamageType;
	SpellBuffer->Debuffs = Spell.Debuffs;
	SpellBuffer->HitboxSize = Spell.HitboxSize;
	SpellBuffer->IBaseDamage = Spell.IBaseDamage;
	SpellBuffer->IBaseManaCost = Spell.IBaseManaCost;
	SpellBuffer->IsProjectile = Spell.IsProjectile;
	//SpellBuffer->SpellAnimation = Spell.SpellAnimation;
	SpellBuffer->SpellIcon = Spell.SpellIcon;
	SpellBuffer->SpellID = Spell.SpellID;
	SpellBuffer->SpellLocation = Spell.SpellLocation;
	SpellBuffer->SpellName = Spell.SpellName;
	SpellBuffer->SpellText = Spell.SpellText;
	SpellBuffer->SpellTier = Spell.SpellTier;
	SpellBuffer->SpellFunction = Spell.SpellFunction;

	switch (SpellBuffer->SpellTier) {
		case 1:
			Tier1Spells.Add(SpellBuffer);

			break;
		case 2:
			Tier2Spells.Add(SpellBuffer);

			break;
		case 3:
			Tier3Spells.Add(SpellBuffer);

			break;
		case 4:
			Tier4Spells.Add(SpellBuffer);

			break;
	}

	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Spell: %i"), Spell.SpellID));
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Spell: %s"), *Spell.SpellName));
}


void ABaseBNSPawn::AddItem(FItemData Item) {
	TSharedPtr<FItemData> ItemBuffer = MakeShareable(new FItemData());
	ItemBuffer->ItemID = Item.ItemID;
	ItemBuffer->ItemName = Item.ItemName;
	ItemBuffer->ItemText = Item.ItemText;
	ItemBuffer->ItemType = Item.ItemType;
	ItemBuffer->ItemIcon = Item.ItemIcon;
	ItemBuffer->ItemFunctions = Item.ItemFunctions;
	ItemBuffer->GhostEquip = Item.GhostEquip;
	ItemBuffer->SkeletonEquip = Item.SkeletonEquip;
	ItemBuffer->CanSell = Item.CanSell;
	ItemBuffer->CanDiscard = Item.CanDiscard;
	ItemBuffer->Value = Item.Value;
	ItemBuffer->BonusDamage = Item.BonusDamage;
	ItemBuffer->BonusDefense = Item.BonusDefense;
	ItemBuffer->BonusHP = Item.BonusHP;

	InventoryItems.Add(ItemBuffer);

	//IngameMenu->UpdateItemList(EPlayer::);



}

void ABaseBNSPawn::SetItemManager(AItemManager* _ItemManager){

	ItemManager = _ItemManager;


}

void ABaseBNSPawn::EquipItem(TSharedPtr<FItemData> Item) {
	
	switch (Item->ItemType) {
		case 1:
			if (EquippedWeapon) {
				for (auto F : EquippedWeapon->ItemFunctions) {
					ItemEffect->Equip = F;
					(ItemEffect->*(ItemEffect->Equip))(this, Item, false);
				}
			}
			EquippedWeapon = Item;
			break;
		case 2:
			if (EquippedArmor) {
				for (auto F : EquippedArmor->ItemFunctions) {
					ItemEffect->Equip = F;
					(ItemEffect->*(ItemEffect->Equip))(this, Item, false);
				}
			}

			EquippedArmor = Item;
			break;
		case 3:
			if (EquippedAccessory) {

				for (auto F : EquippedAccessory->ItemFunctions) {
					ItemEffect->Equip = F;
					(ItemEffect->*(ItemEffect->Equip))(this, Item, false);
				}
			}
			EquippedAccessory = Item;
			break;
	}

	UpdateItemStats();

	for (auto F : Item->ItemFunctions) {
		ItemEffect->Equip = F;
		(ItemEffect->*(ItemEffect->Equip))(this, Item, true);
	}
}

void ABaseBNSPawn::UnequipItem(TSharedPtr<FItemData> Item) {

	switch (Item->ItemType) {
	case 1:
		if (EquippedWeapon) {
			for (auto F : EquippedWeapon->ItemFunctions) {
				ItemEffect->Equip = F;
				(ItemEffect->*(ItemEffect->Equip))(this, Item, false);
			}
		}
		EquippedWeapon = NULL;
		break;
	case 2:
		if (EquippedArmor) {
			for (auto F : EquippedArmor->ItemFunctions) {
				ItemEffect->Equip = F;
				(ItemEffect->*(ItemEffect->Equip))(this, Item, false);
			}
		}
		EquippedArmor = NULL;
		break;
	case 3:
		if (EquippedAccessory) {

			for (auto F : EquippedAccessory->ItemFunctions) {
				ItemEffect->Equip = F;
				(ItemEffect->*(ItemEffect->Equip))(this, Item, false);
			}
		}
		EquippedAccessory = NULL;
		break;
	case 4:
	
		break;
	}

	UpdateItemStats();
}

void ABaseBNSPawn::UpdateItemStats() {
	int NewHealth = BaseHealth;
	int NewDamage = BaseDamage;
	int NewDefense = BaseDefense;

	if (EquippedWeapon) {
		NewHealth += EquippedWeapon->BonusHP;
		NewDamage += EquippedWeapon->BonusDamage;
		NewDefense += EquippedWeapon->BonusDefense;
	}
	if (EquippedArmor) {
		NewHealth += EquippedArmor->BonusHP;
		NewDamage += EquippedArmor->BonusDamage;
		NewDefense += EquippedArmor->BonusDefense;

	}
	if (EquippedAccessory) {
		NewHealth += EquippedAccessory->BonusHP;
		NewDamage += EquippedAccessory->BonusDamage;
		NewDefense += EquippedAccessory->BonusDefense;


	}

	MaxHealth = NewHealth;
	Damage = NewDamage;
	Defense = NewDefense;

}

int ABaseBNSPawn::DiscardItem(TSharedPtr<FItemData> Item) {

	UnequipItem(Item);
	
	return InventoryItems.RemoveSingle(Item);

}

void ABaseBNSPawn::SetLocallyControlled(bool Controlled){
	IsLocallyControlled = Controlled;
}

void ABaseBNSPawn::Die()
{
}

void ABaseBNSPawn::Patrol()
{
}

void ABaseBNSPawn::CheckForPlayer(float _Distance)
{
}

void ABaseBNSPawn::ChasePlayer()
{
}

void ABaseBNSPawn::LookAtPlayer(bool AtMyPawn)
{
}

void ABaseBNSPawn::CheckForGiveUp()
{
}

void ABaseBNSPawn::Unpossess()
{

}


int ABaseBNSPawn::GiveItem(TSharedPtr<FItemData> Item) {
	return 0;
}

void ABaseBNSPawn::RequestItem(TSharedPtr<FItemData> Item)
{
}

void ABaseBNSPawn::MoveToPoint(float PointX) {

	if (abs(GetActorLocation().X - PointX) < 20.0f) {
		SetActorLocation(FVector(PointX, GetActorLocation().Y, GetActorLocation().Z));


	}
	
	else {
		if (GetActorLocation().X < PointX) {
			Move(1);
		}
		else {
			Move(-1);
		}
	}

}

void ABaseBNSPawn::TeleportToOtherPawn() {
	if (OtherPawn) {
		if (OtherPawn->Grounded) {
			SetActorLocation(OtherPawn->GetActorLocation());
		}
	}

}



void ABaseBNSPawn::Follow() {

	if (OtherPawn ) {
		if (abs(OtherPawn->GetActorLocation().X - GetActorLocation().X) > 3000.0f || abs(OtherPawn->GetActorLocation().Z - GetActorLocation().Z) > 3000.0f) {
	//		TeleportToOtherPawn();




		}

		else{
			if (PlatformID != -1 && OtherPawn->PlatformID != -1) {
				//	Pathfinder->FindPath(PlatformID, OtherPawn->PlatformID);

				if (PlatformID == OtherPawn->PlatformID) {
					if (JumpingTime >= FollowJumpingTime && FollowJumpingTime != 0.0f) {
						StopJump();
						FollowJumpingTime = 0;
					}

					if (FollowJumpingPosition != FVector(0.0f, 0.0f, 0.0f)) {

						if (abs(GetActorLocation().X - FollowJumpingPosition.X) < 2.0f) {
							Jump();
							FollowJumpingPosition = FVector(0.0f, 0.0f, 0.0f);
						}

					}
					FHitResult TraceResult;

				/*	if (OtherPawn->GetActorLocation().X > GetActorLocation().X + 150.0f && GetWorld()->LineTraceSingleByChannel(TraceResult, FVector(GetActorLocation().X + BoxComponent->GetScaledBoxExtent().X, 0, GetActorLocation().Z), FVector(GetActorLocation().X + BoxComponent->GetScaledBoxExtent().X, 0, GetActorLocation().Z - (BoxComponent->GetScaledBoxExtent().Z + 50.0f)), ECollisionChannel::ECC_GameTraceChannel2)) {
						Move(1);
					}
					else if (OtherPawn->GetActorLocation().X < GetActorLocation().X - 150.0f && GetWorld()->LineTraceSingleByChannel(TraceResult, FVector(GetActorLocation().X - BoxComponent->GetScaledBoxExtent().X, 0, GetActorLocation().Z), FVector(GetActorLocation().X - BoxComponent->GetScaledBoxExtent().X, 0, GetActorLocation().Z - (BoxComponent->GetScaledBoxExtent().Z + 50.0f)), ECollisionChannel::ECC_GameTraceChannel2)) {
						Move(-1);
					}*/
					if (OtherPawn->GetActorLocation().X > GetActorLocation().X + 150.0f) {
						Move(1);
					}
					else if (OtherPawn->GetActorLocation().X < GetActorLocation().X - 150.0f) {
						Move(-1);
					}
					else {

						Move(0);
					}
				}

				else {

					if (HasPath) {
					
						MoveToPlatform(OtherPawn->PlatformID);
					}


				}
			}
		}
	}
	

}

void ABaseBNSPawn::MoveToPlatform(int _PlatformID) {

	if (HasVectorPath) {
	//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("VectorPathed")));
		if (VectorPath.Num() > 0) {
			//for (auto path : VectorPath) {
			//	
			//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("VECTORPATH %s"), *path.ToString()));
			//}
//			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("GOAL %f"), VectorPath[0].X));
			
			if (abs(VectorPath[0].X - GetActorLocation().X) < 20.0f) {


				if (GetActorLocation().X != VectorPath[0].X) {
					GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("GOAL1 %f"), VectorPath[0].X));
				//	StopJump();
					SetActorLocation(FVector(VectorPath[0].X, GetActorLocation().Y, GetActorLocation().Z));
				}

				else {

					if (Grounded) {
						GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("GOAL2 %f"), VectorPath[0].X));
						VectorPath.RemoveAt(0);

						if (VectorPath.Num() > 0) {
							if (HasToJump.Num() > 0) {
								if (HasToJump[0]) {
									
									Jump();

								}
							
								HasToJump.RemoveAt(0);
							}

						}
						else {
							if (PlatformID == OtherPawn->PlatformID) {
								HasVectorPath = false;
								HasPath = false;
							}
							else {
								FindPath();
							}
						}
					}
				}
			}





			else {
				if (VectorPath[0].X > GetActorLocation().X) {
					Move(1);
				}
				else if (VectorPath[0].X < GetActorLocation().X) {
					Move(-1);
				}
			}
		}

		

	}

	else {
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("NotVectorPathed %i"), PlatformPath.Num()));

		for (int i = 0; i < PlatformPath.Num(); i++) {
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Neighbors.Num() %i"), Pathfinder->RoomPlatforms[PlatformPath[i]]->Neighbors.Num()));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("PlatformName %s"), *Pathfinder->RoomPlatforms[PlatformPath[i]]->GetFName().ToString()));
			for (FNeighbor Neighbor : Pathfinder->RoomPlatforms[PlatformPath[i]]->Neighbors) {

				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("III %i"), i));
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("PlatformPath.Num() %i"), PlatformPath.Num()));
				if (i < PlatformPath.Num() - 1) {
							GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("ID %i"), Neighbor.PlatformID));
					if (Neighbor.PlatformID == PlatformPath[i + 1]) {
						if (Neighbor.TransitionPoint == EPoint::A) {
							VectorPath.Add(Pathfinder->RoomPlatforms[PlatformPath[i]]->PointA);
							GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("ID POINT A %s"), *Pathfinder->RoomPlatforms[PlatformPath[i]]->PointA.ToString()));
						}
						else {
							VectorPath.Add(Pathfinder->RoomPlatforms[PlatformPath[i]]->PointB);
							GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("ID POINT B %s"), *Pathfinder->RoomPlatforms[PlatformPath[i]]->PointB.ToString()));
						}

						if (Neighbor.TransitionMode == ETransitionMode::Jump) {
							GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("JUMP1")));
							HasToJump.Add(true);
						}
						else {
							GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("NOJUMP1")));
							HasToJump.Add(false);
						}

					/*	if (PlatformPath.Num() > 0) {
							PlatformPath.Pop();
						}*/
						break;
					}
				}
				else if(i == PlatformPath.Num() - 1) {
					GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("ID %i"), Neighbor.PlatformID));
					if (Neighbor.PlatformID == PlatformPath[i - 1]) {
						if (Neighbor.TransitionPoint == EPoint::A) {
							VectorPath.Add(Pathfinder->RoomPlatforms[PlatformPath[i]]->PointA);
							GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("ID %s"), *Pathfinder->RoomPlatforms[Neighbor.PlatformID]->PointA.ToString()));
						}
						else {
							VectorPath.Add(Pathfinder->RoomPlatforms[PlatformPath[i]]->PointB);
							GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("ID %s"), *Pathfinder->RoomPlatforms[Neighbor.PlatformID]->PointB.ToString()));
						}

						if (Neighbor.TransitionMode == ETransitionMode::Jump) {
							GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("JUMP2")));
							HasToJump.Add(true);
						}
						else {
							GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("NOJUMP2")));
							HasToJump.Add(false);
						}
						

						/*if (PlatformPath.Num() > 0) {
							PlatformPath.Pop();
						}*/
						break;

					}
				}
				


			}

			
		}

		PlatformPath.Empty();
		HasVectorPath = true;

	}



}

void ABaseBNSPawn::FindPath() {
	PlatformPath.Empty();
	HasToJump.Empty();
	VectorPath.Empty();
	NewPath = false;
	HasVectorPath = false;
	PlatformPath = Pathfinder->FindPath(PlatformID, OtherPawn->PlatformID);
	if (PlatformPath.Num() > 0) {
		HasPath = true;
	}

}

void ABaseBNSPawn::LockCamera(float _LocationX, float _LocationY)
{
	if (BNSCamera) {
		BNSCamera->SetActorLocation(FVector(_LocationX, BNSCamera->GetActorLocation().Y, _LocationY));
		BNSCamera->UnsetPlayerPawn();
	}

}

void ABaseBNSPawn::UnlockCamera()
{
	if (BNSCamera) {
		BNSCamera->SetPlayerPawn(this);
	}
}

void ABaseBNSPawn::ChangeCharacter() {
	if (!IsInMenu) {
		IsPossessed = false;
		if (BNSCamera) {
			BNSCamera->UnsetBGLayer();
		}
		BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel8, ECollisionResponse::ECR_Ignore);
		OtherPawn->Possess();
	}
}

void ABaseBNSPawn::SendMultiplayerPosition() {
	OldPosition = GetActorLocation();
	OtherPawn->Vectors.Add(GetActorLocation());

}


void ABaseBNSPawn::BoxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {

	//CameraHitbox->SetWorldLocation(FVector(OtherActor->GetActorLocation().X, 500.0f, GetActorLocation().Z));
	
}

void ABaseBNSPawn::BoxOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}

void ABaseBNSPawn::NetworkRoomChange(float x, float z, uint8_t _Animation, uint8_t LookingRight, uint16_t RoomSwitcher) {
	Vectors.Empty();
	Animations.Empty();
	WasRight.Empty();
	WasHurting.Empty();
	Interpolating = false;

	if (LookingRight == 0) {
		Rotation = 0.0f;
	}
	else {
		Rotation = 180.0f;
	}

	BoxComponent->SetWorldRotation(FRotator(0.0f, Rotation, 0.0f));
	SetActorLocation(FVector(x, GetActorLocation().Y, z));
	SetAnimation(_Animation);
	GetEventController()->SwitchRooms(this, RoomSwitcher, GameInstance->GetRoomSwitchers()[RoomSwitcher]->SwitcherToNumber, GameInstance->GetRoomSwitchers()[RoomSwitcher]->SwitcherToRoomNumber, false, 0);


}

void ABaseBNSPawn::PlayerChangedPlatforms(int _PlatformID){

	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Trying, platform %i"), _PlatformID));
	

	if (PlatformID != -1) {

		if (!Grounded) {
			NewPath = true;
		}
		else {
			

			if (PlayerRoom == OtherPawn->PlayerRoom) {
				FindPath();
			}

		}
		

		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("num %i"), PlatformPath.Num()));
	//	for (int i : Pathfinder->FindPath(PlatformID, OtherPawn->PlatformID)) {
		
		//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Position2 %i"), i));

		//}
	}

}

void ABaseBNSPawn::SendPosition(bool Attacking)
{
	bool LookingRight;
	if (BoxComponent->GetComponentRotation().Yaw == 0) {
		LookingRight = true;
	}
	else {
		LookingRight = false;
	}
	if (!Attacking) {

		GameSession->SendPosition(GetActorLocation(), Animation, false, LookingRight, IsHurting);
		LastPositionSent = GetActorLocation();
		LastAnimationSent = Animation;
		
	}
	else {

		GameSession->SendPosition(GetActorLocation(), Animation, true, LookingRight, IsHurting);
		LastPositionSent = GetActorLocation();
		LastAnimationSent = Animation;



	}
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Send Position")));


}

void ABaseBNSPawn::SendPositionCasting(ESpellID SpellID) {

	bool LookingRight;
	if (BoxComponent->GetComponentRotation().Yaw == 0) {
		LookingRight = true;
	}
	else {
		LookingRight = false;
	}


	LastPositionSent = GetActorLocation();
	LastAnimationSent = Animation;

	GameSession->SendCastingPosition(GetActorLocation(), Animation, static_cast<int>(SpellID), LookingRight, IsHurting);


}