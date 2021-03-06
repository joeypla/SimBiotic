// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"

AShooterCharacter::AShooterCharacter(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UShooterCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	FirstPersonCameraComponent = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->AttachParent = GetCapsuleComponent();
	// Position the camera a bit above the eyes
	FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 50.0f + BaseEyeHeight);
	// Allow the pawn to control rotation.
	FirstPersonCameraComponent->bUseControllerViewRotation = true;

	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("PawnMesh1P"));
	Mesh1P->AttachParent = FirstPersonCameraComponent;
	Mesh1P->bOnlyOwnerSee = true;
	Mesh1P->bOwnerNoSee = false;
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh1P->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	Mesh1P->bChartDistanceFactor = false;
	Mesh1P->SetCollisionObjectType(ECC_Pawn);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);

	GetMesh()->bOnlyOwnerSee = false;
	GetMesh()->bOwnerNoSee = true;
	GetMesh()->bReceivesDecals = false;
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	TargetingSpeedModifier = 0.5f;
	bIsTargeting = false;
	RunningSpeedModifier = 1.5f;
	bWantsToRun = false;
	bWantsToFire = false;
	LowHealthPercentage = 0.5f;

	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	GravityDirection = FVector(0.f, 0.f, -1.0f);
	GravityMode = GRAVITY_ZNEGATIVE;

	IsBot = false;
}

FVector FlattenVector(FVector inVector, SBGravityMode GravityMode) {
	float Tolerance = SMALL_NUMBER;
	float X, Y, Z;
	X = inVector.X;
	Y = inVector.Y;
	Z = inVector.Z;


	float Scale;
	float SquareSum;
	switch (GravityMode)
	{
	case GRAVITY_XNEGATIVE:
	case GRAVITY_XPOSITIVE:
		SquareSum = Y*Y + Z*Z;

		// Not sure if it's safe to add tolerance in there. Might introduce too many errors
		if (SquareSum == 1.f)
		{
			if (X == 0.f)
			{
				return FVector(X, Y, Z);
			}
			else
			{
				return FVector(0.f, Y, Z);
			}
		}
		else if (SquareSum < Tolerance)
		{
			return FVector::ZeroVector;
		}

		Scale = FMath::InvSqrt(SquareSum);
		return FVector(0.f, Y*Scale, Z*Scale);
		break;
	case GRAVITY_YNEGATIVE:
	case GRAVITY_YPOSITIVE:
		SquareSum = X*X + Z*Z;

		// Not sure if it's safe to add tolerance in there. Might introduce too many errors
		if (SquareSum == 1.f)
		{
			if (Y == 0.f)
			{
				return FVector(X, Y, Z);
			}
			else
			{
				return FVector(X, 0.f, Z);
			}
		}
		else if (SquareSum < Tolerance)
		{
			return FVector::ZeroVector;
		}

		Scale = FMath::InvSqrt(SquareSum);
		return FVector(X*Scale, 0.f, Z*Scale);
		break;
	case GRAVITY_ZNEGATIVE:
	case GRAVITY_ZPOSITIVE:
		SquareSum = X*X + Y*Y;

		// Not sure if it's safe to add tolerance in there. Might introduce too many errors
		if (SquareSum == 1.f)
		{
			if (Z == 0.f)
			{
				return FVector(X, Y, Z);
			}
			else
			{
				return FVector(X, Y, 0.f);
			}
		}
		else if (SquareSum < Tolerance)
		{
			return FVector::ZeroVector;
		}

		Scale = FMath::InvSqrt(SquareSum);
		return FVector(X*Scale, Y*Scale, 0.f);
		break;
	default:
		break;
	}

	//const float SquareSum = X*X + Y*Y;

	//// Not sure if it's safe to add tolerance in there. Might introduce too many errors
	//if (SquareSum == 1.f)
	//{
	//	if (Z == 0.f)
	//	{
	//		return FVector(X, Y, Z);
	//	}
	//	else
	//	{
	//		return FVector(X, Y, 0.f);
	//	}
	//}
	//else if (SquareSum < Tolerance)
	//{
	//	return FVector::ZeroVector;
	//}

	//const float Scale = FMath::InvSqrt(SquareSum);
	//return FVector(X*Scale, Y*Scale, 0.f);
	return FVector(0.0f, 0.0f, 0.0f);
}
void AShooterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Role == ROLE_Authority)
	{
		Health = GetMaxHealth();
		SpawnDefaultInventory();
	}

	// set initial mesh visibility (3rd person view)
	UpdatePawnMeshes();
	
	// create material instance for setting team colors (3rd person view)
	for (int32 iMat = 0; iMat < GetMesh()->GetNumMaterials(); iMat++)
	{
		MeshMIDs.Add(GetMesh()->CreateAndSetMaterialInstanceDynamic(iMat));
	}

	// play respawn effects
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (RespawnFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(this, RespawnFX, GetActorLocation(), GetActorRotation());
		}

		if (RespawnSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, RespawnSound, GetActorLocation());
		}
	}
}

void AShooterCharacter::Destroyed()
{
	Super::Destroyed();
	DestroyInventory();
}

void AShooterCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// switch mesh to 1st person view
	UpdatePawnMeshes();

	// reattach weapon if needed
	SetCurrentWeapon(CurrentWeapon);

	// set team colors for 1st person view
	UMaterialInstanceDynamic* Mesh1PMID = Mesh1P->CreateAndSetMaterialInstanceDynamic(0);
	UpdateTeamColors(Mesh1PMID);
	//Set Gravity back to Z down
	if (Controller) {
		AShooterPlayerController* PC = Cast<AShooterPlayerController>(Controller);
		PC->SetGravityMode(GRAVITY_ZNEGATIVE);
	}
}

void AShooterCharacter::PossessedBy(class AController* InController)
{
	Super::PossessedBy(InController);

	// [server] as soon as PlayerState is assigned, set team colors of this pawn for local player
	UpdateTeamColorsAllMIDs();
}

void AShooterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// [client] as soon as PlayerState is assigned, set team colors of this pawn for local player
	if (PlayerState != NULL)
	{
		UpdateTeamColorsAllMIDs();
	}
}

FRotator AShooterCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}

