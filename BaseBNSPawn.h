// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "PaperFlipbook.h"
#ifndef Include_Spell_Types
#define Include_Spell_Types
#include "../Spells/SpellTypes.h"
#endif
#include <BoneNSoul/Items/ItemManager.h>
#include "BaseBNSPawn.generated.h"


struct DamageInfo {
	FString SourceName;
	float Damage;
	UPrimitiveComponent* DamagingComp;
	UPrimitiveComponent* DamagedComp;
	FVector ImpactPoint;


};


UCLASS()
class BONENSOUL_API ABaseBNSPawn : public APawn
{
	GENERATED_BODY()





public:
	// Sets default values for this pawn's properties
	ABaseBNSPawn();
	virtual void Possess();
	virtual void GhostPossess(class AGhostPawn* GP);
	void NetworkGhostPossess(AGhostPawn* GP);
	virtual void SetIngameMenu(class UIngameMenuWidget* _IngameMenu, class UPlayerHUDWidget* _PlayerHUD);
	UFUNCTION()
	virtual void UpdateMenu();
	enum EType { Player, Enemy, PossessedEnemy };
	EType TypeOfPawn;
	uint8_t NetworkID;

	class ABNSCamera* BNSCamera;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;



	enum EVerticality { Straight, Up, DiagonalUp, DiagonalDown, Down };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
		class UBoxComponent* BoxComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
		class UPathfinder* Pathfinder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
		class USpringArmComponent* CameraBoom;

	// The animation to play while running around
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
		class UPaperFlipbookComponent* FlipBook;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gay)
		class UArrowComponent* Arrow;

	/** Side view camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		class UCameraComponent* SideViewCameraComponent;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
		class UBoxComponent* CameraHitbox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
		class UParticleSystemComponent* DebuffParticles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
		class UPaperFlipbook* DeathAnimation;

	UPROPERTY(VisibleAnywhere)
		class UEventController* EventController;


	UFUNCTION()
	virtual void HitOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void HitOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OpenMenu();
	void ShowStats();
	virtual void DamageOverlappedPawns();
	void MenuChange();
	
	class UIngameMenuWidget* IngameMenu;
	class UPlayerHUDWidget* PlayerHUDWidget;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void ProcessPong(int _PingID);
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	class UEventController* GetEventController();


	FVector GetHitboxSize();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	int GetMaxHealth();
	UFUNCTION(BlueprintCallable, Category = "Stats")
	int GetAttack();
	UFUNCTION(BlueprintCallable, Category = "Stats")
	int GetDefense();

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void AddDebuff(ABaseBNSPawn* From, struct DebuffData Debuff);

	virtual void GravitySimulation();

	virtual void SetOtherPawn(ABaseBNSPawn* _OtherPawn);
	
	virtual void ForceMove(float Value);
	virtual void Move(float Value);
	virtual void Stand();
	virtual void Crouch();


	virtual void EquipSpell(TSharedPtr<FSpellData> Spell);
	virtual void UnequipSpell(TSharedPtr<FSpellData> Spell);

	virtual int GiveItem(TSharedPtr<FItemData> Item);
	virtual void RequestItem(TSharedPtr<FItemData> Item);


	virtual void Jump();
	virtual void ChangeCharacter();
	void SendMultiplayerPosition();
	void StopJump();

	virtual void Attack();

	UFUNCTION()
	virtual void OnAttackEnd();

	virtual void TakingDamage(DamageInfo* Source);

	void Invincibility(bool On);

	virtual void Healing(float Healing);

	virtual void NetworkHealing(float Healing);

	void CycleDebuffs(float Seconds);

	virtual int PlayRezzAnimation();
	virtual void StopRezzing(float Position);
	virtual void SetAnimation(int animation);
	virtual void PlayHurtAnimation();
	virtual void HurtAnimationUpdate();
	virtual void KillSelf();
	virtual void CheckAnimation(unsigned char _Animation, bool Right);
	virtual void Rotate(uint16_t _Rotation);
	virtual void SaveRange(bool _CanSave);
	virtual void OpenSaveMenu();

	UFUNCTION()
	virtual void BoxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void BoxOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void NetworkRoomChange(float x, float z, uint8_t _Animation, uint8_t LookingRight, uint16_t RoomSwitcher);

	virtual void PlayerChangedPlatforms(int _PlatformID);
	virtual void SendPosition(bool Attacking);

	void SendPositionCasting(ESpellID SpellID);

	virtual void InterpolatePosition(float _DeltaTime);

	virtual void SetNewPosition(float x, float z, unsigned char _Animation, int PCounter, bool Attacking, uint8_t RightAndHurting);

	virtual void NetworkAttack(FVector Location, unsigned char _Animation, uint8_t RightAndHurting);

	virtual void NetworkCast(FVector Location, unsigned char _Animation, uint16_t SpellID, uint8_t RightAndHurting);

	virtual void NetworkResurrect(int Position);

	virtual void NetworkPlayRezzAnimation(int Position);

	virtual void NetworkResurrectEnd(float _Health);

	virtual void SendMonsterPosition(bool Attacking);

	virtual void SetNewMonsterPosition(float x, float z, char _Animation, bool _IsAttacking, uint8_t IsRightAndHurting, uint16_t _MonsterPCounter);

	virtual void InterpolateMonsterPosition(float _DeltaTime);

	virtual void AddSpell(struct FSpellData Spell);
	virtual void AddItem(FItemData Item);

	void SetItemManager(AItemManager* _ItemManager);
	virtual void EquipItem(TSharedPtr<FItemData> Item);

	virtual void UnequipItem(TSharedPtr<FItemData> Item);

	void UpdateItemStats();

	virtual int DiscardItem(TSharedPtr<FItemData> Item);

	virtual void SetLocallyControlled(bool Controlled);

	UFUNCTION()
	virtual void RezzAnimationFinish();

	UFUNCTION()
	virtual void Die();

	virtual void Patrol();
	virtual void CheckForPlayer(float _Distance);
	virtual void ChasePlayer();
	virtual void LookAtPlayer(bool AtMyPawn);
	virtual void CheckForGiveUp();
	virtual void Unpossess();

	void MoveToPoint(float XLocation);

	void TeleportToOtherPawn();

	void Follow();

	void MoveToPlatform(int _PlatformID);

	void FindPath();

	void LockCamera(float _LocationX, float _LocationY);

	virtual void UnlockCamera();


	enum AnimationEnum { Idle, Running, Jumping, Crouching, Death};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMovement)
		float Speed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CustomMovement)
		float Rotation;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = CustomMovement)
		float DeltaTime;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = CustomMovement)
		float LocationX;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = CustomMovement)
		float Gravity;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = CustomMovement)
		bool Grounded;

	FString Name;
	float OldSpeed;
	bool CanMove;
	bool CanJump;
	bool CanAttack;
	bool IsStunned;
	bool CanBePossessed;
	bool IsHurting;
	bool InEvent;
	bool CanRezz;
	bool CanSave;
	bool OnSemiPlatform;
	EVerticality Verticality;
	unsigned char Animation;
	int InterpolationN;
	int TimesPerSecond;
	int PlatformID;
	int PlayerRoom;
	int PingID;
	float SendTimeCorrected;
	float PSendTime;
	float HurtingTime;
	float LastMonsterNotificationSent;
	int32  BaseHealth;
	int32  MaxHealth;
	int32  Health;
	int32  BaseMana;
	int32  Mana;
	int32  MaxMana;
	float PreJumpTime;
	float AttackCooldown;
	float DefaultAttackCooldown;
	float CurrentAttackCooldown;
	float JumpingTime;
	float FollowJumpingTime;
	float SendTime;
	float BaseDamage;
	float Damage;
	float BaseDefense;
	float Defense;
	float T;
	float InterpolationTime;
	float BoxX;
	float BottomBorder;
	int32 CurrentFrame;
	FVector OldPosition;
	FVector NewPosition;
	FVector LastPositionSent;
	FVector OriginalPosition;
	FVector StandingHitboxExtent;
	FVector CrouchingHitboxExtent;
	FVector SpriteStandingPosition;
	FVector SpriteCrouchingPosition;
	FVector FollowJumpingPosition;
	float FollowJumpSpeed;
	unsigned char LastAnimationSent;
	FScriptDelegate FlipbookDelegate;
	FScriptDelegate DeathFlipbookDelegate;
	FScriptDelegate NetworkFlipbookDelegate;
	
	TArray<ABaseBNSPawn*> OverlappedPawns;
	TArray<FVector> LastPosition;
	TArray<FVector> Vectors;
	TArray<bool> WasRight;
	TArray<bool> WasHurting;
	TArray<FVector> AttackingVectors;
	TArray<unsigned char> Animations;
	TArray<unsigned char> AttackingAnimations;
	TArray<DebuffData> CurrentDebuffs;
	TArray<TSharedPtr<FSpellData>> Tier1Spells;
	TArray<TSharedPtr<FSpellData>> Tier2Spells;
	TArray<TSharedPtr<FSpellData>> Tier3Spells;
	TArray<TSharedPtr<FSpellData>> Tier4Spells;

	TArray<TSharedPtr<FSpellData>> EquippedTier1Spells;
	TArray<TSharedPtr<FSpellData>> EquippedTier2Spells;
	TArray<TSharedPtr<FSpellData>> EquippedTier3Spells;
	TArray<TSharedPtr<FSpellData>> EquippedTier4Spells;

	TArray<TSharedPtr<FSpellData>> SpellBook;

	TArray<TSharedPtr<FItemData>> InventoryItems;

	TSharedPtr<FItemData> EquippedWeapon;
	TSharedPtr<FItemData> EquippedArmor;
	TSharedPtr<FItemData> EquippedAccessory;

	float Money;

	int SelectedTier1Spell;
	int SelectedTier2Spell;
	int SelectedTier3Spell;
	int SelectedTier4Spell;

	int SelectedTier;
	int Tier1Charges;
	int Tier2Charges;
	int Tier3Charges;
	int Tier4Charges;
	int Tier1MaxCharges;
	int Tier2MaxCharges;
	int Tier3MaxCharges;
	int Tier4MaxCharges;
	int Tier1Slots;
	int Tier2Slots;
	int Tier3Slots;
	int Tier4Slots;
	//uint16_t MPCounter;
	//uint16_t NewMPCounter;
	SpellEffects* SpellEffect;
	ItemEffects* ItemEffect;
	bool InterpolatingMonster;

	bool WannaJump;
	bool HasAnimationChanged;
	bool IsAttacking;
	bool IsCrouching; 
	bool IsJumping;
	bool IsPossessed;
	bool Interpolating;
	bool FirstPosition;
	bool LastWasRight;
	bool IsInMenu;
	bool IsStatsVisible;
	bool IsCasting;
	bool IsDead;
	bool IsRezzing;

	//AI
	bool HasPath;
	bool HasVectorPath;
	TArray<bool> HasToJump;
	bool IsAIControlled;
	bool IsLocallyControlled;
	bool SeesPlayer;
	bool NewPath;

	uint8_t ChasingEnemy;
	float ChaseTime;
	float PauseTime;
	float BodyDamage;


	TArray<int> PlatformPath;
	TArray<FVector> VectorPath;
	int FPSIterator;
	float FPS;
	int PingIterator;
	float PingTime;
	float PingCooldown;
	bool PingSent;
	float PossessedMS;

	TArray<class ABaseBNSPawn*> DamagedPawns;
	uint16_t MonsterPCounter;
	ABaseBNSPawn* OtherPawn;
	uint8_t PawnClassID;
	AItemManager* ItemManager;
	class UDamageWidget* WidgetObject;
	class ABaseBNSPawn* PossessingPawn;
	class UBoneNSoulGameInstance* GameInstance;
	class ABoneNSoulGameSession* GameSession;
	class ABoneNSoulHUD* BNSHUD;
	UInputComponent* IC;
	UMaterialInstanceDynamic* DynamicMaterial;
	float MaterialColor;
};