bool AShooterCharacter::IsEnemyFor(AController* TestPC) const
{
	if (TestPC == Controller || TestPC == NULL)
	{
		return false;
	}

	AShooterPlayerState* TestPlayerState = Cast<AShooterPlayerState>(TestPC->PlayerState);
	AShooterPlayerState* MyPlayerState = Cast<AShooterPlayerState>(PlayerState);

	bool bIsEnemy = true;
	if (GetWorld()->GameState && GetWorld()->GameState->GameModeClass)
	{
		const AShooterGameMode* DefGame = GetWorld()->GameState->GameModeClass->GetDefaultObject<AShooterGameMode>();
		if (DefGame && MyPlayerState && TestPlayerState)
		{
			bIsEnemy = DefGame->CanDealDamage(TestPlayerState, MyPlayerState);
		}
	}

	return bIsEnemy;
}


//////////////////////////////////////////////////////////////////////////
// Meshes

void AShooterCharacter::UpdatePawnMeshes()
{
	bool const bFirstPerson = IsFirstPerson();

	Mesh1P->MeshComponentUpdateFlag = !bFirstPerson ? EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered : EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	Mesh1P->SetOwnerNoSee(!bFirstPerson);

	GetMesh()->MeshComponentUpdateFlag = bFirstPerson ? EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered : EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	GetMesh()->SetOwnerNoSee(bFirstPerson);
}

void AShooterCharacter::UpdateTeamColors(UMaterialInstanceDynamic* UseMID)
{
	if (UseMID)
	{
		AShooterPlayerState* MyPlayerState = Cast<AShooterPlayerState>(PlayerState);
		if (MyPlayerState != NULL)
		{
			float MaterialParam = (float)MyPlayerState->GetTeamNum();
			UseMID->SetScalarParameterValue(TEXT("Team Color Index"), MaterialParam);
		}
	}
}

void AShooterCharacter::OnCameraUpdate(const FVector& CameraLocation, const FRotator& CameraRotation)
{

	return;
	USkeletalMeshComponent* DefMesh1P = Cast<USkeletalMeshComponent>(GetClass()->GetDefaultSubobjectByName(TEXT("PawnMesh1P")));
	const FMatrix DefMeshLS = FRotationTranslationMatrix(DefMesh1P->RelativeRotation, DefMesh1P->RelativeLocation);
	const FMatrix LocalToWorld = ActorToWorld().ToMatrixWithScale();

	// Mesh rotating code expect uniform scale in LocalToWorld matrix

	const FRotator RotCameraPitch(CameraRotation.Pitch, 0.0f, 0.0f);
	const FRotator RotCameraYaw(0.0f, CameraRotation.Yaw, 0.0f);

	const FMatrix LeveledCameraLS = FRotationTranslationMatrix(RotCameraYaw, CameraLocation) * LocalToWorld.Inverse();
	const FMatrix PitchedCameraLS = FRotationMatrix(RotCameraPitch) * LeveledCameraLS;
	const FMatrix MeshRelativeToCamera = DefMeshLS * LeveledCameraLS.Inverse();
	const FMatrix PitchedMesh = MeshRelativeToCamera * PitchedCameraLS;

	//Mesh1P->SetRelativeLocationAndRotation(CameraLocation/*PitchedMesh.GetOrigin()*/, CameraRotation/*CameraRotation*//*PitchedMesh.Rotator()*/);
	//Mesh1P->AttachParent = FirstPersonCameraComponent;
	//Mesh1P->SetRelativeLocation(FVector(-9.0f, 0.0f, -150.0f));
	//Mesh1P->SetWorldRotation(Controller->GetControlRotation());
}


//////////////////////////////////////////////////////////////////////////
// Damage & death


void AShooterCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	Die(Health, FDamageEvent(dmgType.GetClass()), NULL, NULL);
}

void AShooterCharacter::Suicide()
{
	KilledBy(this);
}

void AShooterCharacter::KilledBy(APawn* EventInstigator)
{
	if (Role == ROLE_Authority && !bIsDying)
	{
		AController* Killer = NULL;
		if (EventInstigator != NULL)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = NULL;
		}

		Die(Health, FDamageEvent(UDamageType::StaticClass()), Killer, NULL);
	}
}


float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->HasGodMode())
	{
		return 0.f;
	}

	if (Health <= 0.f)
	{
		return 0.f;
	}

	// Modify based on game rules.
	AShooterGameMode* const Game = GetWorld()->GetAuthGameMode<AShooterGameMode>();
	Damage = Game ? Game->ModifyDamage(Damage, this, DamageEvent, EventInstigator, DamageCauser) : 0.f;

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage > 0.f)
	{
		Health -= ActualDamage;
		if (Health <= 0)
		{
			Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
		}
		else
		{
			PlayHit(ActualDamage, DamageEvent, EventInstigator ? EventInstigator->GetPawn() : NULL, DamageCauser);
		}

		MakeNoise(1.0f, EventInstigator ? EventInstigator->GetPawn() : this);
	}

	return ActualDamage;
}


bool AShooterCharacter::CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const
{
	if ( bIsDying										// already dying
		|| IsPendingKill()								// already destroyed
		|| Role != ROLE_Authority						// not authority
		|| GetWorld()->GetAuthGameMode() == NULL
		|| GetWorld()->GetAuthGameMode()->GetMatchState() == MatchState::LeavingMap)	// level transition occurring
	{
		return false;
	}

	return true;
}


bool AShooterCharacter::Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser)
{
	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
	{
		return false;
	}

	Health = FMath::Min(0.0f, Health);

	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	AController* const KilledPlayer = (Controller != NULL) ? Controller : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<AShooterGameMode>()->Killed(Killer, KilledPlayer, this, DamageType);

	NetUpdateFrequency = GetDefault<AShooterCharacter>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : NULL, DamageCauser);
	return true;
}


void AShooterCharacter::OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (bIsDying)
	{
		return;
	}

	bReplicateMovement = false;
	bTearOff = true;
	bIsDying = true;

	if (Role == ROLE_Authority)
	{
		ReplicateHit(KillingDamage, DamageEvent, PawnInstigator, DamageCauser, true);	

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			UShooterDamageType *DamageType = Cast<UShooterDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->KilledForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageType->KilledForceFeedback, false, "Damage");
			}
		}
	}

	// cannot use IsLocallyControlled here, because even local client's controller may be NULL here
	if (GetNetMode() != NM_DedicatedServer && DeathSound && Mesh1P && Mesh1P->IsVisible())
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	// remove all weapons
	DestroyInventory();
	
	// switch back to 3rd person view
	UpdatePawnMeshes();

	DetachFromControllerPendingDestroy();
	StopAllAnimMontages();

	if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
	{
		LowHealthWarningPlayer->Stop();
	}

	if (RunLoopAC)
	{
		RunLoopAC->Stop();
	}

	// disable collisions on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

	if (GetMesh())
	{
		static FName CollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionProfileName(CollisionProfileName);
	}
	SetActorEnableCollision(true);

	// Death anim
	float DeathAnimDuration = PlayAnimMontage(DeathAnim);

	// Ragdoll
	if (DeathAnimDuration > 0.f)
	{
		GetWorldTimerManager().SetTimer(this, &AShooterCharacter::SetRagdollPhysics, FMath::Min(0.1f, DeathAnimDuration), false);
	}
	else
	{
		SetRagdollPhysics();
	}
}

void AShooterCharacter::PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (Role == ROLE_Authority)
	{
		ReplicateHit(DamageTaken, DamageEvent, PawnInstigator, DamageCauser, false);

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			UShooterDamageType *DamageType = Cast<UShooterDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->HitForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageType->HitForceFeedback, false, "Damage");
			}
		}
	}

	if (DamageTaken > 0.f)
	{
		ApplyDamageMomentum(DamageTaken, DamageEvent, PawnInstigator, DamageCauser);
	}
	
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	AShooterHUD* MyHUD = MyPC ? Cast<AShooterHUD>(MyPC->GetHUD()) : NULL;
	if (MyHUD)
	{
		MyHUD->NotifyHit(DamageTaken, DamageEvent, PawnInstigator);
	}

	if (PawnInstigator && PawnInstigator != this && PawnInstigator->IsLocallyControlled())
	{
		AShooterPlayerController* InstigatorPC = Cast<AShooterPlayerController>(PawnInstigator->Controller);
		AShooterHUD* InstigatorHUD = InstigatorPC ? Cast<AShooterHUD>(InstigatorPC->GetHUD()) : NULL;
		if (InstigatorHUD)
		{
			InstigatorHUD->NotifyEnemyHit();
		}
	}
}


void AShooterCharacter::SetRagdollPhysics()
{
	bool bInRagdoll = false;

	if (IsPendingKill())
	{
		bInRagdoll = false;
	}
	else if (!GetMesh() || !GetMesh()->GetPhysicsAsset())
	{
		bInRagdoll = false;
	}
	else
	{
		// initialize physics/etc
		GetMesh()->SetAllBodiesSimulatePhysics(true);
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		bInRagdoll = true;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		SetLifeSpan( 1.0f );
	}
	else
	{
		SetLifeSpan( 10.0f );
	}
}



void AShooterCharacter::ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FDamageEvent const& LastDamageEvent = LastTakeHitInfo.GetDamageEvent();
	if ((PawnInstigator == LastTakeHitInfo.PawnInstigator.Get()) && (LastDamageEvent.DamageTypeClass == LastTakeHitInfo.DamageTypeClass) && (LastTakeHitTimeTimeout == TimeoutTime))
	{
		// same frame damage
		if (bKilled && LastTakeHitInfo.bKilled)
		{
			// Redundant death take hit, just ignore it
			return;
		}

		// otherwise, accumulate damage done this frame
		Damage += LastTakeHitInfo.ActualDamage;
	}

	LastTakeHitInfo.ActualDamage = Damage;
	LastTakeHitInfo.PawnInstigator = Cast<AShooterCharacter>(PawnInstigator);
	LastTakeHitInfo.DamageCauser = DamageCauser;
	LastTakeHitInfo.SetDamageEvent(DamageEvent);		
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();

	LastTakeHitTimeTimeout = TimeoutTime;
}

void AShooterCharacter::OnRep_LastTakeHitInfo()
{
	if (LastTakeHitInfo.bKilled)
	{
		OnDeath(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
	else
	{
		PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
}

//Pawn::PlayDying sets this lifespan, but when that function is called on client, dead pawn's role is still SimulatedProxy despite bTearOff being true. 
void AShooterCharacter::TornOff()
{
	SetLifeSpan(25.f);
}


//////////////////////////////////////////////////////////////////////////
// Inventory

void AShooterCharacter::SpawnDefaultInventory()
{
	if (Role < ROLE_Authority)
	{
		return;
	}

	int32 NumWeaponClasses = DefaultInventoryClasses.Num();	
	for (int32 i = 0; i < NumWeaponClasses; i++)
	{
		if (DefaultInventoryClasses[i])
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.bNoCollisionFail = true;
			AShooterWeapon* NewWeapon = GetWorld()->SpawnActor<AShooterWeapon>(DefaultInventoryClasses[i], SpawnInfo);
			AddWeapon(NewWeapon);
		}
	}

	// equip first weapon in inventory
	if (Inventory.Num() > 0)
	{
		EquipWeapon(Inventory[0]);
	}
}

void AShooterCharacter::DestroyInventory()
{
	if (Role < ROLE_Authority)
	{
		return;
	}

	// remove all weapons from inventory and destroy them
	for (int32 i = Inventory.Num() - 1; i >= 0; i--)
	{
		AShooterWeapon* Weapon = Inventory[i];
		if (Weapon)
		{
			RemoveWeapon(Weapon);
			Weapon->Destroy();
		}
	}
}

void AShooterCharacter::AddWeapon(AShooterWeapon* Weapon)
{
	if (Weapon && Role == ROLE_Authority)
	{
		Weapon->OnEnterInventory(this);
		Inventory.AddUnique(Weapon);
	}
}

void AShooterCharacter::RemoveWeapon(AShooterWeapon* Weapon)
{
	if (Weapon && Role == ROLE_Authority)
	{
		Weapon->OnLeaveInventory();
		Inventory.RemoveSingle(Weapon);
	}
}

AShooterWeapon* AShooterCharacter::FindWeapon(TSubclassOf<AShooterWeapon> WeaponClass)
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i] && Inventory[i]->IsA(WeaponClass))
		{
			return Inventory[i];
		}
	}

	return NULL;
}

void AShooterCharacter::EquipWeapon(AShooterWeapon* Weapon)
{
	if (Weapon)
	{
		if (Role == ROLE_Authority)
		{
			SetCurrentWeapon(Weapon);
		}
		else
		{
			ServerEquipWeapon(Weapon);
		}
	}
}

bool AShooterCharacter::ServerEquipWeapon_Validate(AShooterWeapon* Weapon)
{
	return true;
}

void AShooterCharacter::ServerEquipWeapon_Implementation(AShooterWeapon* Weapon)
{
	EquipWeapon(Weapon);
}

void AShooterCharacter::OnRep_CurrentWeapon(AShooterWeapon* LastWeapon)
{
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void AShooterCharacter::SetCurrentWeapon(class AShooterWeapon* NewWeapon, class AShooterWeapon* LastWeapon)
{
	AShooterWeapon* LocalLastWeapon = NULL;
	
	if (LastWeapon != NULL)
	{
		LocalLastWeapon = LastWeapon;
	}
	else if (NewWeapon != CurrentWeapon)
	{
		LocalLastWeapon = CurrentWeapon;
	}

	// unequip previous
	if (LocalLastWeapon)
	{
		LocalLastWeapon->OnUnEquip();
	}

	CurrentWeapon = NewWeapon;

	// equip new one
	if (NewWeapon)
	{
		NewWeapon->SetOwningPawn(this);	// Make sure weapon's MyPawn is pointing back to us. During replication, we can't guarantee APawn::CurrentWeapon will rep after AWeapon::MyPawn!
		NewWeapon->OnEquip();
	}
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

void AShooterCharacter::StartWeaponFire()
{
	if (!bWantsToFire)
	{
		bWantsToFire = true;
		if (CurrentWeapon)
		{
			CurrentWeapon->StartFire();
		}
	}
}

void AShooterCharacter::StopWeaponFire()
{
	if (bWantsToFire)
	{
		bWantsToFire = false;
		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
		}
	}
}

bool AShooterCharacter::CanFire() const
{
	return IsAlive();
}

bool AShooterCharacter::CanReload() const
{
	return true;
}

void AShooterCharacter::SetTargeting(bool bNewTargeting)
{
	bIsTargeting = bNewTargeting;

	if (TargetingSound)
	{
		UGameplayStatics::PlaySoundAttached(TargetingSound, GetRootComponent());
	}

	if (Role < ROLE_Authority)
	{
		ServerSetTargeting(bNewTargeting);
	}
}

bool AShooterCharacter::ServerSetTargeting_Validate(bool bNewTargeting)
{
	return true;
}

void AShooterCharacter::ServerSetTargeting_Implementation(bool bNewTargeting)
{
	SetTargeting(bNewTargeting);
}

//////////////////////////////////////////////////////////////////////////
// Movement

void AShooterCharacter::SetRunning(bool bNewRunning, bool bToggle)
{
	bWantsToRun = bNewRunning;
	bWantsToRunToggled = bNewRunning && bToggle;

	if (Role < ROLE_Authority)
	{
		ServerSetRunning(bNewRunning, bToggle);
	}

	UpdateRunSounds(bNewRunning);
}

bool AShooterCharacter::ServerSetRunning_Validate(bool bNewRunning, bool bToggle)
{
	return true;
}

void AShooterCharacter::ServerSetRunning_Implementation(bool bNewRunning, bool bToggle)
{
	SetRunning(bNewRunning, bToggle);
}

void AShooterCharacter::UpdateRunSounds(bool bNewRunning)
{
	if (bNewRunning)
	{
		if (!RunLoopAC && RunLoopSound)
		{
			RunLoopAC = UGameplayStatics::PlaySoundAttached(RunLoopSound, GetRootComponent());
			if (RunLoopAC)
			{
				RunLoopAC->bAutoDestroy = false;
			}
			
		}
		else if (RunLoopAC)
		{
			RunLoopAC->Play();
		}
	}
	else
	{
		if (RunLoopAC)
		{
			RunLoopAC->Stop();
		}

		if (RunStopSound)
		{
			UGameplayStatics::PlaySoundAttached(RunStopSound, GetRootComponent());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Animations

float AShooterCharacter::PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate, FName StartSectionName) 
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance)
	{
		return UseMesh->AnimScriptInstance->Montage_Play(AnimMontage, InPlayRate);
	}

	return 0.0f;
}

void AShooterCharacter::StopAnimMontage(class UAnimMontage* AnimMontage)
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance &&
		UseMesh->AnimScriptInstance->Montage_IsPlaying(AnimMontage))
	{
		UseMesh->AnimScriptInstance->Montage_Stop(AnimMontage->BlendOutTime);
	}
}

void AShooterCharacter::StopAllAnimMontages()
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (UseMesh && UseMesh->AnimScriptInstance)
	{
		UseMesh->AnimScriptInstance->Montage_Stop(0.0f);
	}
}


//////////////////////////////////////////////////////////////////////////
// Input

void AShooterCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	check(InputComponent);
	InputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	InputComponent->BindAxis("MoveUp", this, &AShooterCharacter::MoveUp);
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);

	InputComponent->BindAction("Fire", IE_Pressed, this, &AShooterCharacter::OnStartFire);
	InputComponent->BindAction("Fire", IE_Released, this, &AShooterCharacter::OnStopFire);

	InputComponent->BindAction("Targeting", IE_Pressed, this, &AShooterCharacter::OnStartTargeting);
	InputComponent->BindAction("Targeting", IE_Released, this, &AShooterCharacter::OnStopTargeting);

	InputComponent->BindAction("NextWeapon", IE_Pressed, this, &AShooterCharacter::OnNextWeapon);
	InputComponent->BindAction("PrevWeapon", IE_Pressed, this, &AShooterCharacter::OnPrevWeapon);

	InputComponent->BindAction("Reload", IE_Pressed, this, &AShooterCharacter::OnReload);

	InputComponent->BindAction("Jump", IE_Pressed, this, &AShooterCharacter::OnStartJump);
	InputComponent->BindAction("Jump", IE_Released, this, &AShooterCharacter::OnStopJump);

	InputComponent->BindAction("Run", IE_Pressed, this, &AShooterCharacter::OnStartRunning);
	InputComponent->BindAction("RunToggle", IE_Pressed, this, &AShooterCharacter::OnStartRunningToggle);
	InputComponent->BindAction("Run", IE_Released, this, &AShooterCharacter::OnStopRunning);

	InputComponent->BindAction("GravityLeft", IE_Pressed, this, &AShooterCharacter::OnGravityLeft);
	InputComponent->BindAction("GravityRight", IE_Pressed, this, &AShooterCharacter::OnGravityRight);
	InputComponent->BindAction("GravityForward", IE_Pressed, this, &AShooterCharacter::OnGravityForward);
}


void AShooterCharacter::MoveForward(float Val)
{
	if (Controller && Val != 0.f)
	{
		// Limit pitch when walking or falling
		const bool bLimitRotation = (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling());
		const FRotator Rotation = bLimitRotation ? GetActorRotation() : Controller->GetControlRotation();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis( EAxis::X );
		AddMovementInput(Direction, Val);
	}
}

void AShooterCharacter::MoveRight(float Val)
{
	if (Val != 0.f)
	{
		const FRotator Rotation = GetActorRotation();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis( EAxis::Y );
		AddMovementInput(Direction, Val);
	}
}

void AShooterCharacter::MoveUp(float Val)
{
	if (Val != 0.f)
	{
		// Not when walking or falling.
		if (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling())
		{
			return;
		}

		AddMovementInput(FVector::UpVector, Val);
	}
}

void AShooterCharacter::TurnAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	
	AddControllerYawInput(Val * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookUpAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	float modVal = Val;
	if (bIsTargeting) modVal *= 0.5f;
	AddControllerPitchInput(Val * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::AddControllerYawInput(float Val)
{
	float modVal = Val;
	if (bIsTargeting) modVal *= 0.5f;

	if (IsGravityLefting || IsGravityRighting || IsGravityForwarding) return;

	if (Controller && Controller->IsLocalPlayerController())
	{
		APlayerController* const PC = CastChecked<APlayerController>(Controller);

		//We need to add input that is equivalent to pitch for the current gravity configuration
		SBGravityMode GravityMode = GetGravityMode();

		FVector rotationAxis;
		rotationAxis.X = 0.0f;
		rotationAxis.Y = 0.0f;
		rotationAxis.Z = 0.0f; // rotation axis needs to be dependant on gravity.
		// we need to find the up world axis for this gravity configuration.
		switch (GravityMode)
		{
		case GRAVITY_XNEGATIVE:
			rotationAxis.X = 1.0f;
			break;
		case GRAVITY_XPOSITIVE:
			rotationAxis.X = -1.0f;
			break;
		case GRAVITY_YNEGATIVE:
			rotationAxis.Y = 1.0f;
			break;
		case GRAVITY_YPOSITIVE:
			rotationAxis.Y = -1.0f;
			break;
		case GRAVITY_ZNEGATIVE:
			rotationAxis.Z = 1.0f;
			break;
		case GRAVITY_ZPOSITIVE:
			rotationAxis.Z = -1.0f;
			break;
		default:
			break;
		}
		FRotator oldRotation = PC->GetControlRotation();
		// Instead of changing the yaw of the rotation, we need to find a way to take the current basis and rotate it around an arbitrary axis.
		FQuat oldQuaternion = FQuat(oldRotation);
		FQuat modQuaternion = FQuat(rotationAxis, modVal * 3.14159 / 180.0f);

		FQuat newQuat = modQuaternion * oldQuaternion;
		FRotator newRotation = newQuat.Rotator();

		PC->SetControlRotation(newRotation);
	}
}

void AShooterCharacter::AddControllerPitchInput(float Val)
{

	float modVal = Val;
	if (bIsTargeting) modVal *= 0.5f;
	if (IsGravityLefting || IsGravityRighting || IsGravityForwarding) return;

	if (Controller && Controller->IsLocalPlayerController())
	{
		APlayerController* const PC = CastChecked<APlayerController>(Controller);

		//We need to add input that is equivalent to pitch for the current gravity configuration
		SBGravityMode GravityMode = GetGravityMode();

		FRotator oldRotation = PC->GetControlRotation();
		FVector strafeAxis = FRotationMatrix(oldRotation).GetUnitAxis(EAxis::Y);
		FQuat oldQuaternion = FQuat(oldRotation);
		FQuat modQuaternion = FQuat(strafeAxis, modVal * 3.14159 / 180.0f);
		FQuat newQuat = modQuaternion * oldQuaternion;
		FRotator newRotation = newQuat.Rotator();

		PC->SetControlRotation(newRotation);
	}
}

void AShooterCharacter::OnStartFire()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		StartWeaponFire();
	}
}

void AShooterCharacter::OnStopFire()
{
	StopWeaponFire();
}

void AShooterCharacter::OnStartTargeting()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		SetTargeting(true);
	}
}

void AShooterCharacter::OnStopTargeting()
{
	SetTargeting(false);
}

void AShooterCharacter::OnNextWeapon()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2 && (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping))
		{
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			AShooterWeapon* NextWeapon = Inventory[(CurrentWeaponIdx + 1) % Inventory.Num()];
			EquipWeapon(NextWeapon);
		}
	}
}

void AShooterCharacter::OnPrevWeapon()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2 && (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping))
		{
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			AShooterWeapon* PrevWeapon = Inventory[(CurrentWeaponIdx - 1 + Inventory.Num()) % Inventory.Num()];
			EquipWeapon(PrevWeapon);
		}
	}
}

void AShooterCharacter::OnReload()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->StartReload();
		}
	}
}

void AShooterCharacter::OnStartRunning()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsTargeting())
		{
			SetTargeting(false);
		}
		StopWeaponFire();
		SetRunning(true, false);
	}
}

void AShooterCharacter::OnStartRunningToggle()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsTargeting())
		{
			SetTargeting(false);
		}
		StopWeaponFire();
		SetRunning(true, true);
	}
}

void AShooterCharacter::OnStopRunning()
{
	SetRunning(false, false);
}

bool AShooterCharacter::IsRunning() const
{	
	if (!GetCharacterMovement())
	{
		return false;
	}
	
	return (bWantsToRun || bWantsToRunToggled) && !GetVelocity().IsZero() && (GetVelocity().SafeNormal2D() | GetActorRotation().Vector()) > -0.1;
}

void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bWantsToRunToggled && !IsRunning())
	{
		SetRunning(false, false);
	}
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->HasHealthRegen())
	{
		if (this->Health < this->GetMaxHealth())
		{
			this->Health +=  5 * DeltaSeconds;
			if (Health > this->GetMaxHealth())
			{
				Health = this->GetMaxHealth();
			}
		}
	}
	
	if (LowHealthSound && GEngine->UseSound())
	{
		if ((this->Health > 0 && this->Health < this->GetMaxHealth() * LowHealthPercentage) && (!LowHealthWarningPlayer || !LowHealthWarningPlayer->IsPlaying()))
		{
			LowHealthWarningPlayer = UGameplayStatics::PlaySoundAttached(LowHealthSound, GetRootComponent(),
				NAME_None, FVector(ForceInit), EAttachLocation::KeepRelativeOffset, true);
			LowHealthWarningPlayer->SetVolumeMultiplier(0.0f);
		} 
		else if ((this->Health > this->GetMaxHealth() * LowHealthPercentage || this->Health < 0) && LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
		{
			LowHealthWarningPlayer->Stop();
		}
		if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
		{
			const float MinVolume = 0.3f;
			const float VolumeMultiplier = (1.0f - (this->Health / (this->GetMaxHealth() * LowHealthPercentage)));
			LowHealthWarningPlayer->SetVolumeMultiplier(MinVolume + (1.0f - MinVolume) * VolumeMultiplier);
		}
	}
	/*Gravity Stuff*/
	UShooterCharacterMovement* usbMovementComponent = Cast<UShooterCharacterMovement>(CharacterMovement);
	
	//if we have a controller then we set the controller from that, otherwise we use the actor gravity mode which is replicated

	AShooterPlayerController* PC = Cast<AShooterPlayerController>(Controller);
	if (PC) {
		GravityMode = PC->GravityMode;
		if (usbMovementComponent)
			usbMovementComponent->setGravityMode(PC->GravityMode);
	}
	else {
		if (usbMovementComponent)
			usbMovementComponent->setGravityMode(GravityMode);
	}
	

	if (IsGravityRighting)
	{
		if (Controller && Controller->IsLocalPlayerController())
		{
			APlayerController* const PC = CastChecked<APlayerController>(Controller);



			FRotator oldRotation = PC->GetControlRotation();

			float rotationAmount = gravityRotationModifier * 20.0f * DeltaSeconds;
			accumulatedGravityAngle += rotationAmount;

			if (accumulatedGravityAngle > 3.1415926535 / 2.0f) {
				rotationAmount -= (accumulatedGravityAngle - 3.1415926535 / 2.0f);
				IsGravityRighting = false;
				accumulatedGravityAngle = 0.f;
			}
			FQuat oldQuaternion = FQuat(oldRotation);
			FQuat modQuaternion = FQuat(currentGravityRotatingAxis, rotationAmount);
			FQuat newQuat = modQuaternion * oldQuaternion;
			FRotator newRotation = newQuat.Rotator();

			if (PC)
			PC->SetControlRotation(newRotation);
		}

	}

	if (IsGravityLefting)
	{
		if (Controller && Controller->IsLocalPlayerController())
		{
			APlayerController* const PC = CastChecked<APlayerController>(Controller);

			

			FRotator oldRotation = PC->GetControlRotation();

			float rotationAmount = gravityRotationModifier * 20.0f * DeltaSeconds;
			accumulatedGravityAngle += rotationAmount;

			if (accumulatedGravityAngle > 3.1415926535 / 2.0f) {
				rotationAmount -= (accumulatedGravityAngle - 3.1415926535 / 2.0f);
				IsGravityLefting = false;
				accumulatedGravityAngle = 0.f;
			}
			FQuat oldQuaternion = FQuat(oldRotation);
			FQuat modQuaternion = FQuat(currentGravityRotatingAxis, -rotationAmount);
			FQuat newQuat = modQuaternion * oldQuaternion;
			FRotator newRotation = newQuat.Rotator();

			if (PC)
			PC->SetControlRotation(newRotation);
		}
		
	}

	if (IsGravityForwarding)
	{
		if (Controller && Controller->IsLocalPlayerController())
		{
			APlayerController* const PC = CastChecked<APlayerController>(Controller);



			FRotator oldRotation = PC->GetControlRotation();

			float rotationAmount = gravityRotationModifier * 20.0f * DeltaSeconds;
			accumulatedGravityAngle += rotationAmount;

			if (accumulatedGravityAngle > 3.1415926535 / 2.0f) {
				rotationAmount -= (accumulatedGravityAngle - 3.1415926535 / 2.0f);
				IsGravityForwarding = false;
				accumulatedGravityAngle = 0.f;
			}
			FQuat oldQuaternion = FQuat(oldRotation);
			FQuat modQuaternion = FQuat(currentGravityRotatingAxis, -rotationAmount);
			FQuat newQuat = modQuaternion * oldQuaternion;
			FRotator newRotation = newQuat.Rotator();

			if (PC)
				PC->SetControlRotation(newRotation);
		}

	}
	/*
	if (PC) {
		SetFullControlRotation(PC->GetControlRotation());
	}
	else{
		if (!IsBot)
			SetActorRotation(FullControlRotation);
	}*/
}

void AShooterCharacter::OnStartJump()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		bPressedJump = true;
	}
}

void AShooterCharacter::OnStopJump()
{
	bPressedJump = false;
}

//////////////////////////////////////////////////////////////////////////
// Replication

void AShooterCharacter::PreReplication( IRepChangedPropertyTracker & ChangedPropertyTracker )
{
	Super::PreReplication( ChangedPropertyTracker );

	// Only replicate this property for a short duration after it changes so join in progress players don't get spammed with fx when joining late
	DOREPLIFETIME_ACTIVE_OVERRIDE( AShooterCharacter, LastTakeHitInfo, GetWorld() && GetWorld()->GetTimeSeconds() < LastTakeHitTimeTimeout );
}

void AShooterCharacter::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	// only to local owner: weapon change requests are locally instigated, other clients don't need it
	DOREPLIFETIME_CONDITION( AShooterCharacter, Inventory,			COND_OwnerOnly );

	// everyone except local owner: flag change is locally instigated
	DOREPLIFETIME_CONDITION( AShooterCharacter, bIsTargeting,		COND_SkipOwner );
	DOREPLIFETIME_CONDITION( AShooterCharacter, bWantsToRun,		COND_SkipOwner );

	DOREPLIFETIME_CONDITION( AShooterCharacter, LastTakeHitInfo,	COND_Custom );

	// everyone
	DOREPLIFETIME( AShooterCharacter, CurrentWeapon );
	DOREPLIFETIME( AShooterCharacter, Health );
	DOREPLIFETIME_CONDITION(AShooterCharacter, GravityMode, COND_SkipOwner);

	DOREPLIFETIME_CONDITION(AShooterCharacter, FullControlRotation, COND_SkipOwner);
}

AShooterWeapon* AShooterCharacter::GetWeapon() const
{
	return CurrentWeapon;
}

int32 AShooterCharacter::GetInventoryCount() const
{
	return Inventory.Num();
}

AShooterWeapon* AShooterCharacter::GetInventoryWeapon(int32 index) const
{
	return Inventory[index];
}

USkeletalMeshComponent* AShooterCharacter::GetPawnMesh() const
{
	return IsFirstPerson() ? Mesh1P : GetMesh();
}

USkeletalMeshComponent* AShooterCharacter::GetSpecifcPawnMesh( bool WantFirstPerson ) const
{
	return WantFirstPerson == true  ? Mesh1P : GetMesh();
}

FName AShooterCharacter::GetWeaponAttachPoint() const
{
	return WeaponAttachPoint;
}

float AShooterCharacter::GetTargetingSpeedModifier() const
{
	return TargetingSpeedModifier;
}

bool AShooterCharacter::IsTargeting() const
{
	return bIsTargeting;
}

float AShooterCharacter::GetRunningSpeedModifier() const
{
	return RunningSpeedModifier;
}

bool AShooterCharacter::IsFiring() const
{
	return bWantsToFire;
};

bool AShooterCharacter::IsFirstPerson() const
{
	return IsAlive() && Controller && Controller->IsLocalPlayerController();
}

int32 AShooterCharacter::GetMaxHealth() const
{
	return GetClass()->GetDefaultObject<AShooterCharacter>()->Health;
}

bool AShooterCharacter::IsAlive() const
{
	return Health > 0;
}

float AShooterCharacter::GetLowHealthPercentage() const
{
	return LowHealthPercentage;
}

void AShooterCharacter::UpdateTeamColorsAllMIDs()
{
	for (int32 i = 0; i < MeshMIDs.Num(); ++i)
	{
		UpdateTeamColors(MeshMIDs[i]);
	}
}

SBGravityMode AShooterCharacter::GetGravityMode(void)
{
	if (abs(GravityDirection.X) > abs(GravityDirection.Y) && abs(GravityDirection.X) > abs(GravityDirection.Z))
	{
		if (GravityDirection.X > 0.f)
			return GRAVITY_XPOSITIVE;
		else return GRAVITY_XNEGATIVE;
	}
	if (abs(GravityDirection.Y) > abs(GravityDirection.X) && abs(GravityDirection.Y) > abs(GravityDirection.Z))
	{
		if (GravityDirection.Y > 0.f)
			return GRAVITY_YPOSITIVE;
		else return GRAVITY_YNEGATIVE;
	}
	if (abs(GravityDirection.Z) > abs(GravityDirection.X) && abs(GravityDirection.Z) > abs(GravityDirection.Y))
	{
		if (GravityDirection.Z > 0.f)
			return GRAVITY_ZPOSITIVE;
		else GRAVITY_ZNEGATIVE;
	}

	return GRAVITY_ZNEGATIVE;
}


#include "SimbioticMath.h"
void AShooterCharacter::OnGravityLeft(void)
{
	if (IsGravityLefting || IsGravityRighting || IsGravityForwarding) return;
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Left Gravity Called"));


	FRotator rotation = Controller->GetControlRotation();
	FVector direction = FRotationMatrix(rotation).GetUnitAxis(EAxis::Y);
	FVector directionStraight = FRotationMatrix(rotation).GetUnitAxis(EAxis::X);
	switch (GetGravityMode()) {
	case SBGravityMode::GRAVITY_XNEGATIVE:
	case SBGravityMode::GRAVITY_XPOSITIVE:
		directionStraight.X = 0.f;
		break;
	case SBGravityMode::GRAVITY_YNEGATIVE:
	case SBGravityMode::GRAVITY_YPOSITIVE:
		directionStraight.Y = 0.f;
		break;
	case SBGravityMode::GRAVITY_ZNEGATIVE:
	case SBGravityMode::GRAVITY_ZPOSITIVE:
		directionStraight.Z = 0.f;
		break;
	}

	SimBioticMath::float3 vector;
	vector.x = -direction.X;
	vector.y = -direction.Y;
	vector.z = -direction.Z;

	SimBioticMath::float3 vectorStraight;
	vectorStraight.x = directionStraight.X;
	vectorStraight.y = directionStraight.Y;
	vectorStraight.z = directionStraight.Z;

	SimBioticMath::float3 unitVector = SimBioticMath::GetClosestUnitVector(vector);
	GravityDirection.X = unitVector.x;
	GravityDirection.Y = unitVector.y;
	GravityDirection.Z = unitVector.z;

	SimBioticMath::float3 unitVectorStraight = SimBioticMath::GetClosestUnitVector(vectorStraight);

	IsGravityLefting = true;
	accumulatedGravityAngle = 0.f;
	currentGravityRotatingAxis.X = unitVectorStraight.x;
	currentGravityRotatingAxis.Y = unitVectorStraight.y;
	currentGravityRotatingAxis.Z = unitVectorStraight.z;

	//if (GEngine)
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Gravity Direction is now: %f, %f, %f"), GravityDirection.X, GravityDirection.Y, GravityDirection.Z));

	if (Controller && Controller->IsLocalPlayerController()) {
		AShooterPlayerController* const PC = CastChecked<AShooterPlayerController>(Controller);
		PC->SetGravityMode(GetGravityMode());
	}
}

void AShooterCharacter::OnGravityRight(void)
{
	if (IsGravityLefting || IsGravityRighting || IsGravityForwarding) return;
	//if (GEngine)
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Right Gravity Called"));


	FRotator rotation = Controller->GetControlRotation();
	FVector direction = FRotationMatrix(rotation).GetUnitAxis(EAxis::Y);
	FVector directionStraight = FRotationMatrix(rotation).GetUnitAxis(EAxis::X);
	switch (GetGravityMode()) {
	case SBGravityMode::GRAVITY_XNEGATIVE:
	case SBGravityMode::GRAVITY_XPOSITIVE:
		directionStraight.X = 0.f;
		break;
	case SBGravityMode::GRAVITY_YNEGATIVE:
	case SBGravityMode::GRAVITY_YPOSITIVE:
		directionStraight.Y = 0.f;
		break;
	case SBGravityMode::GRAVITY_ZNEGATIVE:
	case SBGravityMode::GRAVITY_ZPOSITIVE:
		directionStraight.Z = 0.f;
		break;
	}

	SimBioticMath::float3 vector;
	vector.x = direction.X;
	vector.y = direction.Y;
	vector.z = direction.Z;

	SimBioticMath::float3 vectorStraight;
	vectorStraight.x = directionStraight.X;
	vectorStraight.y = directionStraight.Y;
	vectorStraight.z = directionStraight.Z;

	SimBioticMath::float3 unitVector = SimBioticMath::GetClosestUnitVector(vector);
	GravityDirection.X = unitVector.x;
	GravityDirection.Y = unitVector.y;
	GravityDirection.Z = unitVector.z;

	SimBioticMath::float3 unitVectorStraight = SimBioticMath::GetClosestUnitVector(vectorStraight);

	IsGravityRighting = true;
	accumulatedGravityAngle = 0.f;
	currentGravityRotatingAxis.X = unitVectorStraight.x;
	currentGravityRotatingAxis.Y = unitVectorStraight.y;
	currentGravityRotatingAxis.Z = unitVectorStraight.z;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Gravity Direction is now: %f, %f, %f"), GravityDirection.X, GravityDirection.Y, GravityDirection.Z));


	if (Controller && Controller->IsLocalPlayerController()) {
		AShooterPlayerController* const PC = CastChecked<AShooterPlayerController>(Controller);
		PC->SetGravityMode(GetGravityMode());

	}
}

void AShooterCharacter::OnGravityForward(void)
{
	if (IsGravityLefting || IsGravityRighting || IsGravityForwarding) return;
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Forward Gravity Called"));


	FRotator rotation = Controller->GetControlRotation();
	FVector direction = FRotationMatrix(rotation).GetUnitAxis(EAxis::X);
	FVector directionStrafe = FRotationMatrix(rotation).GetUnitAxis(EAxis::Y);
	switch (GetGravityMode()) {
	case SBGravityMode::GRAVITY_XNEGATIVE:
	case SBGravityMode::GRAVITY_XPOSITIVE:
		direction.X = 0.f;
		break;
	case SBGravityMode::GRAVITY_YNEGATIVE:
	case SBGravityMode::GRAVITY_YPOSITIVE:
		direction.Y = 0.f;
		break;
	case SBGravityMode::GRAVITY_ZNEGATIVE:
	case SBGravityMode::GRAVITY_ZPOSITIVE:
		direction.Z = 0.f;
		break;
	}

	SimBioticMath::float3 vector;
	vector.x = direction.X;
	vector.y = direction.Y;
	vector.z = direction.Z;

	SimBioticMath::float3 vectorStrafe;
	vectorStrafe.x = directionStrafe.X;
	vectorStrafe.y = directionStrafe.Y;
	vectorStrafe.z = directionStrafe.Z;

	SimBioticMath::float3 unitVector = SimBioticMath::GetClosestUnitVector(vector);
	GravityDirection.X = unitVector.x;
	GravityDirection.Y = unitVector.y;
	GravityDirection.Z = unitVector.z;

	SimBioticMath::float3 unitVectorStrafe = SimBioticMath::GetClosestUnitVector(vectorStrafe);

	IsGravityForwarding = true;
	accumulatedGravityAngle = 0.f;
	currentGravityRotatingAxis.X = unitVectorStrafe.x;
	currentGravityRotatingAxis.Y = unitVectorStrafe.y;
	currentGravityRotatingAxis.Z = unitVectorStrafe.z;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Gravity Direction is now: %f, %f, %f"), GravityDirection.X, GravityDirection.Y, GravityDirection.Z));


	if (Controller && Controller->IsLocalPlayerController()) {
		AShooterPlayerController* const PC = CastChecked<AShooterPlayerController>(Controller);
		PC->SetGravityMode(GetGravityMode());

	}
}
void AShooterCharacter::FaceRotation(FRotator NewControlRotation, float DeltaTime)
{
	if (Role == ROLE_SimulatedProxy) {
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("FaceRotation on simulated proxy"));
	}
	const FRotator CurrentRotation = GetActorRotation();

	FVector direction = FRotationMatrix(NewControlRotation).GetUnitAxis(EAxis::X);
	FVector pitchVector = FRotationMatrix(NewControlRotation).GetUnitAxis(EAxis::Y);
	FVector flattenedDirection = direction;// = FlattenVector(direction, GravityMode);
	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
	case GRAVITY_XPOSITIVE:
		flattenedDirection.X = 0.f;
		break;
	case GRAVITY_YNEGATIVE:
	case GRAVITY_YPOSITIVE:
		flattenedDirection.Y = 0.f;
		break;
	case GRAVITY_ZNEGATIVE:
	case GRAVITY_ZPOSITIVE:
		flattenedDirection.Z = 0.f;
		break;
	}

	
	direction.Normalize();
	flattenedDirection.Normalize();

	float angleBetween = acos(direction | flattenedDirection);
	FVector axisOfRotation = FVector::CrossProduct(direction, flattenedDirection);
	FRotator oldRotation = NewControlRotation;
	FQuat oldQuaternion = FQuat(oldRotation);
	FQuat modQuaternion = FQuat(axisOfRotation, angleBetween);
	FQuat newQuat = modQuaternion * oldQuaternion;
	FRotator newRotation = newQuat.Rotator();
	/*
	if (!bUseControllerRotationPitch)
	{
		NewControlRotation.Pitch = CurrentRotation.Pitch;
	}

	if (!bUseControllerRotationYaw)
	{
		NewControlRotation.Yaw = CurrentRotation.Yaw;
	}

	if (!bUseControllerRotationRoll)
	{
		NewControlRotation.Roll = CurrentRotation.Roll;
	}
	*/
	//SetActorRotation(NewControlRotation);
	//return;
	SetActorRotation(newRotation);
}

void AShooterCharacter::SetFullControlRotation(const FRotator& NewFullControlRotation) {
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("SETFULL"));
	FullControlRotation = NewFullControlRotation;
	FaceRotation(FullControlRotation, 0.f);

	if (Role < ROLE_Authority) {
		ServerSetFullControlRotation(NewFullControlRotation);
	}
}
bool AShooterCharacter::ServerSetFullControlRotation_Validate(const FRotator& NewFullControlRotation) {
	return true;
}

void AShooterCharacter::ServerSetFullControlRotation_Implementation(const FRotator& NewFullControlRotation) {
	SetFullControlRotation(NewFullControlRotation);
}