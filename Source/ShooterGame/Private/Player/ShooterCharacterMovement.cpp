// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Engine.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacterMovement, Log, All);
const float MAX_STEP_SIDE_Z = 0.08f;	// maximum z value for the normal on the vertical side of steps

/*
void FFindFloorResult::SetFromSweep(const FHitResult& InHit, const float InSweepFloorDist, const bool bIsWalkableFloor)
{
	bBlockingHit = InHit.IsValidBlockingHit();
	bWalkableFloor = bIsWalkableFloor;
	bLineTrace = false;
	FloorDist = InSweepFloorDist;
	LineDist = 0.f;
	HitResult = InHit;
}

void FFindFloorResult::SetFromLineTrace(const FHitResult& InHit, const float InSweepFloorDist, const float InLineDist, const bool bIsWalkableFloor)
{
	// We require a sweep that hit if we are going to use a line result.
	check(HitResult.bBlockingHit);
	if (HitResult.bBlockingHit && InHit.bBlockingHit)
	{
		// Override most of the sweep result with the line result, but save some values
		FHitResult OldHit(HitResult);
		HitResult = InHit;

		// Restore some of the old values. We want the new normals and hit actor, however.
		HitResult.Time = OldHit.Time;
		HitResult.ImpactPoint = OldHit.ImpactPoint;
		HitResult.Location = OldHit.Location;
		HitResult.TraceStart = OldHit.TraceStart;
		HitResult.TraceEnd = OldHit.TraceEnd;

		bLineTrace = true;
		LineDist = InLineDist;
		bWalkableFloor = bIsWalkableFloor;
	}
}
*/
//----------------------------------------------------------------------//
// UPawnMovementComponent
//----------------------------------------------------------------------//
UShooterCharacterMovement::UShooterCharacterMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GravityMode = GRAVITY_ZNEGATIVE;
}


float UShooterCharacterMovement::GetMaxSpeed() const
{
	float MaxSpeed = Super::GetMaxSpeed();

	const AShooterCharacter* ShooterCharacterOwner = Cast<AShooterCharacter>(PawnOwner);
	if (ShooterCharacterOwner)
	{
		if (ShooterCharacterOwner->IsTargeting())
		{
			MaxSpeed *= ShooterCharacterOwner->GetTargetingSpeedModifier();
		}
		if (ShooterCharacterOwner->IsRunning())
		{
			MaxSpeed *= ShooterCharacterOwner->GetRunningSpeedModifier();
		}
	}

	return MaxSpeed;
}

void UShooterCharacterMovement::setGravityMode(SBGravityMode mode)
{
	GravityMode = mode;
}
bool UShooterCharacterMovement::DoJump(bool bReplaysMove)
{

	if (CharacterOwner)
	{

		/* depending on gravity mode, select specific PlaneConstraint vector component*/
		float planeConstraintNormalComponent = 0.0f;
		switch (GravityMode)
		{
		case GRAVITY_XNEGATIVE:
			// Don't jump if we can't move up/down.
			if (FMath::Abs(PlaneConstraintNormal.X) != 1.f)
			{
				Velocity.X = JumpZVelocity;
				SetMovementMode(MOVE_Falling);
				return true;
			}
			break;
		case GRAVITY_XPOSITIVE:
			// Don't jump if we can't move up/down.
			if (FMath::Abs(PlaneConstraintNormal.X) != 1.f)
			{
				Velocity.X = -JumpZVelocity;
				SetMovementMode(MOVE_Falling);
				return true;
			}
			break;
		case GRAVITY_YNEGATIVE:
			// Don't jump if we can't move up/down.
			if (FMath::Abs(PlaneConstraintNormal.Y) != 1.f)
			{
				Velocity.Y = JumpZVelocity;
				SetMovementMode(MOVE_Falling);
				return true;
			}
			break;
		case GRAVITY_YPOSITIVE:
			// Don't jump if we can't move up/down.
			if (FMath::Abs(PlaneConstraintNormal.Y) != 1.f)
			{
				Velocity.Y = -JumpZVelocity;
				SetMovementMode(MOVE_Falling);
				return true;
			}
			break;
		case GRAVITY_ZNEGATIVE:
			// Don't jump if we can't move up/down.
			if (FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
			{
				Velocity.Z = JumpZVelocity;
				SetMovementMode(MOVE_Falling);
				return true;
			}
			break;
		case GRAVITY_ZPOSITIVE:
			// Don't jump if we can't move up/down.
			if (FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
			{
				Velocity.Z = -JumpZVelocity;
				SetMovementMode(MOVE_Falling);
				return true;
			}
			break;
		default:
			break;
		}


	}

	return false;
}

void UShooterCharacterMovement::JumpOff(AActor* MovementBaseActor)
{
	
	if (!bPerformingJumpOff)
	{
		bPerformingJumpOff = true;
		if (CharacterOwner)
		{
			const float MaxSpeed = GetMaxSpeed() * 0.85f;
			Velocity += MaxSpeed * GetBestDirectionOffActor(MovementBaseActor);
			if (/*Velocity.Size2D()*/GDSize2D(Velocity) > MaxSpeed)
			{
				Velocity = MaxSpeed * Velocity.SafeNormal();
			}

			switch (GravityMode)
			{
			case GRAVITY_XNEGATIVE:
				Velocity.X = JumpOffJumpZFactor * JumpZVelocity;
				break;
			case GRAVITY_XPOSITIVE:
				Velocity.X = JumpOffJumpZFactor * -JumpZVelocity;
				break;
			case GRAVITY_YNEGATIVE:
				Velocity.Y = JumpOffJumpZFactor * JumpZVelocity;
				break;
			case GRAVITY_YPOSITIVE:
				Velocity.Y = JumpOffJumpZFactor * -JumpZVelocity;
				break;
			case GRAVITY_ZNEGATIVE:
				Velocity.Z = JumpOffJumpZFactor * JumpZVelocity;
				break;
			case GRAVITY_ZPOSITIVE:
				Velocity.Z = JumpOffJumpZFactor * -JumpZVelocity;
				break;
			}

			SetMovementMode(MOVE_Falling);
		}
		bPerformingJumpOff = false;
	}
}

void UShooterCharacterMovement::SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode)
{

	if (NewMovementMode != MOVE_Custom)
	{
		NewCustomMode = 0;
	}

	// Do nothing if nothing is changing.
	if (MovementMode == NewMovementMode)
	{
		// Allow changes in custom sub-mode.
		if ((NewMovementMode != MOVE_Custom) || (NewCustomMode == CustomMovementMode))
		{
			return;
		}
	}

	const EMovementMode PrevMovementMode = MovementMode;
	const uint8 PrevCustomMode = CustomMovementMode;

	MovementMode = NewMovementMode;
	CustomMovementMode = NewCustomMode;

	// We allow setting movement mode before we have a component to update, in case this happens at startup.
	if (!HasValidData())
	{
		return;
	}

	// Handle change in movement mode
	OnMovementModeChanged(PrevMovementMode, PrevCustomMode);

	// @todo UE4 do we need to disable ragdoll physics here? Should this function do nothing if in ragdoll?

}

void UShooterCharacterMovement::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) {
	if (!HasValidData())
	{
		return;
	}

	//Here we select which velocity component is being changed. Before it was Velocity.Z = 0.0f, now it could be any.
	float* velocityComponentToZero = NULL;
	switch (GravityMode)
	{
	case GRAVITY_XNEGATIVE:
	case GRAVITY_XPOSITIVE:
		velocityComponentToZero = &Velocity.X;
		break;
	case GRAVITY_YNEGATIVE:
	case GRAVITY_YPOSITIVE:
		velocityComponentToZero = &Velocity.Y;
		break;
	case GRAVITY_ZNEGATIVE:
	case GRAVITY_ZPOSITIVE:
		velocityComponentToZero = &Velocity.Z;
		break;
	default:
		break;
	}
	if (MovementMode == MOVE_Walking)
	{
		//Velocity.Z = 0.f;
		*velocityComponentToZero = 0.f;
	}

	if (MovementMode == MOVE_None)
	{
		return;
	}

	if (MovementMode == MOVE_Walking)
	{
		// Walking uses only XY velocity, and must be on a walkable floor, with a Base.
		*velocityComponentToZero = 0.f;
		bCrouchMaintainsBaseLocation = true;

		// make sure we update our new floor/base on initial entry of the walking physics
		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
		AdjustFloorHeight();
		SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
	}
	else
	{
		CurrentFloor.Clear();
		bCrouchMaintainsBaseLocation = false;

		if (MovementMode == MOVE_Falling)
		{
			Velocity += GetImpartedMovementBaseVelocity();
			CharacterOwner->Falling();
		}

		SetBase(NULL);

		if (MovementMode == MOVE_None)
		{
			// Kill velocity and clear queued up events
			StopMovementKeepPathing();
			CharacterOwner->ClearJumpInput();
		}
	}

	CharacterOwner->OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

/* STILL TO DO GRAVITY DEPENDENCE*/

/*
void UShooterCharacterMovement::PerformAirControl(FVector Direction, float ZDiff)
{
	UE_LOG(LogCharacterMovement, Warning, TEXT("PerformAirControl"));
	//TO DO: HOWEVER WE NEED TO KNOW MORE ABOUT HOW ZDIFF IS IMPLEMENTED
	// use air control if low grav or above destination and falling towards it
	if (CharacterOwner && Velocity.Z < 0.f && (ZDiff < 0.f || GetGravityZ() > 0.9f * GetWorld()->GetDefaultGravityZ()))
	{
		if (ZDiff > 0.f)
		{
			if (ZDiff > 2.f * GetMaxJumpHeight())
			{
				if (PathFollowingComp.IsValid())
				{
					PathFollowingComp->AbortMove(TEXT("missed jump"));
				}
			}
		}
		else
		{
			if ((Velocity.X == 0.f) && (Velocity.Y == 0.f))
			{
				Acceleration = FVector::ZeroVector;
			}
			else
			{
				float Dist2D = GDSize2D(Direction);// Direction.Size2D();
				//Direction.Z = 0.f;
				Acceleration = Direction.SafeNormal() * GetModifiedMaxAcceleration();

				if ((Dist2D < 0.5f * FMath::Abs(Direction.Z)) && ((Velocity | Direction) > 0.5f*FMath::Square(Dist2D)))
				{
					Acceleration *= -1.f;
				}

				if (Dist2D < 1.5f*CharacterOwner->CapsuleComponent->GetScaledCapsuleRadius())
				{
					Velocity.X = 0.f;
					Velocity.Y = 0.f;
					Acceleration = FVector::ZeroVector;
				}
				else if ((Velocity | Direction) < 0.f)
				{
					float M = FMath::Max(0.f, 0.2f - GetWorld()->DeltaTimeSeconds);
					Velocity.X *= M;
					Velocity.Y *= M;
				}
			}
		}
	}
}
*/

void UShooterCharacterMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{



	//Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const FVector InputVector = ConsumeInputVector();
	if (!HasValidData() || ShouldSkipUpdate(DeltaTime) || UpdatedComponent->IsSimulatingPhysics())
	{
		return;
	}

	if (AvoidanceLockTimer > 0.0f)
	{
		AvoidanceLockTimer -= DeltaTime;
	}

	if (CharacterOwner->Role > ROLE_SimulatedProxy)
	{
		if (CharacterOwner->Role == ROLE_Authority)
		{
			// Check we are still in the world, and stop simulating if not.
			const bool bStillInWorld = (bCheatFlying || CharacterOwner->CheckStillInWorld());
			if (!bStillInWorld || !HasValidData())
			{
				return;
			}
		}

		// If we are a client we might have received an update from the server.
		const bool bIsClient = (GetNetMode() == NM_Client && CharacterOwner->Role == ROLE_AutonomousProxy);
		if (bIsClient)
		{
			ClientUpdatePositionAfterServerUpdate();
		}

		// Allow root motion to move characters that have no controller.
		if (CharacterOwner->IsLocallyControlled() || bRunPhysicsWithNoController || (!CharacterOwner->Controller && CharacterOwner->IsPlayingRootMotion()))
		{
			// We need to check the jump state before adjusting input acceleration, to minimize latency
			// and to make sure acceleration respects our potentially new falling state.
			CharacterOwner->CheckJumpInput(DeltaTime);

			// apply input to acceleration
			Acceleration = ScaleInputAcceleration(ConstrainInputAcceleration(InputVector));
			AnalogInputModifier = ComputeAnalogInputModifier();

			if (CharacterOwner->Role == ROLE_Authority)
			{
				PerformMovement(DeltaTime);
			}
			else if (bIsClient)
			{
				ReplicateMoveToServer(DeltaTime, Acceleration);
			}
		}
		else if (CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy)
		{
			// Server ticking for remote client.
			// Between net updates from the client we need to update position if based on another object,
			// otherwise the object will move on intermediate frames and we won't follow it.
			MaybeUpdateBasedMovement(DeltaTime);
			SaveBaseLocation();
		}
	}
	else if (CharacterOwner->Role == ROLE_SimulatedProxy)
	{
		AdjustProxyCapsuleSize();
		
		SimulatedTick(DeltaTime);
	}

	UpdateDefaultAvoidance();

	if (bEnablePhysicsInteraction)
	{
		if (CurrentFloor.HitResult.IsValidBlockingHit())
		{
			// Apply downwards force when walking on top of physics objects
			if (UPrimitiveComponent* BaseComp = CurrentFloor.HitResult.GetComponent())
			{
				if (StandingDownwardForceScale != 0.f && BaseComp->IsAnySimulatingPhysics())
				{

					const FVector ForceLocation = CurrentFloor.HitResult.ImpactPoint;


					//Again, like usual, GravZ and others are ACTUALLY gravity for any direction. We just don't want to start renaming stuff.
					float GravZ = GetGravityZ();
					FVector gravityVector;
					switch (GravityMode)
					{
					case GRAVITY_XNEGATIVE:
						gravityVector = FVector(GravZ * Mass *StandingDownwardForceScale, 0, 0);
						break;
					case GRAVITY_XPOSITIVE:
						gravityVector = FVector(-GravZ * Mass *StandingDownwardForceScale, 0, 0);
						break;
					case GRAVITY_YNEGATIVE:
						gravityVector = FVector(0, GravZ * Mass *StandingDownwardForceScale, 0);
						break;
					case GRAVITY_YPOSITIVE:
						gravityVector = FVector(0, -GravZ * Mass *StandingDownwardForceScale, 0);
						break;
					case GRAVITY_ZNEGATIVE:
						gravityVector = FVector(0, 0, GravZ * Mass *StandingDownwardForceScale);
						break;
					case GRAVITY_ZPOSITIVE:
						gravityVector = FVector(0, 0, -GravZ * Mass *StandingDownwardForceScale);
						break;
					}

					BaseComp->AddForceAtLocation(gravityVector, ForceLocation, CurrentFloor.HitResult.BoneName);
				}
			}
		}

		ApplyRepulsionForce(DeltaTime);
	}



	/*
	FString text = TEXT("");
	UE_LOG(LogCharacterMovement, Warning, TEXT("Velocity After Tick: %f, %f, %f"), Velocity.X, Velocity.Y, Velocity.Z);
	if (MovementMode == MOVE_Falling) text = TEXT("FALLING");
	if (MovementMode == MOVE_Walking) text = TEXT("WALKING");


	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
	text = TEXT("X NEGATIVE");
	break;
	case GRAVITY_XPOSITIVE:
	text = TEXT("X POSITIVE");
	break;
	case GRAVITY_YNEGATIVE:
	text = TEXT("Y NEGATIVE");
	break;
	case GRAVITY_YPOSITIVE:
	text = TEXT("Y POSITIVE");
	break;
	case GRAVITY_ZNEGATIVE:
	text = TEXT("Z NEGATIVE");
	break;
	case GRAVITY_ZPOSITIVE:
	text = TEXT("Z POSITIVE");
	break;
	}
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, text);
	*/
}
/*
void UShooterCharacterMovement::SimulateRootMotion(float DeltaSeconds, const FTransform & LocalRootMotionTransform)
{
if (CharacterOwner && CharacterOwner->Mesh && (DeltaSeconds > 0.f))
{
// Convert Local Space Root Motion to world space. Do it right before used by physics to make sure we use up to date transforms, as translation is relative to rotation.
const FTransform WorldSpaceRootMotionTransform = CharacterOwner->Mesh->ConvertLocalRootMotionToWorld(LocalRootMotionTransform);

// Compute root motion velocity to be used by physics
const FVector RootMotionVelocity = WorldSpaceRootMotionTransform.GetTranslation() / DeltaSeconds;

//THIS MAY NEED to be updated, not sure how this works exactly with multiple gravity directions. We will try this for now, il keep the old
//implementation there in case.
// Do not override Velocity.Z if falling.
//Velocity = FVector(RootMotionVelocity.X, RootMotionVelocity.Y, (MovementMode == MOVE_Falling ? Velocity.Z : RootMotionVelocity.Z));
switch (GravityMode)
{
case GRAVITY_XNEGATIVE:
case GRAVITY_XPOSITIVE:
Velocity = FVector((MovementMode == MOVE_Falling ? Velocity.X : RootMotionVelocity.X), RootMotionVelocity.Y, RootMotionVelocity.Z);
break;
case GRAVITY_YNEGATIVE:
case GRAVITY_YPOSITIVE:
Velocity = FVector(RootMotionVelocity.X, (MovementMode == MOVE_Falling ? Velocity.Y : RootMotionVelocity.Y), RootMotionVelocity.Z);
break;
case GRAVITY_ZNEGATIVE:
case GRAVITY_ZPOSITIVE:
Velocity = FVector(RootMotionVelocity.X, RootMotionVelocity.Y, (MovementMode == MOVE_Falling ? Velocity.Z : RootMotionVelocity.Z));
break;
}

StartNewPhysics(DeltaSeconds, 0);
// fixme laurent - simulate movement seems to have step up issues? investigate as that would be cheaper to use.
// 		SimulateMovement(DeltaSeconds);

// Apply Root Motion rotation after movement is complete.
const FRotator RootMotionRotation = WorldSpaceRootMotionTransform.GetRotation().Rotator();
if (!RootMotionRotation.IsNearlyZero())
{
const FRotator NewActorRotation = (CharacterOwner->GetActorRotation() + RootMotionRotation).GetNormalized();
MoveUpdatedComponent(FVector::ZeroVector, NewActorRotation, true);
}
}
}
*/

void UShooterCharacterMovement::SimulateMovement(float DeltaSeconds)
{
	if (!HasValidData() || UpdatedComponent->Mobility != EComponentMobility::Movable || UpdatedComponent->IsSimulatingPhysics())
	{
		return;
	}

	const bool bIsSimulatedProxy = (CharacterOwner->Role == ROLE_SimulatedProxy);

	// Workaround for replication not being updated initially
	if (bIsSimulatedProxy &&
		CharacterOwner->ReplicatedMovement.Location.IsZero() &&
		CharacterOwner->ReplicatedMovement.Rotation.IsZero() &&
		CharacterOwner->ReplicatedMovement.LinearVelocity.IsZero())
	{
		return;
	}

	// If base is not resolved on the client, we should not try to simulate at all
	if (CharacterOwner->GetBasedMovement().IsBaseUnresolved())
	{
		UE_LOG(LogCharacterMovement, Verbose, TEXT("Base for simulated character '%s' is not resolved on client, skipping SimulateMovement"), *CharacterOwner->GetName());
		return;
	}

	FVector OldVelocity;
	FVector OldLocation;

	// Scoped updates can improve performance of multiple MoveComponent calls.
	{
		FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

		if (bIsSimulatedProxy)
		{
			// Handle network changes
			if (bNetworkUpdateReceived)
			{
				bNetworkUpdateReceived = false;
				if (bNetworkMovementModeChanged)
				{
					bNetworkMovementModeChanged = false;
					ApplyNetworkMovementMode(CharacterOwner->GetReplicatedMovementMode());
				}
				else if (bJustTeleported)
				{
					// Make sure floor is current. We will continue using the replicated base, if there was one.
					bJustTeleported = false;
					UpdateFloorFromAdjustment();
				}
			}

			HandlePendingLaunch();
		}

		if (MovementMode == MOVE_None)
		{
			return;
		}

		Acceleration = Velocity.SafeNormal();	// Not currently used for simulated movement
		AnalogInputModifier = 1.0f;				// Not currently used for simulated movement

		MaybeUpdateBasedMovement(DeltaSeconds);

		// simulated pawns predict location
		OldVelocity = Velocity;
		OldLocation = UpdatedComponent->GetComponentLocation();
		FStepDownResult StepDownResult;
		MoveSmooth(Velocity, DeltaSeconds, &StepDownResult);

		// consume path following requested velocity
		bHasRequestedVelocity = false;

		// if simulated gravity, find floor and check if falling
		const bool bEnableFloorCheck = (!CharacterOwner->bSimGravityDisabled || !bIsSimulatedProxy);
		if (bEnableFloorCheck && (MovementMode == MOVE_Walking || MovementMode == MOVE_Falling))
		{
			const FVector CollisionCenter = UpdatedComponent->GetComponentLocation();
			bool velocityDemonstratesFalling = false;//Velocity.Z <= 0.f
			FVector newFallVelocity(0.f, 0.f, 0.f);
			switch (GravityMode)
			{
			case GRAVITY_XNEGATIVE:
				velocityDemonstratesFalling = (Velocity.X <= 0.f);
				newFallVelocity = NewFallVelocity(Velocity, FVector(GetGravityZ(), 0.f, 0.f), DeltaSeconds);
				break;
			case GRAVITY_XPOSITIVE:
				velocityDemonstratesFalling = (Velocity.X >= 0.f);
				newFallVelocity = NewFallVelocity(Velocity, FVector(-GetGravityZ(), 0.f, 0.f), DeltaSeconds);
				break;
			case GRAVITY_YNEGATIVE:
				velocityDemonstratesFalling = (Velocity.Y <= 0.f);
				newFallVelocity = NewFallVelocity(Velocity, FVector(0.f, GetGravityZ(), 0.f), DeltaSeconds);
				break;
			case GRAVITY_YPOSITIVE:
				velocityDemonstratesFalling = (Velocity.Y >= 0.f);
				newFallVelocity = NewFallVelocity(Velocity, FVector(0.f, -GetGravityZ(), 0.f), DeltaSeconds);
				break;
			case GRAVITY_ZNEGATIVE:
				velocityDemonstratesFalling = (Velocity.Z <= 0.f);
				newFallVelocity = NewFallVelocity(Velocity, FVector(0.f, 0.f, GetGravityZ()), DeltaSeconds);
				break;
			case GRAVITY_ZPOSITIVE:
				velocityDemonstratesFalling = (Velocity.Z >= 0.f);
				newFallVelocity = NewFallVelocity(Velocity, FVector(0.f, 0.f, -GetGravityZ()), DeltaSeconds);
				break;
			default:
				break;
			}

			if (StepDownResult.bComputedFloor)
			{
				CurrentFloor = StepDownResult.FloorResult;
			}
			else if (velocityDemonstratesFalling)
			{
				FindFloor(CollisionCenter, CurrentFloor, Velocity.IsZero(), NULL);
			}
			else
			{
				CurrentFloor.Clear();
			}

			if (!CurrentFloor.IsWalkableFloor())
			{
				// No floor, must fall.
				Velocity = newFallVelocity;// NewFallVelocity(Velocity, FVector(0.f, 0.f, GetGravityZ()), DeltaSeconds);
				SetMovementMode(MOVE_Falling);
			}
			else
			{
				// Walkable floor
				if (MovementMode == MOVE_Walking)
				{
					AdjustFloorHeight();
					SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
				}
				else if (MovementMode == MOVE_Falling)
				{
					if (CurrentFloor.FloorDist <= MIN_FLOOR_DIST)
					{
						// Landed
						SetMovementMode(MOVE_Walking);
					}
					else
					{
						// Continue falling.
						Velocity = newFallVelocity;// NewFallVelocity(Velocity, FVector(0.f, 0.f, GetGravityZ()), DeltaSeconds);
						CurrentFloor.Clear();
					}
				}
			}
		}

		OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
	} // End scoped movement update

	// Call custom post-movement events. These happen after the scoped movement completes in case the events want to use the current state of overlaps etc.
	CallMovementUpdateDelegate(DeltaSeconds, OldLocation, OldVelocity);

	SaveBaseLocation();
	UpdateComponentVelocity();
	bJustTeleported = false;

	LastUpdateLocation = UpdatedComponent ? UpdatedComponent->GetComponentLocation() : FVector::ZeroVector;
}
/*
void UShooterCharacterMovement::SimulateMovement(float DeltaSeconds)
{
	UE_LOG(LogCharacterMovement, Warning, TEXT("Simulate"));
	if (!CharacterOwner || !UpdatedComponent)
	{
		return;
	}

	// Workaround for replication not being updated initially
	if (CharacterOwner->ReplicatedMovement.Location.IsZero() &&
		CharacterOwner->ReplicatedMovement.Rotation.IsZero() &&
		CharacterOwner->ReplicatedMovement.LinearVelocity.IsZero())
	{
		return;
	}

	// If base is not resolved on the client, we should not try to simulate at all
	if (CharacterOwner->GetBasedMovement().IsBaseUnresolved())
	{
		UE_LOG(LogCharacterMovement, Verbose, TEXT("Base for simulated character '%s' is not resolved on client, skipping SimulateMovement"), *CharacterOwner->GetName());
		return;
	}

	FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

	// handle pending launches
	if (!PendingLaunchVelocity.IsZero() && IsValid(UpdatedComponent->GetOwner()))
	{
		UE_LOG(LogCharacterMovement, Warning, TEXT("PendingLaunch"));
		// Handle pending Launch here
		Velocity = PendingLaunchVelocity;
		SetMovementMode(MOVE_Falling);
		PendingLaunchVelocity = FVector::ZeroVector;
	}

	// Movement mode is not replicated, guess at it.
	SetMovementMode(DetermineSimulatedMovementMode());
	if (MovementMode == MOVE_None)
	{
		return;
	}

	Acceleration = Velocity.SafeNormal();

	// simulated pawns predict location
	FStepDownResult StepDownResult;
	MoveSmooth(Velocity, DeltaSeconds, &StepDownResult);

	// if simulated gravity, find floor and check if falling
	if (CharacterOwner->bSimulateGravity && !CharacterOwner->bSimGravityDisabled && (MovementMode == MOVE_Walking || MovementMode == MOVE_Falling))
	{
		FVector newFallVelocity;

		const FVector CollisionCenter = UpdatedComponent->GetComponentLocation();
		bool velocityDemonstratesFalling = false;//Velocity.Z <= 0.f
		switch (GravityMode)
		{
		case GRAVITY_XNEGATIVE:
			velocityDemonstratesFalling = (Velocity.X <= 0.f);
			newFallVelocity = NewFallVelocity(Velocity, FVector(GetGravityZ(), 0.f, 0.f), DeltaSeconds);
			break;
		case GRAVITY_XPOSITIVE:
			velocityDemonstratesFalling = (Velocity.X >= 0.f);
			newFallVelocity = NewFallVelocity(Velocity, FVector(-GetGravityZ(), 0.f, 0.f), DeltaSeconds);
			break;
		case GRAVITY_YNEGATIVE:
			velocityDemonstratesFalling = (Velocity.Y <= 0.f);
			newFallVelocity = NewFallVelocity(Velocity, FVector(0.f, GetGravityZ(), 0.f), DeltaSeconds);
			break;
		case GRAVITY_YPOSITIVE:
			velocityDemonstratesFalling = (Velocity.Y >= 0.f);
			newFallVelocity = NewFallVelocity(Velocity, FVector(0.f, -GetGravityZ(), 0.f), DeltaSeconds);
			break;
		case GRAVITY_ZNEGATIVE:
			velocityDemonstratesFalling = (Velocity.Z <= 0.f);
			newFallVelocity = NewFallVelocity(Velocity, FVector(0.f, 0.f, GetGravityZ()), DeltaSeconds);
			break;
		case GRAVITY_ZPOSITIVE:
			velocityDemonstratesFalling = (Velocity.Z >= 0.f);
			newFallVelocity = NewFallVelocity(Velocity, FVector(0.f, 0.f, -GetGravityZ()), DeltaSeconds);
			break;
		default:
			break;
		}
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else if (velocityDemonstratesFalling)
		{
			FindFloor(CollisionCenter, CurrentFloor, Velocity.IsZero(), NULL);
		}
		else
		{
			CurrentFloor.Clear();
		}

		if (!CurrentFloor.IsWalkableFloor())
		{
			// No floor, must fall. However, we must fall according to the current gravity configuration.
			Velocity = newFallVelocity;
			SetMovementMode(MOVE_Falling);
		}
		else
		{
			// Walkable floor
			if (MovementMode == MOVE_Walking)
			{
				AdjustFloorHeight();
				SetBase(CurrentFloor.HitResult.Component.Get());
			}
			else if (MovementMode == MOVE_Falling)
			{
				if (CurrentFloor.FloorDist <= MIN_FLOOR_DIST)
				{
					SetMovementMode(MOVE_Walking);
				}
				else
				{
					// Continue falling.


					Velocity = newFallVelocity;
					CurrentFloor.Clear();
				}
			}
		}
	}

	UpdateComponentVelocity();
}
*/
void UShooterCharacterMovement::UpdateBasedRotation(FRotator &FinalRotation, const FRotator& ReducedRotation)
{
	AController* Controller = CharacterOwner ? CharacterOwner->Controller : NULL;
	float ControllerRoll = 0.f;
	if (Controller && !bIgnoreBaseRotation)
	{
		FRotator const ControllerRot = Controller->GetControlRotation();
		ControllerRoll = ControllerRot.Roll;
		Controller->SetControlRotation(ControllerRot + ReducedRotation);
	}

	// Remove roll
	FinalRotation.Roll = 0.f;
	if (Controller)
	{
		FinalRotation.Roll = CharacterOwner->GetActorRotation().Roll;
		FRotator NewRotation = Controller->GetControlRotation();
		NewRotation.Roll = ControllerRoll;
		Controller->SetControlRotation(NewRotation);
	}
}
void UShooterCharacterMovement::PerformMovement(float DeltaSeconds)
{
	//SCOPE_CYCLE_COUNTER(STAT_CharacterMovementAuthority);

	if (!HasValidData())
	{
		return;
	}

	// no movement if we can't move, or if currently doing physical simulation on UpdatedComponent
	if (MovementMode == MOVE_None || UpdatedComponent->Mobility != EComponentMobility::Movable || UpdatedComponent->IsSimulatingPhysics())
	{
		return;
	}

	// Force floor update if we've moved outside of CharacterMovement since last update.
	bForceNextFloorCheck |= (IsMovingOnGround() && UpdatedComponent->GetComponentLocation() != LastUpdateLocation);

	FVector OldVelocity;
	FVector OldLocation;

	// Scoped updates can improve performance of multiple MoveComponent calls.
	{
		FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

		MaybeUpdateBasedMovement(DeltaSeconds);

		OldVelocity = Velocity;
		OldLocation = CharacterOwner->GetActorLocation();

		ApplyAccumulatedForces(DeltaSeconds);

		// Check for a change in crouch state. Players toggle crouch by changing bWantsToCrouch.
		const bool bAllowedToCrouch = CanCrouchInCurrentState();
		if ((!bAllowedToCrouch || !bWantsToCrouch) && IsCrouching())
		{
			UnCrouch(false);
		}
		else if (bWantsToCrouch && bAllowedToCrouch && !IsCrouching())
		{
			Crouch(false);
		}

		// Character::LaunchCharacter() has been deferred until now.
		HandlePendingLaunch();

		// If using RootMotion, tick animations before running physics.
		if (!CharacterOwner->bClientUpdating && CharacterOwner->IsPlayingRootMotion() && CharacterOwner->GetMesh())
		{
			TickCharacterPose(DeltaSeconds);

			// Make sure animation didn't trigger an event that destroyed us
			if (!HasValidData())
			{
				return;
			}

			// For local human clients, save off root motion data so it can be used by movement networking code.
			if (CharacterOwner->IsLocallyControlled() && (CharacterOwner->Role == ROLE_AutonomousProxy) && CharacterOwner->IsPlayingNetworkedRootMotionMontage())
			{
				CharacterOwner->ClientRootMotionParams = RootMotionParams;
			}
		}

		// if we're about to use root motion, convert it to world space first.
		if (HasRootMotion())
		{
			USkeletalMeshComponent * SkelMeshComp = CharacterOwner->GetMesh();
			if (SkelMeshComp)
			{
				// Convert Local Space Root Motion to world space. Do it right before used by physics to make sure we use up to date transforms, as translation is relative to rotation.
				RootMotionParams.Set(SkelMeshComp->ConvertLocalRootMotionToWorld(RootMotionParams.RootMotionTransform));
				UE_LOG(LogRootMotion, Log, TEXT("PerformMovement WorldSpaceRootMotion Translation: %s, Rotation: %s, Actor Facing: %s"),
					*RootMotionParams.RootMotionTransform.GetTranslation().ToCompactString(), *RootMotionParams.RootMotionTransform.GetRotation().Rotator().ToCompactString(), *CharacterOwner->GetActorRotation().Vector().ToCompactString());
			}

			// Then turn root motion to velocity to be used by various physics modes.
			if (DeltaSeconds > 0.f)
			{
				const FVector RootMotionVelocity = RootMotionParams.RootMotionTransform.GetTranslation() / DeltaSeconds;
				// Do not override Velocity.Z if in falling physics, we want to keep the effect of gravity.
				switch (GravityMode)
				{
				case GRAVITY_XNEGATIVE:
				case GRAVITY_XPOSITIVE:
					Velocity = FVector((MovementMode == MOVE_Falling ? Velocity.Z : RootMotionVelocity.X), RootMotionVelocity.Y, RootMotionVelocity.Z);
					break;
				case GRAVITY_YNEGATIVE:
				case GRAVITY_YPOSITIVE:
					Velocity = FVector(RootMotionVelocity.X, (MovementMode == MOVE_Falling ? Velocity.Z : RootMotionVelocity.Y), RootMotionVelocity.Z);
					break;
				case GRAVITY_ZNEGATIVE:
				case GRAVITY_ZPOSITIVE:
					Velocity = FVector(RootMotionVelocity.X, RootMotionVelocity.Y, (MovementMode == MOVE_Falling ? Velocity.Z : RootMotionVelocity.Z));
					break;
				default:
					break;
				}
			}
		}

		// NaN tracking
		checkf(!Velocity.ContainsNaN(), TEXT("UCharacterMovementComponent::PerformMovement: Velocity contains NaN (%s: %s)\n%s"), *GetPathNameSafe(this), *GetPathNameSafe(GetOuter()), *Velocity.ToString());

		// Clear jump input now, to allow movement events to trigger it for next update.
		CharacterOwner->ClearJumpInput();

		// change position
		StartNewPhysics(DeltaSeconds, 0);

		if (!HasValidData())
		{
			return;
		}

		// uncrouch if no longer allowed to be crouched
		if (IsCrouching() && !CanCrouchInCurrentState())
		{
			UnCrouch(false);
		}

		if (!HasRootMotion() && !CharacterOwner->IsMatineeControlled())
		{
			PhysicsRotation(DeltaSeconds);
		}

		// Apply Root Motion rotation after movement is complete.
		if (HasRootMotion())
		{
			const FRotator OldActorRotation = CharacterOwner->GetActorRotation();
			const FRotator RootMotionRotation = RootMotionParams.RootMotionTransform.GetRotation().Rotator();
			if (!RootMotionRotation.IsNearlyZero())
			{
				const FRotator NewActorRotation = (OldActorRotation + RootMotionRotation).GetNormalized();
				MoveUpdatedComponent(FVector::ZeroVector, NewActorRotation, true);
			}

			// debug
			if (false)
			{
				const FVector ResultingLocation = CharacterOwner->GetActorLocation();
				const FRotator ResultingRotation = CharacterOwner->GetActorRotation();

				// Show current position
				DrawDebugCoordinateSystem(GetWorld(), CharacterOwner->GetMesh()->GetComponentLocation() + FVector(0, 0, 1), ResultingRotation, 50.f, false);

				// Show resulting delta move.
				DrawDebugLine(GetWorld(), OldLocation, ResultingLocation, FColor::Red, true, 10.f);

				// Log details.
				UE_LOG(LogRootMotion, Warning, TEXT("PerformMovement Resulting DeltaMove Translation: %s, Rotation: %s, MovementBase: %s"),
					*(ResultingLocation - OldLocation).ToCompactString(), *(ResultingRotation - OldActorRotation).GetNormalized().ToCompactString(), *GetNameSafe(CharacterOwner->GetMovementBase()));

				const FVector RMTranslation = RootMotionParams.RootMotionTransform.GetTranslation();
				const FRotator RMRotation = RootMotionParams.RootMotionTransform.GetRotation().Rotator();
				UE_LOG(LogRootMotion, Warning, TEXT("PerformMovement Resulting DeltaError Translation: %s, Rotation: %s"),
					*(ResultingLocation - OldLocation - RMTranslation).ToCompactString(), *(ResultingRotation - OldActorRotation - RMRotation).GetNormalized().ToCompactString());
			}

			// Root Motion has been used, clear
			RootMotionParams.Clear();
		}

		// consume path following requested velocity
		bHasRequestedVelocity = false;

		OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
	} // End scoped movement update

	// Call external post-movement events. These happen after the scoped movement completes in case the events want to use the current state of overlaps etc.
	CallMovementUpdateDelegate(DeltaSeconds, OldLocation, OldVelocity);

	SaveBaseLocation();
	UpdateComponentVelocity();

	LastUpdateLocation = UpdatedComponent ? UpdatedComponent->GetComponentLocation() : FVector::ZeroVector;
}

/*
void UShooterCharacterMovement::Crouch(bool bClientSimulation)
{
	if (!CharacterOwner || !UpdatedComponent)
	{
		return;
	}

	// Do not perform if collision is already at desired size.
	if (CharacterOwner->CapsuleComponent->GetUnscaledCapsuleHalfHeight() == CrouchedHalfHeight)
	{
		return;
	}

	if (!CanCrouchInCurrentState())
	{
		return;
	}

	if (bClientSimulation && CharacterOwner->Role == ROLE_SimulatedProxy)
	{
		// restore collision size before crouching
		ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		CharacterOwner->CapsuleComponent->SetCapsuleSize(DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleRadius(), DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleHalfHeight());
		bShrinkProxyCapsule = true;
	}

	// Change collision size to crouching dimensions
	const float ComponentScale = CharacterOwner->CapsuleComponent->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->CapsuleComponent->GetUnscaledCapsuleHalfHeight();
	CharacterOwner->CapsuleComponent->SetCapsuleSize(CharacterOwner->CapsuleComponent->GetUnscaledCapsuleRadius(), CrouchedHalfHeight);
	float HalfHeightAdjust = (OldUnscaledHalfHeight - CrouchedHalfHeight);
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	if (!bClientSimulation)
	{
		// Crouching to a larger height? (this is rare)
		if (CrouchedHalfHeight > OldUnscaledHalfHeight)
		{
			static const FName NAME_CrouchTrace = FName(TEXT("CrouchTrace"));
			FCollisionQueryParams CapsuleParams(NAME_CrouchTrace, false, CharacterOwner);
			FCollisionResponseParams ResponseParam;
			InitCollisionParams(CapsuleParams, ResponseParam);
			//Create Offset vector for crouching depending on gravity mode
			FVector offsetVector(0.0f, 0.0f, 0.0f);
			switch (GravityMode)
			{
			case GRAVITY_XNEGATIVE:offsetVector = FVector(-ScaledHalfHeightAdjust, 0.f, 0.f); break;
			case GRAVITY_XPOSITIVE:offsetVector = FVector(ScaledHalfHeightAdjust, 0.f, 0.f); break;
			case GRAVITY_YNEGATIVE:offsetVector = FVector(0.f, -ScaledHalfHeightAdjust, 0.f); break;
			case GRAVITY_YPOSITIVE:offsetVector = FVector(0.f, ScaledHalfHeightAdjust, 0.f); break;
			case GRAVITY_ZNEGATIVE:offsetVector = FVector(0.f, 0.f, -ScaledHalfHeightAdjust); break;
			case GRAVITY_ZPOSITIVE:offsetVector = FVector(0.f, 0.f, ScaledHalfHeightAdjust); break;
			default:break;
			}

			const bool bEncroached = GetWorld()->OverlapTest(CharacterOwner->GetActorLocation() + offsetVector, FQuat::Identity,
				UpdatedComponent->GetCollisionObjectType(), GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleParams, ResponseParam);

			// If encroached, cancel
			if (bEncroached)
			{
				CharacterOwner->CapsuleComponent->SetCapsuleSize(CharacterOwner->CapsuleComponent->GetUnscaledCapsuleRadius(), OldUnscaledHalfHeight);
				return;
			}
		}

		if (bCrouchMovesCharacterDown)
		{
			// Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.

			FVector offsetVector(0.f, 0.f, 0.f);
			switch (GravityMode)
			{
			case GRAVITY_XNEGATIVE:offsetVector = FVector(-ScaledHalfHeightAdjust, 0.f, 0.f); break;
			case GRAVITY_XPOSITIVE:offsetVector = FVector(ScaledHalfHeightAdjust, 0.f, 0.f); break;
			case GRAVITY_YNEGATIVE:offsetVector = FVector(0.f, -ScaledHalfHeightAdjust, 0.f); break;
			case GRAVITY_YPOSITIVE:offsetVector = FVector(0.f, ScaledHalfHeightAdjust, 0.f); break;
			case GRAVITY_ZNEGATIVE:offsetVector = FVector(0.f, 0.f, -ScaledHalfHeightAdjust); break;
			case GRAVITY_ZPOSITIVE:offsetVector = FVector(0.f, 0.f, ScaledHalfHeightAdjust); break;
			default:break;
			}
			UpdatedComponent->MoveComponent(offsetVector, CharacterOwner->GetActorRotation(), true);
		}

		CharacterOwner->bIsCrouched = true;
	}

	bForceNextFloorCheck = true;

	// OnStartCrouch takes the change from the Default size, not the current one (though they are usually the same).
	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	HalfHeightAdjust = (DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleHalfHeight() - CrouchedHalfHeight);
	ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	AdjustProxyCapsuleSize();
	CharacterOwner->OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}
*/

/*
void UShooterCharacterMovement::UnCrouch(bool bClientSimulation)
{
	if (!CharacterOwner || !UpdatedComponent)
	{
		return;
	}

	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();

	// Do not perform if collision is already at desired size.
	if (CharacterOwner->CapsuleComponent->GetUnscaledCapsuleHalfHeight() == DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleHalfHeight())
	{
		return;
	}

	const float ComponentScale = CharacterOwner->CapsuleComponent->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->CapsuleComponent->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightAdjust = DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleHalfHeight() - OldUnscaledHalfHeight;
	const float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	const FVector PawnLocation = CharacterOwner->GetActorLocation();

	// Grow to uncrouched size.
	check(CharacterOwner->CapsuleComponent);
	bool bUpdateOverlaps = false;
	CharacterOwner->CapsuleComponent->SetCapsuleSize(DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleRadius(), DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleHalfHeight(), bUpdateOverlaps);
	CharacterOwner->CapsuleComponent->UpdateBounds(); // Force an update of the bounds with the new dimensions

	if (!bClientSimulation)
	{
		UPrimitiveComponent* OldBaseComponent = CharacterOwner->GetMovementBase();
		FFindFloorResult OldFloor = CurrentFloor;
		SetBase(NULL, false);

		// Try to stay in place and see if the larger capsule fits. We use a slightly taller capsule to avoid penetration.
		static const FName NAME_CrouchTrace = FName(TEXT("CrouchTrace"));
		const float SweepInflation = KINDA_SMALL_NUMBER;
		FCollisionQueryParams CapsuleParams(NAME_CrouchTrace, false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(CapsuleParams, ResponseParam);
		const FCollisionShape StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation);
		const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
		// Perf: Avoid this test when on the ground, because it almost always fails and we should just continue to the next step.
		bool bEncroached = true;
		if (!IsMovingOnGround())
		{
			bEncroached = GetWorld()->OverlapTest(PawnLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
		}

		if (bEncroached)
		{
			// Try adjusting capsule position to see if we can avoid encroachment.
			if (ScaledHalfHeightAdjust > 0.f)
			{
				// Shrink to a short capsule, sweep down to base to find where that would hit something, and then try to stand up from there.
				float PawnRadius, PawnHalfHeight;
				CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
				const float ShrinkHalfHeight = PawnHalfHeight - PawnRadius;
				const float TraceDist = PawnHalfHeight - ShrinkHalfHeight;

				FVector Down = FVector(0.f, 0.f, -TraceDist);
				switch (GravityMode)
				{
				case GRAVITY_XNEGATIVE:Down = FVector(-TraceDist, 0.f, 0.f); break;
				case GRAVITY_XPOSITIVE:Down = FVector(TraceDist, 0.f, 0.f); break;
				case GRAVITY_YNEGATIVE:Down = FVector(0.f, -TraceDist, 0.f); break;
				case GRAVITY_YPOSITIVE:Down = FVector(0.f, TraceDist, 0.f); break;
				case GRAVITY_ZNEGATIVE:Down = FVector(0.f, 0.f, -TraceDist); break;
				case GRAVITY_ZPOSITIVE:Down = FVector(0.f, 0.f, TraceDist); break;
				default:break;
				}


				FHitResult Hit(1.f);
				const FCollisionShape ShortCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, ShrinkHalfHeight);
				const bool bBlockingHit = GetWorld()->SweepSingle(Hit, PawnLocation, PawnLocation + Down, FQuat::Identity, CollisionChannel, ShortCapsuleShape, CapsuleParams);
				if (Hit.bStartPenetrating)
				{
					bEncroached = true;
				}
				else
				{
					// Compute where the base of the sweep ended up, and see if we can stand there
					const float DistanceToBase = (Hit.Time * TraceDist) + ShortCapsuleShape.Capsule.HalfHeight;
					//const FVector NewLoc = FVector(PawnLocation.X, PawnLocation.Y, PawnLocation.Z - DistanceToBase + PawnHalfHeight + SweepInflation + MIN_FLOOR_DIST / 2.f);
					FVector NewLoc(0.f, 0.f, 0.f);
					switch (GravityMode)
					{
					case GRAVITY_XNEGATIVE:NewLoc = FVector(PawnLocation.X - DistanceToBase + PawnHalfHeight + SweepInflation + MIN_FLOOR_DIST / 2.f, PawnLocation.Y, PawnLocation.Z); break;
					case GRAVITY_XPOSITIVE:NewLoc = FVector(PawnLocation.X + DistanceToBase - PawnHalfHeight - SweepInflation - MIN_FLOOR_DIST / 2.f, PawnLocation.Y, PawnLocation.Z); break;
					case GRAVITY_YNEGATIVE:NewLoc = FVector(PawnLocation.X, PawnLocation.Y - DistanceToBase + PawnHalfHeight + SweepInflation + MIN_FLOOR_DIST / 2.f, PawnLocation.Z); break;
					case GRAVITY_YPOSITIVE:NewLoc = FVector(PawnLocation.X, PawnLocation.Y + DistanceToBase - PawnHalfHeight - SweepInflation - MIN_FLOOR_DIST / 2.f, PawnLocation.Z); break;
					case GRAVITY_ZNEGATIVE:NewLoc = FVector(PawnLocation.X, PawnLocation.Y, PawnLocation.Z - DistanceToBase + PawnHalfHeight + SweepInflation + MIN_FLOOR_DIST / 2.f); break;
					case GRAVITY_ZPOSITIVE:NewLoc = FVector(PawnLocation.X, PawnLocation.Y, PawnLocation.Z + DistanceToBase - PawnHalfHeight - SweepInflation - MIN_FLOOR_DIST / 2.f); break;
					default:break;
					}
					bEncroached = GetWorld()->OverlapTest(NewLoc, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams);
					if (!bEncroached)
					{
						// Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
						UpdatedComponent->MoveComponent(NewLoc - PawnLocation, CharacterOwner->GetActorRotation(), false);
						const bool bBaseChanged = (Hit.Component.Get() != OldBaseComponent);
						CurrentFloor.SetFromSweep(Hit, 0.f, IsWalkable(Hit));
						SetBase(Hit.Component.Get(), bBaseChanged);
					}
				}
			}

			// If still encroached then abort.
			if (bEncroached)
			{
				CharacterOwner->CapsuleComponent->SetCapsuleSize(CharacterOwner->CapsuleComponent->GetUnscaledCapsuleRadius(), OldUnscaledHalfHeight, false);
				CharacterOwner->CapsuleComponent->UpdateBounds(); // Update bounds again back to old value
				CurrentFloor = OldFloor;
				SetBase(OldBaseComponent, false);
				return;
			}
		}

		CharacterOwner->bIsCrouched = false;
	}
	else
	{
		bShrinkProxyCapsule = true;
	}

	// now call SetCapsuleSize() to cause touch/untouch events
	bUpdateOverlaps = true;
	CharacterOwner->CapsuleComponent->SetCapsuleSize(DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleRadius(), DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleHalfHeight(), bUpdateOverlaps);

	bForceNextFloorCheck = true;
	AdjustProxyCapsuleSize();
	CharacterOwner->OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}
*/
FVector UShooterCharacterMovement::GetPenetrationAdjustment(const FHitResult& Hit) const
{
	FVector Adjustment = Super::GetPenetrationAdjustment(Hit);

	bool shouldMakeAdjustment = false;
	float* pAdjustmentComponent = 0;
	switch (GravityMode)
	{
	case GRAVITY_XNEGATIVE:
		shouldMakeAdjustment = (Adjustment.X > 0.f && IsMovingOnGround());
		pAdjustmentComponent = &Adjustment.X;
		break;
	case GRAVITY_XPOSITIVE:
		shouldMakeAdjustment = (Adjustment.X < 0.f && IsMovingOnGround());
		pAdjustmentComponent = &Adjustment.X;
		break;
	case GRAVITY_YNEGATIVE:
		shouldMakeAdjustment = (Adjustment.Y > 0.f && IsMovingOnGround());
		pAdjustmentComponent = &Adjustment.Y;
		break;
	case GRAVITY_YPOSITIVE:
		shouldMakeAdjustment = (Adjustment.Y < 0.f && IsMovingOnGround());
		pAdjustmentComponent = &Adjustment.Y;
		break;
	case GRAVITY_ZNEGATIVE:
		shouldMakeAdjustment = (Adjustment.Z > 0.f && IsMovingOnGround());
		pAdjustmentComponent = &Adjustment.Z;
		break;
	case GRAVITY_ZPOSITIVE:
		shouldMakeAdjustment = (Adjustment.Z < 0.f && IsMovingOnGround());
		pAdjustmentComponent = &Adjustment.Z;
		break;
	default:
		return Adjustment;
		break;
	}
	//if (Adjustment.Z > 0.f && IsMovingOnGround())
	//{
	//	// Don't allow upward adjustments to move us high off the ground.
	//	// Floor height adjustments will take care of penetrations on the lower portion of the capsule.
	//	//Adjustment.Z = 0.f;
	//	*pAdjustmentComponent = 0.f;
	//}

	if (IsMovingOnGround())
	{
		switch (GravityMode)
		{
		case GRAVITY_XNEGATIVE:
			if (Adjustment.X > 0.f) Adjustment.X = 0.f;
			break;
		case GRAVITY_XPOSITIVE:
			if (Adjustment.X < 0.f) Adjustment.X = 0.f;
			break;
		case GRAVITY_YNEGATIVE:
			if (Adjustment.Y > 0.f) Adjustment.Y = 0.f;
			break;
		case GRAVITY_YPOSITIVE:
			if (Adjustment.Y < 0.f) Adjustment.Y = 0.f;
			break;
		case GRAVITY_ZNEGATIVE:
			if (Adjustment.Z > 0.f) Adjustment.Z = 0.f;
			break;
		case GRAVITY_ZPOSITIVE:
			if (Adjustment.Z < 0.f) Adjustment.Z = 0.f;
			break;
		default:
			break;
		}
	}


	return Adjustment;
}

/* Gravity Dependent Safe Normal 2D Function. This function is meant to be used instead of FVector::SafeNormal2D() when gravity dependence plays a role*/
FVector UShooterCharacterMovement::GDSafeNormal2D(const FVector inVector) const
{
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

float UShooterCharacterMovement::GDSize2D(const FVector inVector) const {
	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
	case GRAVITY_XPOSITIVE:
		return FMath::Sqrt(inVector.Y * inVector.Y + inVector.Z * inVector.Z);
	case GRAVITY_YNEGATIVE:
	case GRAVITY_YPOSITIVE:
		return FMath::Sqrt(inVector.X * inVector.X + inVector.Z * inVector.Z);
	case GRAVITY_ZNEGATIVE:
	case GRAVITY_ZPOSITIVE:
		return FMath::Sqrt(inVector.X * inVector.X + inVector.Y * inVector.Y);
	}
	return 0.0f;

}

float UShooterCharacterMovement::GDSizeSquared2D(const FVector inVector) const {
	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
	case GRAVITY_XPOSITIVE:
		return inVector.Y * inVector.Y + inVector.Z * inVector.Z;
	case GRAVITY_YNEGATIVE:
	case GRAVITY_YPOSITIVE:
		return inVector.X * inVector.X + inVector.Z * inVector.Z;
	case GRAVITY_ZNEGATIVE:
	case GRAVITY_ZPOSITIVE:
		return inVector.X * inVector.X + inVector.Y * inVector.Y;
	}
	return 0.0f;

}

FVector UShooterCharacterMovement::GDClampSize2D(FVector inVector, float min, float max) const {
	float VecSize2D = GDSize2D(inVector);
	const FVector VecDir = (VecSize2D > SMALL_NUMBER) ? (inVector / VecSize2D) : FVector::ZeroVector;

	VecSize2D = FMath::Clamp(VecSize2D, min, max);

	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
	case GRAVITY_XPOSITIVE:
		return FVector(inVector.X, VecSize2D * VecDir.Y, VecSize2D * VecDir.Z);
		break;
	case GRAVITY_YNEGATIVE:
	case GRAVITY_YPOSITIVE:
		return FVector(VecSize2D * VecDir.X, inVector.Y, VecSize2D * VecDir.Z);
		break;
	case GRAVITY_ZNEGATIVE:
	case GRAVITY_ZPOSITIVE:
		return FVector(VecSize2D * VecDir.X, VecSize2D * VecDir.Y, inVector.Z);
		break;
	}
	return FVector(VecSize2D * VecDir.X, VecSize2D * VecDir.Y, inVector.Z);
}
FVector UShooterCharacterMovement::GDClampMaxSize2D(FVector inVector, float maxSize) const {
	if (maxSize < KINDA_SMALL_NUMBER)
	{
		switch (GravityMode) {
		case GRAVITY_XNEGATIVE:
		case GRAVITY_XPOSITIVE:
			return FVector(inVector.X, 0.f, 0.f);
		case GRAVITY_YNEGATIVE:
		case GRAVITY_YPOSITIVE:
			return FVector(0.f, inVector.Y, 0.f);
		case GRAVITY_ZNEGATIVE:
		case GRAVITY_ZPOSITIVE:
			return FVector(0.f, 0.f, inVector.Z);
		}

	}

	const float VSq2D = GDSizeSquared2D(inVector);
	if (VSq2D > FMath::Square(maxSize))
	{
		const float Scale = maxSize * FMath::InvSqrt(VSq2D);

		switch (GravityMode) {
		case GRAVITY_XNEGATIVE:
		case GRAVITY_XPOSITIVE:
			return FVector(inVector.X, inVector.Y*Scale, inVector.Z*Scale);
		case GRAVITY_YNEGATIVE:
		case GRAVITY_YPOSITIVE:
			return FVector(inVector.X*Scale, inVector.Y, inVector.Z*Scale);
		case GRAVITY_ZNEGATIVE:
		case GRAVITY_ZPOSITIVE:
			return FVector(inVector.X*Scale, inVector.Y*Scale, inVector.Z);
		}

	}
	else
	{
		return inVector;
	}

	return inVector;

}

bool UShooterCharacterMovement::GDIsWithinEdgeTolerance(const FVector& CapsuleLocation, const FVector& TestImpactPoint, const float CapsuleRadius) const {
	const float DistFromCenterSq = GDSizeSquared2D(TestImpactPoint - CapsuleLocation);//.SizeSquared2D();
	const float ReducedRadiusSq = FMath::Square(FMath::Max(KINDA_SMALL_NUMBER, CapsuleRadius - SWEEP_EDGE_REJECT_DISTANCE));
	return DistFromCenterSq < ReducedRadiusSq;
}
float UShooterCharacterMovement::SlideAlongSurface(const FVector& Delta, float Time, const FVector& InNormal, FHitResult &Hit, bool bHandleImpact)
{
	if (!Hit.bBlockingHit)
	{
		return 0.f;
	}

	FVector Normal(InNormal);
	if (IsMovingOnGround())
	{
		// We don't want to be pushed up an unwalkable surface.
		//Find if the normal points "upwards" in any way. Not only do we have
		//to override this function, we have to override the functionality of Normal.SafeNormal2D() in
		//some way so that it works in a manner that is dependent on gravity. We won't actually override the function
		//but we will have a local function above that will do the same thing, but taking into account your 
		//current gravity configuration.

		const FVector FloorNormal = CurrentFloor.HitResult.Normal;
		bool normalUpComponentIsAboveZero = false;
		bool normalUpComponentIsBelowSmallNumber = false;
		bool bFloorOpposedToMovement = false;
		switch (GravityMode)
		{
		case GRAVITY_XNEGATIVE:
			normalUpComponentIsAboveZero = (Normal.X > 0.f);
			normalUpComponentIsBelowSmallNumber = (Normal.X < -KINDA_SMALL_NUMBER);
			bFloorOpposedToMovement = (Delta | FloorNormal) < 0.f && (FloorNormal.X < 1.f - DELTA);
			break;
		case GRAVITY_XPOSITIVE:
			normalUpComponentIsAboveZero = (Normal.X < 0.f);
			normalUpComponentIsBelowSmallNumber = (Normal.X > KINDA_SMALL_NUMBER);
			bFloorOpposedToMovement = (Delta | FloorNormal) < 0.f && (FloorNormal.Y < 1.f - DELTA);
			break;
		case GRAVITY_YNEGATIVE:
			normalUpComponentIsAboveZero = (Normal.Y > 0.f);
			normalUpComponentIsBelowSmallNumber = (Normal.Y < -KINDA_SMALL_NUMBER);
			bFloorOpposedToMovement = (Delta | FloorNormal) < 0.f && (FloorNormal.Y < 1.f - DELTA);
			break;
		case GRAVITY_YPOSITIVE:
			normalUpComponentIsAboveZero = (Normal.Y < 0.f);
			normalUpComponentIsBelowSmallNumber = (Normal.Y > KINDA_SMALL_NUMBER);
			bFloorOpposedToMovement = (Delta | FloorNormal) < 0.f && (FloorNormal.Y < 1.f - DELTA);
			break;
		case GRAVITY_ZNEGATIVE:
			normalUpComponentIsAboveZero = (Normal.Z > 0.f);
			normalUpComponentIsBelowSmallNumber = (Normal.Z < -KINDA_SMALL_NUMBER);
			bFloorOpposedToMovement = (Delta | FloorNormal) < 0.f && (FloorNormal.Z < 1.f - DELTA);
			break;
		case GRAVITY_ZPOSITIVE:
			normalUpComponentIsAboveZero = (Normal.Z < 0.f);
			normalUpComponentIsBelowSmallNumber = (Normal.Z > KINDA_SMALL_NUMBER);
			bFloorOpposedToMovement = (Delta | FloorNormal) < 0.f && (-FloorNormal.Z < -(1.f - DELTA));
			break;
		default:
			break;
		}
		if (normalUpComponentIsAboveZero)
		{
			if (!IsWalkable(Hit))
			{
				//Normal = Normal.SafeNormal2D();
				Normal = GDSafeNormal2D(Normal);
			}
		}
		else if (normalUpComponentIsBelowSmallNumber)
		{
			// Don't push down into the floor when the impact is on the upper portion of the capsule.
			if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
			{
				//const FVector FloorNormal = CurrentFloor.HitResult.Normal;
				//const bool bFloorOpposedToMovement = (Delta | FloorNormal) < 0.f && (FloorNormal.Z < 1.f - DELTA);
				if (bFloorOpposedToMovement)
				{
					Normal = FloorNormal;
				}

				//Normal = Normal.SafeNormal2D();
				Normal = GDSafeNormal2D(Normal);
			}
		}
	}

	return Super::SlideAlongSurface(Delta, Time, Normal, Hit, bHandleImpact);
}

void UShooterCharacterMovement::TwoWallAdjust(FVector &Delta, const FHitResult& Hit, const FVector &OldHitNormal) const
{
	FVector InDelta = Delta;
	Super::TwoWallAdjust(Delta, Hit, OldHitNormal);

	if (IsMovingOnGround())
	{
		switch (GravityMode)
		{
		case GRAVITY_XNEGATIVE:
			//CASE FOR GRAVITY_XNEGATIVE
			// Allow slides up walkable surfaces, but not unwalkable ones (treat those as vertical barriers).
			if (Delta.X > 0.f)
			{
				if ((Hit.Normal.X >= WalkableFloorComponent || IsWalkable(Hit)) && Hit.Normal.X > KINDA_SMALL_NUMBER)
				{
					// Maintain horizontal velocity
					const float Time = (1.f - Hit.Time);
					const FVector ScaledDelta = Delta.SafeNormal() * InDelta.Size();
					//Delta = FVector(InDelta.X, InDelta.Y, ScaledDelta.Z / Hit.Normal.Z) * Time;
					Delta = FVector(ScaledDelta.X / Hit.Normal.X, InDelta.Y, InDelta.Z) * Time;
				}
				else
				{
					Delta.X = 0.f;
				}
			}
			else if (Delta.X < 0.f)
			{
				// Don't push down into the floor.
				if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
				{
					Delta.X = 0.f;
				}
			}
			break;

		case GRAVITY_XPOSITIVE:
			//CASE FOR GRAVITY_XPOSITIVE
			// Allow slides up walkable surfaces, but not unwalkable ones (treat those as vertical barriers).
			if (Delta.X < 0.f)
			{
				if ((abs(Hit.Normal.X) >= WalkableFloorComponent || IsWalkable(Hit)) && abs(Hit.Normal.X) > KINDA_SMALL_NUMBER)
				{
					// Maintain horizontal velocity
					const float Time = (1.f - Hit.Time);
					const FVector ScaledDelta = Delta.SafeNormal() * InDelta.Size();
					//Delta = FVector(InDelta.X, InDelta.Y, ScaledDelta.Z / Hit.Normal.Z) * Time;
					Delta = FVector(ScaledDelta.X / Hit.Normal.X, InDelta.Y, InDelta.Z) * Time;
				}
				else
				{
					Delta.X = 0.f;
				}
			}
			else if (Delta.X > 0.f)
			{
				// Don't push down into the floor.
				if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
				{
					Delta.X = 0.f;
				}
			}
			break;

		case GRAVITY_YNEGATIVE:
			//CASE FOR GRAVITY_YNEGATIVE
			// Allow slides up walkable surfaces, but not unwalkable ones (treat those as vertical barriers).
			if (Delta.Y > 0.f)
			{
				if ((Hit.Normal.Y >= WalkableFloorComponent || IsWalkable(Hit)) && Hit.Normal.Y > KINDA_SMALL_NUMBER)
				{
					// Maintain horizontal velocity
					const float Time = (1.f - Hit.Time);
					const FVector ScaledDelta = Delta.SafeNormal() * InDelta.Size();
					//Delta = FVector(InDelta.X, InDelta.Y, ScaledDelta.Z / Hit.Normal.Z) * Time;
					Delta = FVector(InDelta.X, ScaledDelta.Y / Hit.Normal.Y, InDelta.Z) * Time;
				}
				else
				{
					Delta.Y = 0.f;
				}
			}
			else if (Delta.Y < 0.f)
			{
				// Don't push down into the floor.
				if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
				{
					Delta.Y = 0.f;
				}
			}
			break;

		case GRAVITY_YPOSITIVE:
			//CASE FOR GRAVITY_YPOSITIVE
			// Allow slides up walkable surfaces, but not unwalkable ones (treat those as vertical barriers).
			if (Delta.Y < 0.f)
			{
				if ((abs(Hit.Normal.Y) >= WalkableFloorComponent || IsWalkable(Hit)) && abs(Hit.Normal.Y) > KINDA_SMALL_NUMBER)
				{
					// Maintain horizontal velocity
					const float Time = (1.f - Hit.Time);
					const FVector ScaledDelta = Delta.SafeNormal() * InDelta.Size();
					//Delta = FVector(InDelta.X, InDelta.Y, ScaledDelta.Z / Hit.Normal.Z) * Time;
					Delta = FVector(InDelta.X, ScaledDelta.Y / Hit.Normal.Y, InDelta.Z) * Time;
				}
				else
				{
					Delta.Y = 0.f;
				}
			}
			else if (Delta.Y > 0.f)
			{
				// Don't push down into the floor.
				if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
				{
					Delta.Y = 0.f;
				}
			}
			break;

		case GRAVITY_ZNEGATIVE:
			//CASE FOR GRAVITY_ZNEGATIVE (Original)
			// Allow slides up walkable surfaces, but not unwalkable ones (treat those as vertical barriers).
			if (Delta.Z > 0.f)
			{
				if ((Hit.Normal.Z >= WalkableFloorComponent || IsWalkable(Hit)) && Hit.Normal.Z > KINDA_SMALL_NUMBER)
				{
					// Maintain horizontal velocity
					const float Time = (1.f - Hit.Time);
					const FVector ScaledDelta = Delta.SafeNormal() * InDelta.Size();
					Delta = FVector(InDelta.X, InDelta.Y, ScaledDelta.Z / Hit.Normal.Z) * Time;
				}
				else
				{
					Delta.Z = 0.f;
				}
			}
			else if (Delta.Z < 0.f)
			{
				// Don't push down into the floor.
				if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
				{
					Delta.Z = 0.f;
				}
			}
			break;
		case GRAVITY_ZPOSITIVE:
			//CASE FOR GRAVITY_ZPOSITIVE
			// Allow slides up walkable surfaces, but not unwalkable ones (treat those as vertical barriers).
			if (Delta.Z < 0.f)
			{
				if ((abs(Hit.Normal.Z) >= WalkableFloorComponent || IsWalkable(Hit)) && abs(Hit.Normal.Z) > KINDA_SMALL_NUMBER)
				{
					// Maintain horizontal velocity
					const float Time = (1.f - Hit.Time);
					const FVector ScaledDelta = Delta.SafeNormal() * InDelta.Size();
					//const FVector ScaledDelta = GDSafeNormal2D()
					Delta = FVector(InDelta.X, InDelta.Y, ScaledDelta.Z / Hit.Normal.Z) * Time;
				}
				else
				{
					Delta.Z = 0.f;
				}
			}
			else if (Delta.Z > 0.f)
			{
				// Don't push down into the floor.
				if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
				{
					Delta.Z = 0.f;
				}
			}
			break;
		}
	}
}
/* funniest youtube comment on elliot rodger video
Spent his whole life gay and he didn't even know it. The girls knew he was gay that's why they stayed away. So sad...? -Hayden James
*/
FVector UShooterCharacterMovement::ComputeSlideVector(const FVector& InDelta, const float Time, const FVector& Normal, const FHitResult& Hit) const
{
	const bool bFalling = IsFalling();
	FVector Delta = InDelta;
	FVector Result = FVector(0.f, 0.f, 0.f);

	switch (GravityMode)
	{
	case GRAVITY_XNEGATIVE:
		// Don't make impacts on the upper hemisphere feel so much like a capsule
		if (bFalling && Delta.X > 0.f)
		{
			if (Hit.Normal.X < KINDA_SMALL_NUMBER)
			{
				float PawnRadius, PawnHalfHeight;
				CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
				const float UpperHemisphereX = UpdatedComponent->GetComponentLocation().X + PawnHalfHeight - PawnRadius;
				if (Hit.ImpactPoint.X > UpperHemisphereX + KINDA_SMALL_NUMBER && GDIsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnRadius))
				{
					Delta = AdjustUpperHemisphereImpact(Delta, Hit);
				}
			}
		}

		Result = Super::ComputeSlideVector(Delta, Time, Normal, Hit);

		// prevent boosting up slopes
		if (bFalling && Result.X > 0.f)
		{
			if (Delta.X < 0.f && (Hit.ImpactNormal.X < MAX_STEP_SIDE_Z))
			{
				// We were moving downward, but a slide was going to send us upward. We want to aim
				// straight down for the next move to make sure we get the most upward-facing opposing normal.
				Result = FVector(Delta.X, 0.f, 0.f);
			}
			else
			{
				Result.X = FMath::Min(Result.X, Delta.X * Time);
			}
		}

		return Result;
		break;
	case GRAVITY_XPOSITIVE:
		// Don't make impacts on the upper hemisphere feel so much like a capsule (original)
		if (bFalling && Delta.X < 0.f)
		{
			if (Hit.Normal.X > -KINDA_SMALL_NUMBER)
			{
				float PawnRadius, PawnHalfHeight;
				CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
				const float UpperHemisphereX = UpdatedComponent->GetComponentLocation().X - PawnHalfHeight + PawnRadius;
				if (Hit.ImpactPoint.X < UpperHemisphereX - KINDA_SMALL_NUMBER && GDIsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnRadius))
				{
					Delta = AdjustUpperHemisphereImpact(Delta, Hit);
				}
			}
		}

		Result = Super::ComputeSlideVector(Delta, Time, Normal, Hit);

		// prevent boosting up slopes
		if (bFalling && Result.X < 0.f)
		{
			if (Delta.X > 0.f && (Hit.ImpactNormal.X > -MAX_STEP_SIDE_Z))
			{
				// We were moving downward, but a slide was going to send us upward. We want to aim
				// straight down for the next move to make sure we get the most upward-facing opposing normal.
				Result = FVector(Delta.X, 0.f, 0.f);
			}
			else
			{
				Result.X = FMath::Min(Result.X, Delta.X * Time);
			}
		}
		
		return Result;
		break;
	case GRAVITY_YNEGATIVE:
		// Don't make impacts on the upper hemisphere feel so much like a capsule
		if (bFalling && Delta.Y > 0.f)
		{
			if (Hit.Normal.Y < KINDA_SMALL_NUMBER)
			{
				float PawnRadius, PawnHalfHeight;
				CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
				const float UpperHemisphereY = UpdatedComponent->GetComponentLocation().Y + PawnHalfHeight - PawnRadius;
				if (Hit.ImpactPoint.Y > UpperHemisphereY + KINDA_SMALL_NUMBER && GDIsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnRadius))
				{
					Delta = AdjustUpperHemisphereImpact(Delta, Hit);
				}
			}
		}

		Result = Super::ComputeSlideVector(Delta, Time, Normal, Hit);

		// prevent boosting up slopes
		if (bFalling && Result.Y > 0.f)
		{
			if (Delta.Y < 0.f && (Hit.ImpactNormal.Y < MAX_STEP_SIDE_Z))
			{
				// We were moving downward, but a slide was going to send us upward. We want to aim
				// straight down for the next move to make sure we get the most upward-facing opposing normal.
				Result = FVector(0.f, Delta.Y, 0.f);
			}
			else
			{
				Result.Y = FMath::Min(Result.Y, Delta.Y * Time);
			}
		}

		return Result;
		break;
	case GRAVITY_YPOSITIVE:
		
		// Don't make impacts on the upper hemisphere feel so much like a capsule (original)
		if (bFalling && Delta.Y < 0.f)
		{
			if (Hit.Normal.Y > -KINDA_SMALL_NUMBER)
			{
				float PawnRadius, PawnHalfHeight;
				CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
				const float UpperHemisphereY = UpdatedComponent->GetComponentLocation().Y - PawnHalfHeight + PawnRadius;
				if (Hit.ImpactPoint.Y < UpperHemisphereY - KINDA_SMALL_NUMBER && GDIsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnRadius))
				{
					Delta = AdjustUpperHemisphereImpact(Delta, Hit);
				}
			}
		}

		Result = Super::ComputeSlideVector(Delta, Time, Normal, Hit);

		// prevent boosting up slopes
		if (bFalling && Result.Y < 0.f)
		{
			if (Delta.Y > 0.f && (Hit.ImpactNormal.Y > -MAX_STEP_SIDE_Z))
			{
				// We were moving downward, but a slide was going to send us upward. We want to aim
				// straight down for the next move to make sure we get the most upward-facing opposing normal.
				Result = FVector(0.f, Delta.Y, 0.f);
			}
			else
			{
				Result.Y = FMath::Min(Result.Y, Delta.Y * Time);
			}
		}
		
		return Result;
		break;
		
	case GRAVITY_ZNEGATIVE:
		// Don't make impacts on the upper hemisphere feel so much like a capsule (original)
		if (bFalling && Delta.Z > 0.f)
		{
			if (Hit.Normal.Z < KINDA_SMALL_NUMBER)
			{
				float PawnRadius, PawnHalfHeight;
				CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
				const float UpperHemisphereZ = UpdatedComponent->GetComponentLocation().Z + PawnHalfHeight - PawnRadius;
				if (Hit.ImpactPoint.Z > UpperHemisphereZ + KINDA_SMALL_NUMBER && GDIsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnRadius))
				{
					Delta = AdjustUpperHemisphereImpact(Delta, Hit);
				}
			}
		}

		Result = Super::ComputeSlideVector(Delta, Time, Normal, Hit);

		// prevent boosting up slopes
		if (bFalling && Result.Z > 0.f)
		{
			if (Delta.Z < 0.f && (Hit.ImpactNormal.Z < MAX_STEP_SIDE_Z))
			{
				// We were moving downward, but a slide was going to send us upward. We want to aim
				// straight down for the next move to make sure we get the most upward-facing opposing normal.
				Result = FVector(0.f, 0.f, Delta.Z);
			}
			else
			{
				Result.Z = FMath::Min(Result.Z, Delta.Z * Time);
			}
		}

		return Result;
		break;
	case GRAVITY_ZPOSITIVE:
		// Don't make impacts on the upper hemisphere feel so much like a capsule (original)
		if (bFalling && Delta.Z < 0.f)
		{
			if (Hit.Normal.Z > -KINDA_SMALL_NUMBER)
			{
				float PawnRadius, PawnHalfHeight;
				CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
				const float UpperHemisphereZ = UpdatedComponent->GetComponentLocation().Z - PawnHalfHeight + PawnRadius;
				if (Hit.ImpactPoint.Z < UpperHemisphereZ - KINDA_SMALL_NUMBER && GDIsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnRadius))
				{
					Delta = AdjustUpperHemisphereImpact(Delta, Hit);
				}
			}
		}

		Result = Super::ComputeSlideVector(Delta, Time, Normal, Hit);

		// prevent boosting up slopes
		if (bFalling && Result.Z < 0.f)
		{
			if (Delta.Z > 0.f && (Hit.ImpactNormal.Z > -MAX_STEP_SIDE_Z))
			{
				// We were moving downward, but a slide was going to send us upward. We want to aim
				// straight down for the next move to make sure we get the most upward-facing opposing normal.
				Result = FVector(0.f, 0.f, Delta.Z);
			}
			else
			{
				Result.Z = FMath::Min(Result.Z, Delta.Z * Time);
			}
		}

		return Result;
		break;
	default:break;
	}

	return Result;
}


FVector UShooterCharacterMovement::AdjustUpperHemisphereImpact(const FVector& Delta, const FHitResult& Hit) const
{
	//return Super::AdjustUpperHemisphereImpact(Delta, Hit);
	float XScale = 0.f;
	float YScale = 0.f;
	float ZScale = 0.f;
	switch (GravityMode)
	{
	case GRAVITY_XNEGATIVE:
	case GRAVITY_XPOSITIVE:
		XScale = FMath::Clamp(1.f - (FMath::Abs(Hit.Normal.X) * UpperImpactNormalScale_DEPRECATED), 0.f, 1.f);
		return FVector(Delta.X * XScale, Delta.Y, Delta.Z);
		break;
	case GRAVITY_YNEGATIVE:
	case GRAVITY_YPOSITIVE:
		YScale = FMath::Clamp(1.f - (FMath::Abs(Hit.Normal.Y) * UpperImpactNormalScale_DEPRECATED), 0.f, 1.f);
		return FVector(Delta.X, Delta.Y * YScale, Delta.Z);
		break;
	case GRAVITY_ZNEGATIVE:
	case GRAVITY_ZPOSITIVE:
		ZScale = FMath::Clamp(1.f - (FMath::Abs(Hit.Normal.Z) * UpperImpactNormalScale_DEPRECATED), 0.f, 1.f);
		return FVector(Delta.X, Delta.Y, Delta.Z * ZScale);
		break;
	default:
		return FVector(0.f, 0.f, 0.f);
		break;
	}

}

float UShooterCharacterMovement::ImmersionDepth()
{
	float depth = 0.f;

	if (CharacterOwner && GetPhysicsVolume()->bWaterVolume)
	{
		const float CollisionHeight = CharacterOwner->GetSimpleCollisionRadius();
		const float CollisionRadius = CharacterOwner->GetSimpleCollisionHalfHeight();

		if ((CollisionHeight == 0.f) || (Buoyancy == 0.f))
		{
			depth = 1.f;
		}
		else
		{
			UBrushComponent* VolumeBrushComp = GetPhysicsVolume()->BrushComponent;
			FHitResult Hit(1.f);
			if (VolumeBrushComp)
			{
				//All we have to make gravity dependent here are the trace start and ends
				FVector TraceStart;
				FVector TraceEnd;

				switch (GravityMode)
				{
				case GRAVITY_XNEGATIVE:
					TraceStart = CharacterOwner->GetActorLocation() + FVector(CollisionHeight, 0.f, 0.f);
					TraceEnd = CharacterOwner->GetActorLocation() - FVector(CollisionHeight, 0.f, 0.f);
					break;
				case GRAVITY_XPOSITIVE:
					TraceEnd = CharacterOwner->GetActorLocation() + FVector(CollisionHeight, 0.f, 0.f);
					TraceStart = CharacterOwner->GetActorLocation() - FVector(CollisionHeight, 0.f, 0.f);
					break;
				case GRAVITY_YNEGATIVE:
					TraceStart = CharacterOwner->GetActorLocation() + FVector(0.f, CollisionHeight, 0.f);
					TraceEnd = CharacterOwner->GetActorLocation() - FVector(0.f, CollisionHeight, 0.f);
					break;
				case GRAVITY_YPOSITIVE:
					TraceEnd = CharacterOwner->GetActorLocation() + FVector(0.f, CollisionHeight, 0.f);
					TraceStart = CharacterOwner->GetActorLocation() - FVector(0.f, CollisionHeight, 0.f);
					break;
				case GRAVITY_ZNEGATIVE:
					TraceStart = CharacterOwner->GetActorLocation() + FVector(0.f, 0.f, CollisionHeight);
					TraceEnd = CharacterOwner->GetActorLocation() - FVector(0.f, 0.f, CollisionHeight);
					break;
				case GRAVITY_ZPOSITIVE:
					TraceEnd = CharacterOwner->GetActorLocation() + FVector(0.f, 0.f, CollisionHeight);
					TraceStart = CharacterOwner->GetActorLocation() - FVector(0.f, 0.f, CollisionHeight);
					break;
				default:break;
				}


				const static FName MovementComp_Character_ImmersionDepthName(TEXT("MovementComp_Character_ImmersionDepth"));
				FCollisionQueryParams NewTraceParams(MovementComp_Character_ImmersionDepthName, true);

				VolumeBrushComp->LineTraceComponent(Hit, TraceStart, TraceEnd, NewTraceParams);
			}

			depth = (Hit.Time == 1.f) ? 1.f : (1.f - Hit.Time);
		}
	}
	return depth;
}

/* TO DO FOR GRAVITY DEPENDENCE, COUPLED WITH PERFORMAIRCONTROL*/
void UShooterCharacterMovement::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("RequestDirectMove"));
	if (MoveVelocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return;
	}

	if (IsFalling())
	{
		const FVector FallVelocity = MoveVelocity.ClampMaxSize(GetModifiedMaxSpeed());
		PerformAirControl(FallVelocity, FallVelocity.Z);
		return;
	}

	RequestedVelocity = MoveVelocity;
	bHasRequestedVelocity = true;
	bRequestedMoveWithMaxSpeed = bForceMaxSpeed;

	if (IsMovingOnGround())
	{
		RequestedVelocity.Z = 0.0f;
	}
}


/* TO DO, BUT NOT SURE IF WE ARE EVER GOING TO NEED THIS FUNCTION FOR GRAVITY DEPENDENCE*/
void UShooterCharacterMovement::PhysFlying(float deltaTime, int32 Iterations)
{
	if (!HasRootMotion())
	{
		if (bCheatFlying && Acceleration.IsZero())
		{
			Velocity = FVector::ZeroVector;
		}
		const float Friction = 0.5f * GetPhysicsVolume()->FluidFriction;
		CalcVelocity(deltaTime, Friction, true, BrakingDecelerationFlying);
	}

	Iterations++;
	bJustTeleported = false;

	FVector OldLocation = CharacterOwner->GetActorLocation();
	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Adjusted, CharacterOwner->GetActorRotation(), true, Hit);

	if (Hit.Time < 1.f && CharacterOwner)
	{
		FVector GravDir = FVector(0.f, 0.f, -1.f);
		FVector VelDir = Velocity.SafeNormal();
		float UpDown = GravDir | VelDir;

		bool bSteppedUp = false;
		if ((FMath::Abs(Hit.ImpactNormal.Z) < 0.2f) && (UpDown < 0.5f) && (UpDown > -0.2f) && CanStepUp(Hit))
		{
			float stepZ = CharacterOwner->GetActorLocation().Z;
			bSteppedUp = StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit);
			if (bSteppedUp)
			{
				OldLocation.Z = CharacterOwner->GetActorLocation().Z + (OldLocation.Z - stepZ);
			}
		}

		if (!bSteppedUp)
		{
			//adjust and try again
			HandleImpact(Hit, deltaTime, Adjusted);
			SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
		}
	}

	if (!bJustTeleported && !HasRootMotion() && CharacterOwner)
	{
		Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / deltaTime;
	}
}


/* TO DO BUT AGAIN, MOST LIKELY NOT GOING TO NEED THIS FUNCTION TO BE GRAVITY DEPENDENT*/
void UShooterCharacterMovement::PhysSwimming(float deltaTime, int32 Iterations)
{
	float NetFluidFriction = 0.f;
	float Depth = ImmersionDepth();
	float NetBuoyancy = Buoyancy * Depth;
	if (!HasRootMotion() && (Velocity.Z > 0.5f*GetMaxSpeed()) && (NetBuoyancy != 0.f))
	{
		//damp positive Z out of water
		Velocity.Z = Velocity.Z * Depth;
	}
	Iterations++;
	FVector OldLocation = CharacterOwner->GetActorLocation();
	bJustTeleported = false;
	if (!HasRootMotion())
	{
		const float Friction = 0.5f * GetPhysicsVolume()->FluidFriction * Depth;
		CalcVelocity(deltaTime, Friction, true, BrakingDecelerationSwimming);
		Velocity.Z += GetGravityZ() * deltaTime * (1.f - NetBuoyancy);
	}

	FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);
	float remainingTime = deltaTime * Swim(Adjusted, Hit);

	//may have left water - if so, script might have set new physics mode
	if (!IsSwimming())
	{
		StartNewPhysics(remainingTime, Iterations);
		return;
	}

	if (Hit.Time < 1.f && CharacterOwner)
	{
		float stepZ = CharacterOwner->GetActorLocation().Z;
		FVector RealVelocity = Velocity;
		Velocity.Z = 1.f;	// HACK: since will be moving up, in case pawn leaves the water
		StepUp(-1.f*Hit.ImpactNormal, Adjusted * (1.f - Hit.Time), Hit);
		//may have left water - if so, script might have set new physics mode
		if (!IsSwimming())
		{
			StartNewPhysics(remainingTime, Iterations);
			return;
		}
		Velocity = RealVelocity;
		OldLocation.Z = CharacterOwner->GetActorLocation().Z + (OldLocation.Z - stepZ);
	}

	if (!HasRootMotion() && !bJustTeleported && (remainingTime < deltaTime) && CharacterOwner)
	{
		bool bWaterJump = !GetPhysicsVolume()->bWaterVolume;
		float velZ = Velocity.Z;
		Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / (deltaTime - remainingTime);
		if (bWaterJump)
		{
			Velocity.Z = velZ;
		}
	}

	if (!GetPhysicsVolume()->bWaterVolume && IsSwimming())
	{
		SetMovementMode(MOVE_Falling); //in case script didn't change it (w/ zone change)
	}

	//may have left water - if so, script might have set new physics mode
	if (!IsSwimming())
	{
		StartNewPhysics(remainingTime, Iterations);
	}
}
/*
void UShooterCharacterMovement::StartSwimming(FVector OldLocation, FVector OldVelocity, float timeTick, float remainingTime, int32 Iterations)
{
if (!HasRootMotion() && !bJustTeleported)
{
if (timeTick > 0.f)
{
Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / timeTick; //actual average velocity
}
Velocity = 2.f*Velocity - OldVelocity; //end velocity has 2* accel of avg
if (Velocity.SizeSquared() > FMath::Square(GetPhysicsVolume()->TerminalVelocity))
{
Velocity = Velocity.SafeNormal();
Velocity *= GetPhysicsVolume()->TerminalVelocity;
}
}
FVector End = FindWaterLine(CharacterOwner->GetActorLocation(), OldLocation);
float waterTime = 0.f;
if (End != CharacterOwner->GetActorLocation())
{
waterTime = timeTick * (End - CharacterOwner->GetActorLocation()).Size() / (CharacterOwner->GetActorLocation() - OldLocation).Size();
remainingTime += waterTime;
MoveUpdatedComponent(End - CharacterOwner->GetActorLocation(), CharacterOwner->GetActorRotation(), true);
}
if (!HasRootMotion() && CharacterOwner && (Velocity.Z > 2.f*SWIMBOBSPEED) && (Velocity.Z < 0.f)) //allow for falling out of water
{
Velocity.Z = SWIMBOBSPEED - Velocity.Size2D() * 0.7f; //smooth bobbing
}
if ((remainingTime > 0.01f) && (Iterations < 8) && CharacterOwner)
{
PhysSwimming(remainingTime, Iterations);
}
}
*/
void UShooterCharacterMovement::PhysFalling(float deltaTime, int32 Iterations)
{
	
	// Bound final 2d portion of velocity
	const float Speed2d = GDSize2D(Velocity);// Velocity.Size2D();
	float BoundSpeed = FMath::Max(Speed2d, GetModifiedMaxSpeed());

	//bound acceleration, falling object has minimal ability to impact acceleration
	FVector RealAcceleration = Acceleration;
	FHitResult Hit(1.f);

	switch (GravityMode)
	{
	case GRAVITY_XNEGATIVE:
	case GRAVITY_XPOSITIVE:
		Acceleration.X = 0.f;
	case GRAVITY_YNEGATIVE:
	case GRAVITY_YPOSITIVE:
		Acceleration.Y = 0.f;
		break;
	case GRAVITY_ZNEGATIVE:
	case GRAVITY_ZPOSITIVE:
		Acceleration.Z = 0.f;
		break;
	default:break;
	}

	if (!HasRootMotion())
	{
		
		// test for slope to avoid using air control to climb walls
		float TickAirControl = AirControl;
		if (TickAirControl > 0.0f && Acceleration.SizeSquared() > 0.f)
		{
			const float TestWalkTime = FMath::Max(deltaTime, 0.05f);
			const FVector TestWalk = ((TickAirControl * GetModifiedMaxAcceleration() * Acceleration.SafeNormal() + GetGravityZ()) * TestWalkTime + Velocity) * TestWalkTime;
			if (!TestWalk.IsZero())
			{
				static const FName FallingTraceParamsTag = FName(TEXT("PhysFalling"));
				FHitResult Result(1.f);
				FCollisionQueryParams CapsuleQuery(FallingTraceParamsTag, false, CharacterOwner);
				FCollisionResponseParams ResponseParam;
				InitCollisionParams(CapsuleQuery, ResponseParam);
				const FVector PawnLocation = CharacterOwner->GetActorLocation();
				const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
				FQuat CapsuleRotation = GetCharacterOwner()->GetCapsuleComponent()->GetComponentRotation().Quaternion();
				const bool bHit = GetWorld()->SweepSingle(Result, PawnLocation, PawnLocation + TestWalk, CapsuleRotation, CollisionChannel, GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleQuery, ResponseParam);
				if (bHit)
				{
					// Only matters if we can't walk there
					if (!IsValidLandingSpot(Result.Location, Result))
					{
						TickAirControl = 0.f;
					}
				}
			}
		}

		float MaxAccel = GetModifiedMaxAcceleration() * TickAirControl;

		// Boost maxAccel to increase player's control when falling
		if ((Speed2d < 10.f) && (TickAirControl > 0.f) && (TickAirControl <= 0.05f)) //allow initial burst
		{
			MaxAccel = MaxAccel + (10.f - Speed2d) / deltaTime;
		}

		Acceleration = Acceleration.ClampMaxSize(MaxAccel);
	}

	float remainingTime = deltaTime;
	float timeTick = 0.1f;

	while ((remainingTime > 0.f) && (Iterations < 8))
	{
		Iterations++;
		timeTick = (remainingTime > 0.05f)
			? FMath::Min(0.05f, remainingTime * 0.5f)
			: remainingTime;

		remainingTime -= timeTick;
		const FVector OldLocation = CharacterOwner->GetActorLocation();
		const FRotator PawnRotation = CharacterOwner->GetActorRotation();
		bJustTeleported = false;

		FVector OldVelocity = Velocity;

		// Apply input
		if (!HasRootMotion())
		{
			//const float SavedVelZ = Velocity.Z;
			float SavedVel = 0.0f;

			switch (GravityMode) {
			case GRAVITY_XNEGATIVE:
			case GRAVITY_XPOSITIVE:
				SavedVel = Velocity.X;
				Velocity.X = 0.f;
				CalcVelocity(timeTick, FallingLateralFriction, false, BrakingDecelerationFalling);
				Velocity.X = SavedVel;
				break;
			case GRAVITY_YNEGATIVE:
			case GRAVITY_YPOSITIVE:
				SavedVel = Velocity.Y;
				Velocity.Y = 0.f;
				CalcVelocity(timeTick, FallingLateralFriction, false, BrakingDecelerationFalling);
				Velocity.Y = SavedVel;
				break;
			case GRAVITY_ZNEGATIVE:
			case GRAVITY_ZPOSITIVE:
				SavedVel = Velocity.Z;
				Velocity.Z = 0.f;
				CalcVelocity(timeTick, FallingLateralFriction, false, BrakingDecelerationFalling);
				Velocity.Z = SavedVel;
				break;
			}
		}

		// Apply gravity - modified to be gravity dependant
		FVector oldAcceleration;
		switch (GravityMode) {
		case GRAVITY_XNEGATIVE: oldAcceleration = FVector(GetGravityZ(), 0.f, 0.f); break;
		case GRAVITY_XPOSITIVE: oldAcceleration = FVector(-GetGravityZ(), 0.f, 0.f); break;
		case GRAVITY_YNEGATIVE: oldAcceleration = FVector(0.f, GetGravityZ(), 0.f); break;
		case GRAVITY_YPOSITIVE: oldAcceleration = FVector(0.f, -GetGravityZ(), 0.f); break;
		case GRAVITY_ZNEGATIVE: oldAcceleration = FVector(0.f, 0.f, GetGravityZ()); break;
		case GRAVITY_ZPOSITIVE: oldAcceleration = FVector(0.f, 0.f, -GetGravityZ()); break;
		}
		Velocity = NewFallVelocity(Velocity, oldAcceleration, timeTick);

		bool VelocityIsDown = false;
		switch (GravityMode) {
		case GRAVITY_XNEGATIVE: VelocityIsDown = (Velocity.X <= 0.f); break;
		case GRAVITY_XPOSITIVE: VelocityIsDown = (Velocity.X >= 0.f); break;
		case GRAVITY_YNEGATIVE: VelocityIsDown = (Velocity.Y <= 0.f); break;
		case GRAVITY_YPOSITIVE: VelocityIsDown = (Velocity.Y >= 0.f); break;
		case GRAVITY_ZNEGATIVE: VelocityIsDown = (Velocity.Z <= 0.f); break;
		case GRAVITY_ZPOSITIVE: VelocityIsDown = (Velocity.Z >= 0.f); break;
		}
		if (bNotifyApex && CharacterOwner->Controller && VelocityIsDown)
		{
			// Just passed jump apex since now going down
			bNotifyApex = false;
			NotifyJumpApex();
		}

		if (!HasRootMotion())
		{
			// make sure not exceeding acceptable speed
			Velocity = GDClampMaxSize2D(Velocity, BoundSpeed);
		}

		FVector Adjusted = 0.5f*(OldVelocity + Velocity) * timeTick;
		SafeMoveUpdatedComponent(Adjusted, PawnRotation, true, Hit);

		if (!CharacterOwner || CharacterOwner->IsPendingKill())
		{
			return;
		}

		if (IsSwimming()) //just entered water
		{
			remainingTime = remainingTime + timeTick * (1.f - Hit.Time);
			StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
			return;
		}
		else if (Hit.Time < 1.f)
		{
			if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
			{
				remainingTime += timeTick * (1.f - Hit.Time);
				if (!bJustTeleported && (Hit.Time > 0.1f) && (Hit.Time * timeTick > 0.003f))
				{
					Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / (timeTick * Hit.Time);
				}
				ProcessLanded(Hit, remainingTime, Iterations);
				return;
			}
			else
			{
				HandleImpact(Hit, deltaTime, Adjusted);

				if (!RealAcceleration.IsZero())
				{
					// If we've changed physics mode, abort.
					if (!CharacterOwner || CharacterOwner->IsPendingKill() || !IsFalling())
					{
						return;
					}
				}

				const FVector OldHitNormal = Hit.Normal;
				const FVector OldHitImpactNormal = Hit.ImpactNormal;
				FVector Delta = ComputeSlideVector(Adjusted, 1.f - Hit.Time, OldHitNormal, Hit);

				if ((Delta | Adjusted) > 0.f)
				{
					SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
					if (Hit.Time < 1.f) //hit second wall
					{
						if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
						{
							remainingTime = 0.f;
							ProcessLanded(Hit, remainingTime, Iterations);
							return;
						}

						HandleImpact(Hit, timeTick, Delta);

						// If we've changed physics mode, abort.
						if (!CharacterOwner || CharacterOwner->IsPendingKill() || !IsFalling())
						{
							return;
						}

						TwoWallAdjust(Delta, Hit, OldHitNormal);

						// bDitch=true means that pawn is straddling two slopes, neither of which he can stand on
						bool bDitch = ((OldHitImpactNormal.Z > 0.f) && (Hit.ImpactNormal.Z > 0.f) && (FMath::Abs(Delta.Z) <= KINDA_SMALL_NUMBER) && ((Hit.ImpactNormal | OldHitImpactNormal) < 0.f));
						SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
						if (Hit.Time == 0)
						{
							// if we are stuck then try to side step
							FVector SideDelta = GDSafeNormal2D(OldHitNormal + Hit.ImpactNormal);// .SafeNormal2D();
							if (SideDelta.IsNearlyZero())
							{
								SideDelta = FVector(OldHitNormal.Y, -OldHitNormal.X, 0).SafeNormal();
							}
							SafeMoveUpdatedComponent(SideDelta, PawnRotation, true, Hit);
						}
						if (bDitch) {
							UE_LOG(LogCharacterMovement, Warning, TEXT("BDITCH=TRUE"));
						}
						if (bDitch || IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit) || Hit.Time == 0)
						{
							remainingTime = 0.f;
							ProcessLanded(Hit, remainingTime, Iterations);
							return;
						}
						else if (GetPerchRadiusThreshold() > 0.f && Hit.Time == 1.f && OldHitImpactNormal.Z >= GetWalkableFloorZ())
						{
							UE_LOG(LogCharacterMovement, Warning, TEXT("virtual ditch"));
							// We might be in a virtual 'ditch' within our perch radius. This is rare.
							const FVector PawnLocation = CharacterOwner->GetActorLocation();
							const float ZMovedDist = FMath::Abs(PawnLocation.Z - OldLocation.Z);
							const float MovedDist2DSq = GDSizeSquared2D(PawnLocation - OldLocation);//(PawnLocation - OldLocation).SizeSquared2D();
							if (ZMovedDist <= 0.2f * timeTick && MovedDist2DSq <= 4.f * timeTick)
							{
								Velocity.X += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
								Velocity.Y += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
								Velocity.Z = FMath::Max<float>(JumpZVelocity * 0.25f, 1.f);
								Delta = Velocity * timeTick;
								SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
							}
						}
					}
				}

				// Calculate average velocity based on actual movement after considering collisions
				if (!bJustTeleported)
				{
					// Use average velocity for XY movement (no acceleration except for air control in those axes), but want actual velocity in Z axis
					float OldVelX = 0.0f;
					float OldVelY = 0.0f;
					float OldVelZ = 0.0f;

					switch (GravityMode) {
					case GRAVITY_XNEGATIVE:
					case GRAVITY_XPOSITIVE:
						OldVelX = OldVelocity.X;
						OldVelocity = (CharacterOwner->GetActorLocation() - OldLocation) / timeTick;
						OldVelocity.X = OldVelX;
						break;
					case GRAVITY_YNEGATIVE:
					case GRAVITY_YPOSITIVE:
						OldVelY = OldVelocity.Y;
						OldVelocity = (CharacterOwner->GetActorLocation() - OldLocation) / timeTick;
						OldVelocity.Y = OldVelY;
						break;
					case GRAVITY_ZNEGATIVE:
					case GRAVITY_ZPOSITIVE:
						OldVelZ = OldVelocity.Z;
						OldVelocity = (CharacterOwner->GetActorLocation() - OldLocation) / timeTick;
						OldVelocity.Z = OldVelZ;
						break;
					}
					/*const float OldVelZ = OldVelocity.Z;
					OldVelocity = (CharacterOwner->GetActorLocation() - OldLocation) / timeTick;
					OldVelocity.Z = OldVelZ;*/
				}
			}
		}

		if (!HasRootMotion() && !bJustTeleported && MovementMode != MOVE_None)
		{
			// refine the velocity by figuring out the average actual velocity over the tick, and then the final velocity.
			// This particularly corrects for situations where level geometry affected the fall.
			Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / timeTick; //actual average velocity

			bool velocityCondition = false; //(Velocity.Z < OldVelocity.Z) || (OldVelocity.Z >= 0.f)
			switch (GravityMode) {
			case GRAVITY_XNEGATIVE: velocityCondition = (Velocity.X < OldVelocity.X) || (OldVelocity.X >= 0.f); break;
			case GRAVITY_XPOSITIVE: velocityCondition = (Velocity.X > OldVelocity.X) || (OldVelocity.X <= 0.f); break;
			case GRAVITY_YNEGATIVE: velocityCondition = (Velocity.Y < OldVelocity.Y) || (OldVelocity.Y >= 0.f); break;
			case GRAVITY_YPOSITIVE: velocityCondition = (Velocity.Y > OldVelocity.Y) || (OldVelocity.Y <= 0.f); break;
			case GRAVITY_ZNEGATIVE: velocityCondition = (Velocity.Z < OldVelocity.Z) || (OldVelocity.Z >= 0.f); break;
			case GRAVITY_ZPOSITIVE: velocityCondition = (Velocity.Z > OldVelocity.Z) || (OldVelocity.Z <= 0.f); break;
			}
			if (velocityCondition)
			{
				Velocity = 2.f*Velocity - OldVelocity; //end velocity has 2* accel of avg
			}

			if (GDSizeSquared2D(Velocity)/*Velocity.SizeSquared2D()*/ <= KINDA_SMALL_NUMBER * 10.f)
			{
				switch (GravityMode) {
				case GRAVITY_XNEGATIVE:
				case GRAVITY_XPOSITIVE:
					Velocity.Y = 0.f;
					Velocity.Z = 0.f;
					break;
				case GRAVITY_YNEGATIVE:
				case GRAVITY_YPOSITIVE:
					Velocity.X = 0.f;
					Velocity.Z = 0.f;
					break;
				case GRAVITY_ZNEGATIVE:
				case GRAVITY_ZPOSITIVE:
					Velocity.X = 0.f;
					Velocity.Y = 0.f;
					break;
				}
				
			}

			Velocity = Velocity.ClampMaxSize(GetPhysicsVolume()->TerminalVelocity);
		}
	}

	Acceleration = RealAcceleration;
}

/*
FVector UShooterCharacterMovement::GetLedgeMove(const FVector& OldLocation, const FVector& Delta, const FVector& GravDir)
{
// We have a ledge!
if (!CharacterOwner)
{
return FVector::ZeroVector;
}

// check which direction ledge goes
float DesiredDistSq = Delta.SizeSquared();

if (DesiredDistSq > 0.f)
{
FVector SideDir(Delta.Y, -1.f * Delta.X, 0.f);

// try left
if (CheckLedgeDirection(OldLocation, SideDir, GravDir))
{
return SideDir;
}

// try right
SideDir *= -1.f;
if (CheckLedgeDirection(OldLocation, SideDir, GravDir))
{
return SideDir;
}
}
return FVector::ZeroVector;
}
*/
void UShooterCharacterMovement::StartFalling(int32 Iterations, float remainingTime, float timeTick, const FVector& Delta, const FVector& subLoc)
{
	// start falling 
	const float DesiredDist = Delta.Size();
	const float ActualDist = GDSize2D((CharacterOwner->GetActorLocation() - subLoc));// (CharacterOwner->GetActorLocation() - subLoc).Size2D();
	remainingTime = (DesiredDist == 0.f)
		? 0.f
		: remainingTime + timeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));

	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
	case GRAVITY_XPOSITIVE:
		Velocity.X = 0.f;
		break;
	case GRAVITY_YNEGATIVE:
	case GRAVITY_YPOSITIVE:
		Velocity.Y = 0.f;
		break;
	case GRAVITY_ZNEGATIVE:
	case GRAVITY_ZPOSITIVE:
		Velocity.Z = 0.f;
		break;
	}
	if (IsMovingOnGround())
	{
		// This is to catch cases where the first frame of PIE is executed, and the
		// level is not yet visible. In those cases, the player will fall out of the
		// world... So, don't set MOVE_Falling straight away.
		if (!GIsEditor || (GetWorld()->HasBegunPlay() && (GetWorld()->GetTimeSeconds() >= 1.f)))
		{
			SetMovementMode(MOVE_Falling); //default behavior if script didn't change physics
		}
		else
		{
			// Make sure that the floor check code continues processing during this delay.
			bForceNextFloorCheck = true;
		}
	}
	StartNewPhysics(remainingTime, Iterations);
}

FVector UShooterCharacterMovement::ComputeGroundMovementDelta(const FVector& Delta, const FHitResult& RampHit, const bool bHitFromLineTrace) const {
	const FVector FloorNormal = RampHit.ImpactNormal;
	const FVector ContactNormal = RampHit.Normal;

	bool shouldDo = false;

	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
		shouldDo = (FloorNormal.X < (1.f - KINDA_SMALL_NUMBER) && FloorNormal.X > KINDA_SMALL_NUMBER && ContactNormal.X > KINDA_SMALL_NUMBER && !bHitFromLineTrace && IsWalkable(RampHit));
		break;
	case GRAVITY_XPOSITIVE:
		shouldDo = (FloorNormal.X > -(1.f - KINDA_SMALL_NUMBER) && FloorNormal.X < -KINDA_SMALL_NUMBER && ContactNormal.X < -KINDA_SMALL_NUMBER && !bHitFromLineTrace && IsWalkable(RampHit));
		break;
	case GRAVITY_YNEGATIVE:
		shouldDo = (FloorNormal.Y < (1.f - KINDA_SMALL_NUMBER) && FloorNormal.Y > KINDA_SMALL_NUMBER && ContactNormal.Y > KINDA_SMALL_NUMBER && !bHitFromLineTrace && IsWalkable(RampHit));
		break;
	case GRAVITY_YPOSITIVE:
		shouldDo = (FloorNormal.Y > -(1.f - KINDA_SMALL_NUMBER) && FloorNormal.Y < -KINDA_SMALL_NUMBER && ContactNormal.Y < -KINDA_SMALL_NUMBER && !bHitFromLineTrace && IsWalkable(RampHit));
		break;
	case GRAVITY_ZNEGATIVE:
		shouldDo = (FloorNormal.Z < (1.f - KINDA_SMALL_NUMBER) && FloorNormal.Z > KINDA_SMALL_NUMBER && ContactNormal.Z > KINDA_SMALL_NUMBER && !bHitFromLineTrace && IsWalkable(RampHit));
		break;
	case GRAVITY_ZPOSITIVE:
		shouldDo = (FloorNormal.Z > -(1.f - KINDA_SMALL_NUMBER) && FloorNormal.Z < -KINDA_SMALL_NUMBER && ContactNormal.Z < -KINDA_SMALL_NUMBER && !bHitFromLineTrace && IsWalkable(RampHit));
		break;
	}
	if (shouldDo)
	{
		const float FloorDotDelta = (FloorNormal | Delta);
		FVector RampMovement;

		switch (GravityMode) {
		case GRAVITY_XNEGATIVE:
			RampMovement = FVector(Delta.X * -FloorDotDelta / FloorNormal.X, Delta.Y, Delta.Z);
			break;
		case GRAVITY_XPOSITIVE:
			RampMovement = FVector(Delta.X * -FloorDotDelta / FloorNormal.X, Delta.Y, Delta.Z);
			break;
		case GRAVITY_YNEGATIVE:
			RampMovement = FVector(Delta.X, Delta.Y * -FloorDotDelta / FloorNormal.Y, Delta.Z);
			break;
		case GRAVITY_YPOSITIVE:
			RampMovement = FVector(Delta.X, Delta.Y * -FloorDotDelta / FloorNormal.Y, Delta.Z);
			break;
		case GRAVITY_ZNEGATIVE:
			RampMovement = FVector(Delta.X, Delta.Y, Delta.Z *-FloorDotDelta / FloorNormal.Z);
			break;
		case GRAVITY_ZPOSITIVE:
			RampMovement = FVector(Delta.X, Delta.Y, Delta.Z *-FloorDotDelta / FloorNormal.Z);
			break;
		}
		//FVector RampMovement(Delta.X, Delta.Y, Delta.Z/*-FloorDotDelta / FloorNormal.Z*/);
		UE_LOG(LogCharacterMovement, Warning, TEXT("ShouldDoRampMovement"));
		if (bMaintainHorizontalGroundVelocity)
		{
			return RampMovement;
		}
		else
		{
			return RampMovement.SafeNormal() * Delta.Size();
		}
	}

	return Delta;
}

void UShooterCharacterMovement::MoveAlongFloor(const FVector& InVelocity, const float DeltaSeconds, FStepDownResult* OutStepDownResult)
{

	FVector Delta;
	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
	case GRAVITY_XPOSITIVE:
		Delta = FVector(0.f, InVelocity.Y, InVelocity.Z) * DeltaSeconds;
		break;
	case GRAVITY_YNEGATIVE:
	case GRAVITY_YPOSITIVE:
		Delta = FVector(InVelocity.X, 0.f, InVelocity.Z) * DeltaSeconds;
		break;
	case GRAVITY_ZNEGATIVE:
	case GRAVITY_ZPOSITIVE:
		Delta = FVector(InVelocity.X, InVelocity.Y, 0.f) * DeltaSeconds;
		break;

	}

	if (!CurrentFloor.IsWalkableFloor())
	{
		return;
	}

	FHitResult Hit(1.f);
	FVector RampVector = ComputeGroundMovementDelta(Delta, CurrentFloor.HitResult, CurrentFloor.bLineTrace);
	SafeMoveUpdatedComponent(RampVector, CharacterOwner->GetActorRotation(), true, Hit);
	if (Hit.bStartPenetrating)
	{
		UE_LOG(LogCharacterMovement, Log, TEXT("%s is stuck and failed to move!"), *CharacterOwner->GetName());

		// Don't update velocity based on our (failed) change in position this update since we're stuck.
		bJustTeleported = true;
	}

	if (Hit.IsValidBlockingHit())
	{
		// See if we impacted something (most likely another ramp, but possibly a barrier). Try to slide along it as well.
		float TimeApplied = Hit.Time;

		bool normalHasSomeUp = false;
		switch (GravityMode) {
		case GRAVITY_XNEGATIVE: normalHasSomeUp = (Hit.Normal.X > KINDA_SMALL_NUMBER); break;
		case GRAVITY_XPOSITIVE: normalHasSomeUp = (Hit.Normal.X < -KINDA_SMALL_NUMBER); break;
		case GRAVITY_YNEGATIVE: normalHasSomeUp = (Hit.Normal.Y > KINDA_SMALL_NUMBER); break;
		case GRAVITY_YPOSITIVE: normalHasSomeUp = (Hit.Normal.Y < -KINDA_SMALL_NUMBER); break;
		case GRAVITY_ZNEGATIVE: normalHasSomeUp = (Hit.Normal.Z > KINDA_SMALL_NUMBER); break;
		case GRAVITY_ZPOSITIVE: normalHasSomeUp = (Hit.Normal.Z < -KINDA_SMALL_NUMBER); break;
		}
		if ((Hit.Time > 0.f) && normalHasSomeUp && IsWalkable(Hit))
		{
			const float PreSlideTimeRemaining = 1.f - Hit.Time;
			RampVector = ComputeGroundMovementDelta(Delta * PreSlideTimeRemaining, Hit, false);
			SafeMoveUpdatedComponent(RampVector, CharacterOwner->GetActorRotation(), true, Hit);

			const float SecondHitPercent = Hit.Time * (1.f - TimeApplied);
			TimeApplied = FMath::Clamp(TimeApplied + SecondHitPercent, 0.f, 1.f);
		}

		if (Hit.IsValidBlockingHit())
		{
			if (CanStepUp(Hit) || (CharacterOwner->GetMovementBase() != NULL && CharacterOwner->GetMovementBase()->GetOwner() == Hit.GetActor()))
			{
				// hit a barrier, try to step up
				UE_LOG(LogCharacterMovement, Warning, TEXT("Hit.IsValidBlockingHit AND CanStepUp(Hit) etc..... IN MoveAlongFloor()"));
				FVector GravDir(0.f, 0.f, -1.f);
				switch (GravityMode) {
				case GRAVITY_XNEGATIVE:
					GravDir = FVector(-1.f, 0.f, 0.f);
					break;
				case GRAVITY_XPOSITIVE:
					GravDir = FVector(1.f, 0.f, 0.f);
					break;
				case GRAVITY_YNEGATIVE:
					GravDir = FVector(0.f, -1.f, 0.f);
					break;
				case GRAVITY_YPOSITIVE:
					GravDir = FVector(0.f, 1.f, 0.f);
					break;
				case GRAVITY_ZNEGATIVE:
					GravDir = FVector(0.f, 0.f, -1.f);
					break;
				case GRAVITY_ZPOSITIVE:
					GravDir = FVector(0.f, 0.f, 1.f);
					break;
				}
				if (!StepUp(GravDir, Delta * (1.f - TimeApplied), Hit, OutStepDownResult))
				{
					UE_LOG(LogCharacterMovement, Verbose, TEXT("- StepUp (ImpactNormal %s, Normal %s"), *Hit.ImpactNormal.ToString(), *Hit.Normal.ToString());
					HandleImpact(Hit, DeltaSeconds, Delta);
					SlideAlongSurface(Delta, 1.f - TimeApplied, Hit.Normal, Hit, true);
				}
				else
				{
					// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
					UE_LOG(LogCharacterMovement, Verbose, TEXT("+ StepUp (ImpactNormal %s, Normal %s"), *Hit.ImpactNormal.ToString(), *Hit.Normal.ToString());
					bJustTeleported |= !bMaintainHorizontalGroundVelocity;
				}
			}
			else if (Hit.Component.IsValid() && !Hit.Component.Get()->CanBeBaseForCharacter(CharacterOwner))
			{
				HandleImpact(Hit, DeltaSeconds, Delta);
				SlideAlongSurface(Delta, 1.f - TimeApplied, Hit.Normal, Hit, true);
			}
		}
	}
}

void UShooterCharacterMovement::MaintainHorizontalGroundVelocity()
{
	bool shouldCorrect = false;
	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
	case GRAVITY_XPOSITIVE:
		shouldCorrect = (Velocity.X != 0.f);
		break;
	case GRAVITY_YNEGATIVE:
	case GRAVITY_YPOSITIVE:
		shouldCorrect = (Velocity.Y != 0.f);
		break;
	case GRAVITY_ZNEGATIVE:
	case GRAVITY_ZPOSITIVE:
		shouldCorrect = (Velocity.Z != 0.f);
		break;
	}
	if (shouldCorrect)
	{
		if (bMaintainHorizontalGroundVelocity)
		{
			// Ramp movement already maintained the velocity, so we just want to remove the vertical component.
			switch (GravityMode) {
			case GRAVITY_XNEGATIVE:
			case GRAVITY_XPOSITIVE:
				Velocity.X = 0.f;
				break;
			case GRAVITY_YNEGATIVE:
			case GRAVITY_YPOSITIVE:
				Velocity.Y = 0.f;
				break;
			case GRAVITY_ZNEGATIVE:
			case GRAVITY_ZPOSITIVE:
				Velocity.Z = 0.f;
				break;
			}
		}
		else
		{
			// Rescale velocity to be horizontal but maintain magnitude of last update.
			Velocity = GDSafeNormal2D(Velocity) * Velocity.Size();
		}
	}
}


void UShooterCharacterMovement::PhysWalking(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if ((!CharacterOwner || !CharacterOwner->Controller) && !bRunPhysicsWithNoController && !HasRootMotion())
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	if (!UpdatedComponent->IsCollisionEnabled())
	{
		SetMovementMode(MOVE_Walking);
		return;
	}

	checkf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN before Iteration (%s: %s)\n%s"), *GetPathNameSafe(this), *GetPathNameSafe(GetOuter()), *Velocity.ToString());

	bJustTeleported = false;
	bool bCheckedFall = false;
	bool bTriedLedgeMove = false;
	float remainingTime = deltaTime;

	// Perform the move
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasRootMotion()))
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save current values
		UPrimitiveComponent * const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FFindFloorResult OldFloor = CurrentFloor;

		// Ensure velocity is horizontal.
		MaintainHorizontalGroundVelocity();
		//Velocity.Z = 0.f;
		const FVector OldVelocity = Velocity;

		// Apply acceleration
		//bound acceleration
		switch (GravityMode) {
		case GRAVITY_XNEGATIVE:
		case GRAVITY_XPOSITIVE:
			Acceleration.X = 0.f;
			break;
		case GRAVITY_YNEGATIVE:
		case GRAVITY_YPOSITIVE:
			Acceleration.Y = 0.f;
			break;
		case GRAVITY_ZNEGATIVE:
		case GRAVITY_ZPOSITIVE:
			Acceleration.Z = 0.f;
			break;
		}

		if (!HasRootMotion())
		{
			CalcVelocity(timeTick, GroundFriction, false, BrakingDecelerationWalking);
		}
		checkf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN after CalcVelocity (%s: %s)\n%s"), *GetPathNameSafe(this), *GetPathNameSafe(GetOuter()), *Velocity.ToString());

		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = timeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;

		if (bZeroDelta)
		{
			remainingTime = 0.f;
		}
		else
		{
			// try to move forward
			MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);

			if (IsFalling())
			{
				// pawn decided to jump up
				const float DesiredDist = Delta.Size();
				if (DesiredDist > KINDA_SMALL_NUMBER)
				{
					const float ActualDist = (CharacterOwner->GetActorLocation() - OldLocation).Size2D();
					remainingTime += timeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));
				}
				StartNewPhysics(remainingTime, Iterations);
				return;
			}
			else if (IsSwimming()) //just entered water
			{
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
				return;
			}
		}

		// Update floor.
		// StepUp might have already done it for us.
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
		}

		// check for ledges here
		const bool bCheckLedges = !CanWalkOffLedges();
		if (bCheckLedges && !CurrentFloor.IsWalkableFloor())
		{
			// calculate possible alternate movement
			FVector GravDir(0.f, 0.f, 0.f);
			switch (GravityMode) {
			case GRAVITY_XNEGATIVE:
				GravDir = FVector(-1.f, 0.f, 0.f);
				break;
			case GRAVITY_XPOSITIVE:
				GravDir = FVector(1.f, 0.f, 0.f);
				break;
			case GRAVITY_YNEGATIVE:
				GravDir = FVector(0.f, -1.f, 0.f);
				break;
			case GRAVITY_YPOSITIVE:
				GravDir = FVector(0.f, 1.f, 0.f);
				break;
			case GRAVITY_ZNEGATIVE:
				GravDir = FVector(0.f, 0.f, -1.f);
				break;
			case GRAVITY_ZPOSITIVE:
				GravDir = FVector(0.f, 0.f, 1.f);
				break;
			}
			
			const FVector NewDelta = bTriedLedgeMove ? FVector::ZeroVector : GetLedgeMove(OldLocation, Delta, GravDir);
			if (!NewDelta.IsZero())
			{
				// first revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);

				// avoid repeated ledge moves if the first one fails
				bTriedLedgeMove = true;

				// Try new movement direction
				Velocity = NewDelta / timeTick;
				remainingTime += timeTick;
				continue;
			}
			else
			{
				// see if it is OK to jump
				// @todo collision : only thing that can be problem is that oldbase has world collision on
				bool bMustJump = bZeroDelta || (OldBase == NULL || (!OldBase->IsCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;

				// revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
				remainingTime = 0.f;
				break;
			}
		}
		else
		{
			// Validate the floor check
			if (CurrentFloor.IsWalkableFloor())
			{
				if (ShouldCatchAir(OldFloor, CurrentFloor))
				{
					CharacterOwner->OnWalkingOffLedge();
					if (IsMovingOnGround())
					{
						// If still walking, then fall. If not, assume the user set a different mode they want to keep.
						StartFalling(Iterations, remainingTime, timeTick, Delta, OldLocation);
					}
					return;
				}

				AdjustFloorHeight();
				SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
			}
			else if (CurrentFloor.HitResult.bStartPenetrating && remainingTime <= 0.f)
			{
				// The floor check failed because it started in penetration
				// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
				FHitResult Hit(CurrentFloor.HitResult);
				FVector TraceVector(0.f, 0.f, 0.f);
				switch (GravityMode) {
				case GRAVITY_XNEGATIVE: TraceVector = FVector(MAX_FLOOR_DIST, 0.f, 0.f); break;
				case GRAVITY_XPOSITIVE: TraceVector = FVector(-MAX_FLOOR_DIST, 0.f, 0.f); break;
				case GRAVITY_YNEGATIVE: TraceVector = FVector(0.f, MAX_FLOOR_DIST, 0.f); break;
				case GRAVITY_YPOSITIVE: TraceVector = FVector(0.f, -MAX_FLOOR_DIST, 0.f); break;
				case GRAVITY_ZNEGATIVE: TraceVector = FVector(0.f, 0.f, MAX_FLOOR_DIST); break;
				case GRAVITY_ZPOSITIVE: TraceVector = FVector(0.f, 0.f, -MAX_FLOOR_DIST); break;
				}
				Hit.TraceEnd = Hit.TraceStart + TraceVector;
				const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
				ResolvePenetration(RequestedAdjustment, Hit, CharacterOwner->GetActorRotation());
			}

			// check if just entered water
			if (IsSwimming())
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, Iterations);
				return;
			}

			// See if we need to start falling.
			if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
			{
				const bool bMustJump = bJustTeleported || bZeroDelta || (OldBase == NULL || (!OldBase->IsCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;
			}
		}


		// Allow overlap events and such to change physics state and velocity
		if (IsMovingOnGround())
		{
			// Make velocity reflect actual move
			if (!bJustTeleported && !HasRootMotion() && timeTick >= MIN_TICK_TIME)
			{
				Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / timeTick;
			}
		}

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (CharacterOwner->GetActorLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}
	}

	if (IsMovingOnGround())
	{
		MaintainHorizontalGroundVelocity();
	}
}
/*
void UShooterCharacterMovement::PhysWalking(float deltaTime, int32 Iterations)
{
	if ((!CharacterOwner || !CharacterOwner->Controller) && !bRunPhysicsWithNoController && !HasRootMotion())
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	if (!UpdatedComponent->IsCollisionEnabled())
	{
		SetMovementMode(MOVE_Walking);
		return;
	}

	// Ensure velocity is horizontal.
	MaintainHorizontalGroundVelocity();

	//bound acceleration
	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
	case GRAVITY_XPOSITIVE:
		Acceleration.X = 0.f;
		break;
	case GRAVITY_YNEGATIVE:
	case GRAVITY_YPOSITIVE:
		Acceleration.Y = 0.f;
		break;
	case GRAVITY_ZNEGATIVE:
	case GRAVITY_ZPOSITIVE:
		Acceleration.Z = 0.f;
		break;
	}

	if (!HasRootMotion())
	{
		CalcVelocity(deltaTime, GroundFriction, false, BrakingDecelerationWalking);
	}

	FVector DesiredMove = Velocity;
	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
	case GRAVITY_XPOSITIVE:
		DesiredMove.X = 0.f;
		break;
	case GRAVITY_YNEGATIVE:
	case GRAVITY_YPOSITIVE:
		DesiredMove.Y = 0.f;
		break;
	case GRAVITY_ZNEGATIVE:
	case GRAVITY_ZPOSITIVE:
		DesiredMove.Z = 0.f;
		break;
	}


	//Perform the move
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FFindFloorResult OldFloor = CurrentFloor;
	UPrimitiveComponent* const OldBase = CharacterOwner->GetMovementBase();
	const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
	bJustTeleported = false;
	bool bCheckedFall = false;
	float remainingTime = deltaTime;

	while ((remainingTime > 0.f) && (Iterations < 8) && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasRootMotion()))
	{
		Iterations++;
		// subdivide moves to be no longer than 0.05 seconds
		const float timeTick = (remainingTime > 0.05f) ? FMath::Min(0.05f, remainingTime * 0.5f) : remainingTime;
		remainingTime -= timeTick;
		const FVector Delta = timeTick * DesiredMove;
		const FVector subLoc = CharacterOwner->GetActorLocation();
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;

		if (bZeroDelta)
		{
			remainingTime = 0.f;
		}
		else
		{
			// @todo hunting down NaN TTP 304692
			checkf(!Delta.ContainsNaN(), TEXT("PhysWalking: NewTransform contains NaN (%s: %s)\n%s"), *GetPathNameSafe(this), *GetPathNameSafe(GetOuter()), *Delta.ToString());
			// @todo hunting down NaN TTP 304692

			// try to move forward
			MoveAlongFloor(DesiredMove, timeTick, &StepDownResult);

			if (IsFalling())
			{
				// pawn decided to jump up
				const float ActualDist = GDSize2D((CharacterOwner->GetActorLocation() - subLoc));// (CharacterOwner->GetActorLocation() - subLoc).Size2D();
				const float DesiredDist = Delta.Size();
				remainingTime += timeTick * (1 - FMath::Min(1.f, ActualDist / DesiredDist));
				StartNewPhysics(remainingTime, Iterations);
				return;
			}
			else if (IsSwimming()) //just entered water
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, Iterations);
				return;
			}
		}

		// Update floor.
		// StepUp might have already done it for us.
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
		}

		// check for ledges here
		const bool bCheckLedges = !CanWalkOffLedges();
		if (bCheckLedges && !CurrentFloor.IsWalkableFloor())
		{
			// calculate possible alternate movement
			const FVector GravDir = FVector(0.f, 0.f, -1.f);
			FVector NewDelta = GetLedgeMove(OldLocation, Delta, GravDir);
			if (!NewDelta.IsZero())
			{
				// first revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);

				// @todo hunting down NaN TTP 304692
				check(timeTick != 0.f);
				// redo move using NewDelta
				DesiredMove = NewDelta / timeTick;
				remainingTime += timeTick;
			}
			else
			{
				// see if it is OK to jump
				// @todo collision : only thing that can be problem is that oldbase has world collision on
				bool bMustJump = bZeroDelta || (OldBase == NULL || ((!OldBase->IsCollisionEnabled()) && !OldBase->IsWorldGeometry()));
				if ((bMustJump || !bCheckedFall) && CheckFall(CurrentFloor.HitResult, Delta, subLoc, remainingTime, timeTick, Iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;

				// revert this move
				//UE_LOG(LogCharacterMovement, Log, TEXT("%s REVERT MOVE 1"), *CharacterOwner->GetName());
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
				remainingTime = 0.f;
			}
		}
		else
		{
			// Validate the floor check
			if (CurrentFloor.IsWalkableFloor())
			{
				const bool bBaseChanged = (CurrentFloor.HitResult.Component != CharacterOwner->GetMovementBase());
				if (bBaseChanged || CurrentFloor.FloorDist > MAX_FLOOR_DIST)
				{
					if (ShouldCatchAir(OldFloor.HitResult.ImpactNormal, CurrentFloor.HitResult.ImpactNormal))
					{
						StartFalling(Iterations, remainingTime, timeTick, Delta, subLoc);
						return;
					}
					else
					{
						AdjustFloorHeight();
						if (bBaseChanged)
						{
							SetBase(CurrentFloor.HitResult.Component.Get());
						}
					}
				}
				else if (CurrentFloor.FloorDist < MIN_FLOOR_DIST)
				{
					AdjustFloorHeight();
				}
			}
			else if (CurrentFloor.HitResult.bStartPenetrating && remainingTime <= 0.f)
			{
				// The floor check failed because it started in penetration
				// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
				FHitResult Hit(CurrentFloor.HitResult);
				Hit.TraceEnd = Hit.TraceStart + FVector(0.f, 0.f, MAX_FLOOR_DIST);
				const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
				ResolvePenetration(RequestedAdjustment, Hit, CharacterOwner->GetActorRotation());
			}

			// check if just entered water
			if (IsSwimming())
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, Iterations);
				return;
			}

			// See if we need to start falling.
			if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
			{
				const bool bMustJump = bJustTeleported || bZeroDelta || (OldBase == NULL || (!OldBase->IsCollisionEnabled() && !OldBase->IsWorldGeometry()));
				if ((bMustJump || !bCheckedFall) && CheckFall(CurrentFloor.HitResult, Delta, subLoc, remainingTime, timeTick, Iterations, bMustJump))
				{
					return;
				}
				bCheckedFall = true;
			}
		}
	}

	// Allow overlap events and such to change physics state and velocity
	if (IsMovingOnGround())
	{
		// Make velocity reflect actual move
		if (!bJustTeleported && !HasRootMotion())
		{
			Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / deltaTime;
		}

		MaintainHorizontalGroundVelocity();
	}
}
*/
void UShooterCharacterMovement::AdjustFloorHeight()
{
	// If we have a floor check that hasn't hit anything, don't adjust height.
	if (!CurrentFloor.bBlockingHit)
	{
		return;
	}

	const float OldFloorDist = CurrentFloor.FloorDist;
	if (CurrentFloor.bLineTrace && OldFloorDist < MIN_FLOOR_DIST)
	{
		// This would cause us to scale unwalkable walls
		return;
		
	}

	// Move up or down to maintain floor height.
	if (OldFloorDist < MIN_FLOOR_DIST || OldFloorDist > MAX_FLOOR_DIST)
	{
		FHitResult AdjustHit(1.f);
		float InitialUpComponent = 0.f;
		switch (GravityMode) {
		case GRAVITY_XNEGATIVE: case GRAVITY_XPOSITIVE: InitialUpComponent = UpdatedComponent->GetComponentLocation().X; break;
		case GRAVITY_YNEGATIVE: case GRAVITY_YPOSITIVE: InitialUpComponent = UpdatedComponent->GetComponentLocation().Y; break;
		case GRAVITY_ZNEGATIVE: case GRAVITY_ZPOSITIVE: InitialUpComponent = UpdatedComponent->GetComponentLocation().Z; break;
		}

		const float AvgFloorDist = (MIN_FLOOR_DIST + MAX_FLOOR_DIST) * 0.5f;
		const float MoveDist = AvgFloorDist - OldFloorDist;

		FVector MoveVector = FVector(0.f, 0.f, 0.f);
		switch (GravityMode) {
		case GRAVITY_XNEGATIVE: MoveVector = FVector(MoveDist, 0.f, 0.f); break;
		case GRAVITY_XPOSITIVE: MoveVector = FVector(-MoveDist, 0.f, 0.f); break;
		case GRAVITY_YNEGATIVE: MoveVector = FVector(0.f, MoveDist, 0.f); break;
		case GRAVITY_YPOSITIVE: MoveVector = FVector(0.f, -MoveDist, 0.f); break;
		case GRAVITY_ZNEGATIVE: MoveVector = FVector(0.f, 0.f, MoveDist); break;
		case GRAVITY_ZPOSITIVE: MoveVector = FVector(0.f, 0.f, -MoveDist); break;
		}
		SafeMoveUpdatedComponent(MoveVector, CharacterOwner->GetActorRotation(), true, AdjustHit);
		UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("Adjust floor height %.3f (Hit = %d)"), MoveDist, AdjustHit.bBlockingHit);

		if (!AdjustHit.IsValidBlockingHit())
		{
			CurrentFloor.FloorDist += MoveDist;
		}
		else if (MoveDist > 0.f)
		{
			// If moving up, use the actual impact location, not the pulled back time/location.
			float FloorDistTime = 0.f;
			switch (GravityMode) {
			case GRAVITY_XNEGATIVE: case GRAVITY_XPOSITIVE: FloorDistTime = FMath::Abs((InitialUpComponent - AdjustHit.Location.X) / MoveDist); break;
			case GRAVITY_YNEGATIVE: case GRAVITY_YPOSITIVE: FloorDistTime = FMath::Abs((InitialUpComponent - AdjustHit.Location.Y) / MoveDist); break;
			case GRAVITY_ZNEGATIVE: case GRAVITY_ZPOSITIVE: FloorDistTime = FMath::Abs((InitialUpComponent - AdjustHit.Location.Z) / MoveDist); break;
			}
			CurrentFloor.FloorDist += MoveDist * FloorDistTime;
		}
		else
		{
			check(MoveDist < 0.f);
			/*const float CurrentZ = UpdatedComponent->GetComponentLocation().Z;
			CurrentFloor.FloorDist = CurrentZ - AdjustHit.Location.Z;*/

			switch (GravityMode) {
			case GRAVITY_XNEGATIVE:
			case GRAVITY_XPOSITIVE:
				CurrentFloor.FloorDist = UpdatedComponent->GetComponentLocation().X - AdjustHit.Location.X;
				break;
			case GRAVITY_YNEGATIVE:
			case GRAVITY_YPOSITIVE:
				CurrentFloor.FloorDist = UpdatedComponent->GetComponentLocation().Y - AdjustHit.Location.Y;
				break;
			case GRAVITY_ZNEGATIVE:
			case GRAVITY_ZPOSITIVE:
				CurrentFloor.FloorDist = UpdatedComponent->GetComponentLocation().Z - AdjustHit.Location.Z;
				break;
			}
			if (IsWalkable(AdjustHit))
			{
				CurrentFloor.SetFromSweep(AdjustHit, CurrentFloor.FloorDist, true);
			}
		}

		// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
		// Also avoid it if we moved out of penetration
		bJustTeleported |= !bMaintainHorizontalGroundVelocity || (OldFloorDist < 0.f);
	}
}

void UShooterCharacterMovement::PhysicsRotation(float DeltaTime)
{

	
	return;
	if (!CharacterOwner || !CharacterOwner->Controller)
	{
		return;
	}

	const FRotator CurrentRotation = CharacterOwner->GetActorRotation();
	FRotator DeltaRot = GetDeltaRotation(DeltaTime);
	FRotator DesiredRotation = CurrentRotation;

	if (bOrientRotationToMovement)
	{
		DesiredRotation = ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRot);
	}
	else if (bUseControllerDesiredRotation)
	{
		DesiredRotation = CharacterOwner->Controller->GetDesiredRotation();
	}
	else
	{
		return;
	}

	// Always remain vertical when walking or falling.
	if (IsMovingOnGround() || IsFalling())
	{
		DesiredRotation.Pitch = 0;
		DesiredRotation.Roll = 0;
	}

	if (CurrentRotation.GetDenormalized().Equals(DesiredRotation.GetDenormalized(), 0.01f))
	{
		return;
	}

	// Accumulate a desired new rotation.
	FRotator NewRotation = CurrentRotation;

	//YAW
	if (DesiredRotation.Yaw != CurrentRotation.Yaw)
	{
		NewRotation.Yaw = FMath::FixedTurn(CurrentRotation.Yaw, DesiredRotation.Yaw, DeltaRot.Yaw);
	}

	// PITCH
	if (DesiredRotation.Pitch != CurrentRotation.Pitch)
	{
		NewRotation.Pitch = FMath::FixedTurn(CurrentRotation.Pitch, DesiredRotation.Pitch, DeltaRot.Pitch);
	}

	// ROLL
	if (DesiredRotation.Roll != CurrentRotation.Roll)
	{
		NewRotation.Roll = FMath::FixedTurn(CurrentRotation.Roll, DesiredRotation.Roll, DeltaRot.Roll);
	}

	//UpdatedComponent->AngularVelocity = CharAngularVelocity( CurrentRotation, NewRotation, deltaTime );

	// Set the new rotation.
	if (!NewRotation.Equals(CurrentRotation.GetDenormalized(), 0.01f))
	{
		MoveUpdatedComponent(FVector::ZeroVector, NewRotation, true);
	}
}

void UShooterCharacterMovement::PhysicsVolumeChanged(APhysicsVolume* NewVolume)
{
	if (!CharacterOwner)
	{
		return;
	}
	if (NewVolume && NewVolume->bWaterVolume)
	{
		// just entered water
		if (!CanEverSwim())
		{
			// AI needs to stop any current moves
			if (PathFollowingComp.IsValid())
			{
				PathFollowingComp->AbortMove(TEXT("water"));
			}
		}
		else if (!IsSwimming())
		{
			SetMovementMode(MOVE_Swimming);
		}
	}
	else if (IsSwimming())
	{
		// just left the water - check if should jump out
		SetMovementMode(MOVE_Falling);
		FVector JumpDir(0.f);
		FVector WallNormal(0.f);
		if (Acceleration.Z > 0.f && ShouldJumpOutOfWater(JumpDir)
			&& ((JumpDir | Acceleration) > 0.f) && CheckWaterJump(JumpDir, WallNormal))
		{
			JumpOutOfWater(WallNormal);
			Velocity.Z = OutofWaterZ; //set here so physics uses this for remainder of tick
		}
	}
}

bool UShooterCharacterMovement::ShouldJumpOutOfWater(FVector& JumpDir)
{
	AController* OwnerController = CharacterOwner->GetController();
	if (OwnerController)
	{
		const FRotator ControllerRot = OwnerController->GetControlRotation();
		if ((Velocity.Z > 0.0f) && (ControllerRot.Pitch > JumpOutOfWaterPitch))
		{
			// if Pawn is going up and looking up, then make him jump
			JumpDir = ControllerRot.Vector();
			return true;
		}
	}

	return false;
}

bool UShooterCharacterMovement::CheckWaterJump(FVector CheckPoint, FVector& WallNormal)
{
	if (!CharacterOwner || !CharacterOwner->CapsuleComponent)
	{
		return false;
	}
	// check if there is a wall directly in front of the swimming pawn
	CheckPoint.Z = 0.f;
	FVector CheckNorm = CheckPoint.SafeNormal();
	float PawnCapsuleRadius, PawnCapsuleHalfHeight;
	CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnCapsuleRadius, PawnCapsuleHalfHeight);
	CheckPoint = CharacterOwner->GetActorLocation() + 1.2f * PawnCapsuleRadius * CheckNorm;
	FVector Extent(PawnCapsuleRadius, PawnCapsuleRadius, PawnCapsuleHalfHeight);
	FHitResult HitInfo(1.f);
	static const FName CheckWaterJumpName(TEXT("CheckWaterJump"));
	FCollisionQueryParams CapsuleParams(CheckWaterJumpName, false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(CapsuleParams, ResponseParam);
	FCollisionShape CapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_None);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
	FQuat CapsuleRotation = GetCharacterOwner()->GetCapsuleComponent()->GetComponentRotation().Quaternion();
	bool bHit = GetWorld()->SweepSingle(HitInfo, CharacterOwner->GetActorLocation(), CheckPoint, CapsuleRotation, CollisionChannel, CapsuleShape, CapsuleParams, ResponseParam);

	if (HitInfo.GetActor() && !Cast<APawn>(HitInfo.GetActor()))
	{
		// hit a wall - check if it is low enough
		WallNormal = -1.f * HitInfo.ImpactNormal;
		FVector Start = CharacterOwner->GetActorLocation();
		Start.Z += MaxOutOfWaterStepHeight;
		CheckPoint = Start + 3.2f * PawnCapsuleRadius * WallNormal;
		FCollisionQueryParams LineParams(CheckWaterJumpName, true, CharacterOwner);
		FCollisionResponseParams LineResponseParam;
		InitCollisionParams(LineParams, LineResponseParam);
		bHit = GetWorld()->LineTraceSingle(HitInfo, Start, CheckPoint, CollisionChannel, LineParams, LineResponseParam);
		// if no high obstruction, or it's a valid floor, then pawn can jump out of water
		return !bHit || IsWalkable(HitInfo);
	}
	return false;
}



void UShooterCharacterMovement::MoveSmooth(const FVector& InVelocity, const float DeltaSeconds, FStepDownResult* OutStepDownResult)
{
	return Super::MoveSmooth(InVelocity, DeltaSeconds, OutStepDownResult);

	FVector Delta = InVelocity * DeltaSeconds;
	if (!CharacterOwner || Delta.IsZero())
	{
		return;
	}

	FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

	if (IsMovingOnGround())
	{
		MoveAlongFloor(InVelocity, DeltaSeconds, OutStepDownResult);
	}
	else
	{
		FHitResult Hit(1.f);
		SafeMoveUpdatedComponent(Delta, CharacterOwner->GetActorRotation(), true, Hit);

		if (Hit.IsValidBlockingHit())
		{
			bool bSteppedUp = false;

			if (IsFlying())
			{
				if (CanStepUp(Hit))
				{
					OutStepDownResult = NULL; // No need for a floor when not walking.
					bool UpComponentLessThan02 = (FMath::Abs(Hit.ImpactNormal.Z) < 0.2f);
					switch (GravityMode) {
					case GRAVITY_XNEGATIVE:
					case GRAVITY_XPOSITIVE:
						UpComponentLessThan02 = (FMath::Abs(Hit.ImpactNormal.X) < 0.2f);
						break;
					case GRAVITY_YNEGATIVE:
					case GRAVITY_YPOSITIVE:
						UpComponentLessThan02 = (FMath::Abs(Hit.ImpactNormal.Y) < 0.2f);
						break;
					case GRAVITY_ZNEGATIVE:
					case GRAVITY_ZPOSITIVE:
						UpComponentLessThan02 = (FMath::Abs(Hit.ImpactNormal.Z) < 0.2f);
						break;
					}
					if (UpComponentLessThan02)
					{
						FVector GravDir = FVector(0.f, 0.f, -1.f);
						switch (GravityMode) {
						case GRAVITY_XNEGATIVE:
							GravDir = FVector(-1.f, 0.f, 0.f);
							break;
						case GRAVITY_XPOSITIVE:
							GravDir = FVector(1.f, 0.f, 0.f);
							break;
						case GRAVITY_YNEGATIVE:
							GravDir = FVector(0.f, -1.f, 0.f);
							break;
						case GRAVITY_YPOSITIVE:
							GravDir = FVector(0.f, 1.f, 0.f);
							break;
						case GRAVITY_ZNEGATIVE:
							GravDir = FVector(0.f, 0.f, -1.f);
							break;
						case GRAVITY_ZPOSITIVE:
							GravDir = FVector(0.f, 0.f, 1.f);
							break;
						}
						const FVector DesiredDir = Delta.SafeNormal();
						const float UpDown = GravDir | DesiredDir;
						if ((UpDown < 0.5f) && (UpDown > -0.2f))
						{
							bSteppedUp = StepUp(GravDir, Delta * (1.f - Hit.Time), Hit, OutStepDownResult);
						}
					}
				}
			}

			// If StepUp failed, try sliding.
			if (!bSteppedUp)
			{
				SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, false);
			}
		}
	}
}

bool UShooterCharacterMovement::IsWalkable(const FHitResult& Hit) const
{
	
	if (!Hit.bBlockingHit)
	{
		return false;
	}

	// Never walk up vertical surfaces.
	bool bVerticalSurface = false;
	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
		bVerticalSurface = (Hit.ImpactNormal.X < KINDA_SMALL_NUMBER);
		break;
	case GRAVITY_XPOSITIVE:
		bVerticalSurface = (Hit.ImpactNormal.X > -KINDA_SMALL_NUMBER);
		break;
	case GRAVITY_YNEGATIVE:
		bVerticalSurface = (Hit.ImpactNormal.Y < KINDA_SMALL_NUMBER);
		break;
	case GRAVITY_YPOSITIVE:
		bVerticalSurface = (Hit.ImpactNormal.Y > -KINDA_SMALL_NUMBER);
		break;
	case GRAVITY_ZNEGATIVE:
		bVerticalSurface = (Hit.ImpactNormal.Z < KINDA_SMALL_NUMBER);
		break;
	case GRAVITY_ZPOSITIVE:
		bVerticalSurface = (Hit.ImpactNormal.Z > -KINDA_SMALL_NUMBER);
		break;
	}
	if (bVerticalSurface)
	{
		return false;
	}

	float TestWalkableZ = GetWalkableFloorZ();

	// See if this component overrides the walkable floor z.
	const UPrimitiveComponent* HitComponent = Hit.Component.Get();
	if (HitComponent)
	{
		const FWalkableSlopeOverride& SlopeOverride = HitComponent->GetWalkableSlopeOverride();
		TestWalkableZ = SlopeOverride.ModifyWalkableFloorZ(TestWalkableZ);
	}

	// Can't walk on this surface if it is too steep.
	bool bTooSteep = false;

	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
		bTooSteep = (Hit.ImpactNormal.X < TestWalkableZ);
		break;
	case GRAVITY_XPOSITIVE:
		bTooSteep = (Hit.ImpactNormal.X > -TestWalkableZ);
		break;
	case GRAVITY_YNEGATIVE:
		bTooSteep = (Hit.ImpactNormal.Y < TestWalkableZ);
		break;
	case GRAVITY_YPOSITIVE:
		bTooSteep = (Hit.ImpactNormal.Y > -TestWalkableZ);
		break;
	case GRAVITY_ZNEGATIVE:
		bTooSteep = (Hit.ImpactNormal.Z < TestWalkableZ);
		break;
	case GRAVITY_ZPOSITIVE:
		bTooSteep = (Hit.ImpactNormal.Z > -TestWalkableZ);
		break;
	}
	if (bTooSteep)
	{
		return false;
	}

	return true;
}

void UShooterCharacterMovement::ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult) const
{
	OutFloorResult.Clear();

	// No collision, no floor...
	if (!UpdatedComponent->IsCollisionEnabled())
	{
		return;
	}

	float PawnRadius, PawnHalfHeight;
	CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	bool bSkipSweep = false;
	if (DownwardSweepResult != NULL && DownwardSweepResult->IsValidBlockingHit())
	{
		// Only if the supplied sweep was vertical and downward.
		bool bIsVerticalAndDownward = false;
		switch (GravityMode) {
		case GRAVITY_XNEGATIVE:
			bIsVerticalAndDownward = (DownwardSweepResult->TraceStart.X > DownwardSweepResult->TraceEnd.X);
			break;
		case GRAVITY_XPOSITIVE:
			bIsVerticalAndDownward = (DownwardSweepResult->TraceStart.X < DownwardSweepResult->TraceEnd.X);
			break;
		case GRAVITY_YNEGATIVE:
			bIsVerticalAndDownward = (DownwardSweepResult->TraceStart.Y > DownwardSweepResult->TraceEnd.Y);
			break;
		case GRAVITY_YPOSITIVE:
			bIsVerticalAndDownward = (DownwardSweepResult->TraceStart.Y < DownwardSweepResult->TraceEnd.Y);
			break;
		case GRAVITY_ZNEGATIVE:
			bIsVerticalAndDownward = (DownwardSweepResult->TraceStart.Z > DownwardSweepResult->TraceEnd.Z);
			break;
		case GRAVITY_ZPOSITIVE:
			bIsVerticalAndDownward = (DownwardSweepResult->TraceStart.Z < DownwardSweepResult->TraceEnd.Z);
			break;
		}
		if (bIsVerticalAndDownward &&
			GDSizeSquared2D((DownwardSweepResult->TraceStart - DownwardSweepResult->TraceEnd))/*.SizeSquared2D()*/ <= KINDA_SMALL_NUMBER)
		{
			// Reject hits that are barely on the cusp of the radius of the capsule
			if (GDIsWithinEdgeTolerance(DownwardSweepResult->Location, DownwardSweepResult->ImpactPoint, PawnRadius))
			{
				// Don't try a redundant sweep, regardless of whether this sweep is usable.
				bSkipSweep = true;

				const bool bIsWalkable = IsWalkable(*DownwardSweepResult);

				float FloorDist = 0.f;
				switch (GravityMode) {
				case GRAVITY_XNEGATIVE:
					FloorDist = (CapsuleLocation.X - DownwardSweepResult->Location.X);
					break;
				case GRAVITY_XPOSITIVE:
					FloorDist = (DownwardSweepResult->Location.X - CapsuleLocation.X);
					break;
				case GRAVITY_YNEGATIVE:
					FloorDist = (CapsuleLocation.Y - DownwardSweepResult->Location.Y);
					break;
				case GRAVITY_YPOSITIVE:
					FloorDist = (DownwardSweepResult->Location.Y - CapsuleLocation.Y);
					break;
				case GRAVITY_ZNEGATIVE:
					FloorDist = (CapsuleLocation.Z - DownwardSweepResult->Location.Z);
					break;
				case GRAVITY_ZPOSITIVE:
					FloorDist = (DownwardSweepResult->Location.Z - CapsuleLocation.Z);
					break;
				}

				OutFloorResult.SetFromSweep(*DownwardSweepResult, FloorDist, bIsWalkable);

				if (bIsWalkable)
				{
					// Use the supplied downward sweep as the floor hit result.			
					return;
				}
			}
		}
	}

	// We require the sweep distance to be >= the line distance, otherwise the HitResult can't be interpreted as the sweep result.
	if (SweepDistance < LineDistance)
	{
		check(SweepDistance >= LineDistance);
		return;
	}

	bool bBlockingHit = false;
	FCollisionQueryParams QueryParams(NAME_None, false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(QueryParams, ResponseParam);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();

	// Sweep test
	if (!bSkipSweep && SweepDistance > 0.f && SweepRadius > 0.f)
	{
		// Use a shorter height to avoid sweeps giving weird results if we start on a surface.
		// This also allows us to adjust out of penetrations.
		const float ShrinkScale = 0.9f;
		const float ShrinkScaleOverlap = 0.6f;
		float ShrinkHeight = (PawnHalfHeight - PawnRadius) * (1.f - ShrinkScale);
		float TraceDist = SweepDistance + ShrinkHeight;
		FVector TraceVector;

		switch (GravityMode) {
		case GRAVITY_XNEGATIVE:
			TraceVector = FVector(-TraceDist, 0.f, 0.f);
			break;
		case GRAVITY_XPOSITIVE:
			TraceVector = FVector(TraceDist, 0.f, 0.f);
			break;
		case GRAVITY_YNEGATIVE:
			TraceVector = FVector(0.f, -TraceDist, 0.f);
			break;
		case GRAVITY_YPOSITIVE:
			TraceVector = FVector(0.f, TraceDist, 0.f);
			break;
		case GRAVITY_ZNEGATIVE:
			TraceVector = FVector(0.f, 0.f, -TraceDist);
			break;
		case GRAVITY_ZPOSITIVE:
			TraceVector = FVector(0.f, 0.f, TraceDist);
			break;
		}
		static const FName ComputeFloorDistName(TEXT("ComputeFloorDistSweep"));
		QueryParams.TraceTag = ComputeFloorDistName;
		FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(SweepRadius, PawnHalfHeight - ShrinkHeight);

		FHitResult Hit(1.f);
		FQuat CapsuleRotation = GetCharacterOwner()->GetCapsuleComponent()->GetComponentRotation().Quaternion();
		bBlockingHit = GetWorld()->SweepSingle(Hit, CapsuleLocation, CapsuleLocation + TraceVector, CapsuleRotation, CollisionChannel, CapsuleShape, QueryParams, ResponseParam);

		if (bBlockingHit)
		{
			// Reject hits adjacent to us, we only care about hits on the bottom portion of our capsule.
			// Check 2D distance to impact point, reject if within a tolerance from radius.
			if (Hit.bStartPenetrating || !GDIsWithinEdgeTolerance(CapsuleLocation, Hit.ImpactPoint, CapsuleShape.Capsule.Radius))
			{
				// Use a capsule with a slightly smaller radius and shorter height to avoid the adjacent object.
				ShrinkHeight = (PawnHalfHeight - PawnRadius) * (1.f - ShrinkScaleOverlap);
				TraceDist = SweepDistance + ShrinkHeight;
				CapsuleShape.Capsule.Radius = FMath::Max(0.f, CapsuleShape.Capsule.Radius - SWEEP_EDGE_REJECT_DISTANCE - KINDA_SMALL_NUMBER);
				CapsuleShape.Capsule.HalfHeight = FMath::Max(PawnHalfHeight - ShrinkHeight, 0.1f);
				FQuat CapsuleRotation = GetCharacterOwner()->GetCapsuleComponent()->GetComponentRotation().Quaternion();
				bBlockingHit = GetWorld()->SweepSingle(Hit, CapsuleLocation, CapsuleLocation + TraceVector, CapsuleRotation, CollisionChannel, CapsuleShape, QueryParams, ResponseParam);
			}

			// Reduce hit distance by ShrinkHeight because we shrank the capsule for the trace.
			// We allow negative distances here, because this allows us to pull out of penetrations.
			const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
			const float SweepResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);

			OutFloorResult.SetFromSweep(Hit, SweepResult, false);
			if (Hit.IsValidBlockingHit() && IsWalkable(Hit))
			{
				if (SweepResult <= SweepDistance)
				{
					// Hit within test distance.
					OutFloorResult.bWalkableFloor = true;
					return;
				}
			}
		}
	}

	// Since we require a longer sweep than line trace, we don't want to run the line trace if the sweep missed everything.
	// We do however want to try a line trace if the sweep was stuck in penetration.
	if (!OutFloorResult.bBlockingHit && !OutFloorResult.HitResult.bStartPenetrating)
	{
		OutFloorResult.FloorDist = SweepDistance;
		return;
	}

	// Line trace
	if (LineDistance > 0.f)
	{
		const float ShrinkHeight = PawnHalfHeight;
		const FVector LineTraceStart = CapsuleLocation;
		const float TraceDist = LineDistance + ShrinkHeight;
		FVector Down = FVector(0.f, 0.f, -TraceDist);
		switch (GravityMode) {
		case GRAVITY_XNEGATIVE: Down = FVector(-TraceDist, 0.f, 0.f); break;
		case GRAVITY_XPOSITIVE: Down = FVector(TraceDist, 0.f, 0.f); break;
		case GRAVITY_YNEGATIVE: Down = FVector(0.f, -TraceDist, 0.f); break;
		case GRAVITY_YPOSITIVE: Down = FVector(0.f, TraceDist, 0.f); break;
		case GRAVITY_ZNEGATIVE: Down = FVector(0.f, 0.f, -TraceDist); break;
		case GRAVITY_ZPOSITIVE: Down = FVector(0.f, 0.f, TraceDist); break;
		}

		static const FName FloorLineTraceName = FName(TEXT("ComputeFloorDistLineTrace"));
		QueryParams.TraceTag = FloorLineTraceName;

		FHitResult Hit(1.f);
		bBlockingHit = GetWorld()->LineTraceSingle(Hit, LineTraceStart, LineTraceStart + Down, CollisionChannel, QueryParams, ResponseParam);

		if (bBlockingHit)
		{
			if (Hit.Time > 0.f)
			{
				// Reduce hit distance by ShrinkHeight because we started the trace higher than the base.
				// We allow negative distances here, because this allows us to pull out of penetrations.
				const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
				const float LineResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);

				OutFloorResult.bBlockingHit = true;
				if (LineResult <= LineDistance && IsWalkable(Hit))
				{
					OutFloorResult.SetFromLineTrace(Hit, OutFloorResult.FloorDist, LineResult, true);
					return;
				}
			}
		}
	}

	// No hits were acceptable.
	OutFloorResult.bWalkableFloor = false;
	OutFloorResult.FloorDist = SweepDistance;
}

bool UShooterCharacterMovement::IsValidLandingSpot(const FVector& CapsuleLocation, const FHitResult& Hit) const
{
	if (!Hit.bBlockingHit)
	{
		return false;
	}

	// Reject unwalkable floor normals.
	if (!IsWalkable(Hit))
	{
		return false;
	}

	// This can happen when landing on upward moving geometry
	if (Hit.bStartPenetrating)
	{
		return true;
	}

	float PawnRadius, PawnHalfHeight;
	CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	// Reject hits that are above our lower hemisphere (can happen when sliding down a vertical surface).
	//THIS HAS TO BE CORRECTED FOR GRAVITY DEPENDENCE ... NEEDS TO BE REWRITTEN!!!!!!!!
	const float LowerHemisphereZ = Hit.Location.Z - PawnHalfHeight + PawnRadius;
	if (Hit.ImpactPoint.Z >= LowerHemisphereZ)
	{

		//return false;
	}

	// Reject hits that are barely on the cusp of the radius of the capsule
	if (!GDIsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnRadius))
	{
		return false;
	}

	FFindFloorResult FloorResult;
	FindFloor(CapsuleLocation, FloorResult, false, &Hit);

	if (!FloorResult.IsWalkableFloor())
	{
		return false;
	}

	return true;
}

bool UShooterCharacterMovement::ShouldComputePerchResult(const FHitResult& InHit, bool bCheckRadius) const
{
	if (!InHit.IsValidBlockingHit())
	{
		return false;
	}

	// Don't try to perch if the edge radius is very small.
	if (GetPerchRadiusThreshold() <= SWEEP_EDGE_REJECT_DISTANCE)
	{
		return false;
	}

	if (bCheckRadius)
	{
		const float DistFromCenterSq = GDSizeSquared2D((InHit.ImpactPoint - InHit.Location));// .SizeSquared2D();
		const float StandOnEdgeRadius = GetValidPerchRadius();
		if (DistFromCenterSq <= FMath::Square(StandOnEdgeRadius))
		{
			// Already within perch radius.
			return false;
		}
	}

	return true;
}

bool UShooterCharacterMovement::ComputePerchResult(const float TestRadius, const FHitResult& InHit, const float InMaxFloorDist, FFindFloorResult& OutPerchFloorResult) const
{
	if (InMaxFloorDist <= 0.f)
	{
		return 0.f;
	}

	// Sweep further than actual requested distance, because a reduced capsule radius means we could miss some hits that the normal radius would contact.
	float PawnRadius, PawnHalfHeight;
	CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	float InHitAboveBase = 0.f;
	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
		InHitAboveBase = FMath::Max(0.f, InHit.ImpactPoint.X - (InHit.Location.X - PawnHalfHeight));
		break;
	case GRAVITY_XPOSITIVE:
		InHitAboveBase = FMath::Max(0.f, InHit.ImpactPoint.X + (InHit.Location.X + PawnHalfHeight));
		break;
	case GRAVITY_YNEGATIVE:
		InHitAboveBase = FMath::Max(0.f, InHit.ImpactPoint.Y - (InHit.Location.Y - PawnHalfHeight));
		break;
	case GRAVITY_YPOSITIVE:
		InHitAboveBase = FMath::Max(0.f, InHit.ImpactPoint.Y + (InHit.Location.Y + PawnHalfHeight));
		break;
	case GRAVITY_ZNEGATIVE:
		InHitAboveBase = FMath::Max(0.f, InHit.ImpactPoint.Z - (InHit.Location.Z - PawnHalfHeight));
		break;
	case GRAVITY_ZPOSITIVE:
		InHitAboveBase = FMath::Max(0.f, InHit.ImpactPoint.Z + (InHit.Location.Z + PawnHalfHeight));
		break;
	}
	//const float InHitAboveBase = FMath::Max(0.f, InHit.ImpactPoint.Z - (InHit.Location.Z - PawnHalfHeight));
	const float PerchLineDist = FMath::Max(0.f, InMaxFloorDist - InHitAboveBase);
	const float PerchSweepDist = FMath::Max(0.f, InMaxFloorDist);

	const float ActualSweepDist = PerchSweepDist + PawnRadius;
	ComputeFloorDist(InHit.Location, PerchLineDist, ActualSweepDist, OutPerchFloorResult, TestRadius);

	if (!OutPerchFloorResult.IsWalkableFloor())
	{
		return false;
	}
	else if (InHitAboveBase + OutPerchFloorResult.FloorDist > InMaxFloorDist)
	{
		// Hit something past max distance
		OutPerchFloorResult.bWalkableFloor = false;
		return false;
	}

	return true;
}

bool UShooterCharacterMovement::StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult &InHit, FStepDownResult* OutStepDownResult)
{
	if (!CanStepUp(InHit))
	{
		return false;
	}

	if (MaxStepHeight <= 0.f)
	{
		return false;
	}

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	float PawnRadius, PawnHalfHeight;
	CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	// Don't bother stepping up if top of capsule is hitting something.
	bool topCapsuleHitting = false;
	float InitialImpactUpComponent;
	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
		InitialImpactUpComponent = InHit.ImpactPoint.X;
		topCapsuleHitting = (InitialImpactUpComponent > OldLocation.X + (PawnHalfHeight - PawnRadius));
		break;
	case GRAVITY_XPOSITIVE:
		InitialImpactUpComponent = InHit.ImpactPoint.X;
		topCapsuleHitting = (InitialImpactUpComponent < OldLocation.X - (PawnHalfHeight + PawnRadius));
		break;
	case GRAVITY_YNEGATIVE:
		InitialImpactUpComponent = InHit.ImpactPoint.Y;
		topCapsuleHitting = (InitialImpactUpComponent > OldLocation.Y + (PawnHalfHeight - PawnRadius));
		break;
	case GRAVITY_YPOSITIVE:
		InitialImpactUpComponent = InHit.ImpactPoint.Y;
		topCapsuleHitting = (InitialImpactUpComponent < OldLocation.Y - (PawnHalfHeight + PawnRadius));
		break;
	case GRAVITY_ZNEGATIVE:
		InitialImpactUpComponent = InHit.ImpactPoint.Z;
		topCapsuleHitting = (InitialImpactUpComponent > OldLocation.Z + (PawnHalfHeight - PawnRadius));
		break;
	case GRAVITY_ZPOSITIVE:
		InitialImpactUpComponent = InHit.ImpactPoint.Z;
		topCapsuleHitting = (InitialImpactUpComponent < OldLocation.Z - (PawnHalfHeight + PawnRadius));
		break;
	}
	
	if (topCapsuleHitting)
	{
		return false;
	}

	// Don't step up if the impact is below us
	bool bImpactIsBelow = false;
	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
		bImpactIsBelow = (InitialImpactUpComponent <= OldLocation.X - PawnHalfHeight);
		break;
	case GRAVITY_XPOSITIVE:
		bImpactIsBelow = (InitialImpactUpComponent >= OldLocation.X + PawnHalfHeight);
		break;
	case GRAVITY_YNEGATIVE:
		bImpactIsBelow = (InitialImpactUpComponent <= OldLocation.Y - PawnHalfHeight);
		break;
	case GRAVITY_YPOSITIVE:
		bImpactIsBelow = (InitialImpactUpComponent >= OldLocation.Y + PawnHalfHeight);
		break;
	case GRAVITY_ZNEGATIVE:
		bImpactIsBelow = (InitialImpactUpComponent <= OldLocation.Z - PawnHalfHeight);
		break;
	case GRAVITY_ZPOSITIVE:
		bImpactIsBelow = (InitialImpactUpComponent >= OldLocation.Z + PawnHalfHeight);
		break;
	}
	if (bImpactIsBelow)
	{
		return false;
	}

	float StepTravelHeight = MaxStepHeight;
	const float StepSideUpComponent = -1.f * (InHit.ImpactNormal | GravDir);
	float PawnInitialFloorBaseUpComponent = OldLocation.Z - PawnHalfHeight;
	switch (GravityMode) {
	case GRAVITY_XNEGATIVE:
		PawnInitialFloorBaseUpComponent = OldLocation.X - PawnHalfHeight;
		break;
	case GRAVITY_XPOSITIVE:
		PawnInitialFloorBaseUpComponent = OldLocation.X + PawnHalfHeight;
		break;
	case GRAVITY_YNEGATIVE:
		PawnInitialFloorBaseUpComponent = OldLocation.Y - PawnHalfHeight;
		break;
	case GRAVITY_YPOSITIVE:
		PawnInitialFloorBaseUpComponent = OldLocation.Y + PawnHalfHeight;
		break;
	case GRAVITY_ZNEGATIVE:
		PawnInitialFloorBaseUpComponent = OldLocation.Z - PawnHalfHeight;
		break;
	case GRAVITY_ZPOSITIVE:
		PawnInitialFloorBaseUpComponent = OldLocation.Z + PawnHalfHeight;
		break;
	}
	float PawnFloorPointUpComponent = PawnInitialFloorBaseUpComponent;

	if (IsMovingOnGround() && CurrentFloor.IsWalkableFloor())
	{
		// Since we float a variable amount off the floor, we need to enforce max step height off the actual point of impact with the floor.
		const float FloorDist = FMath::Max(0.f, CurrentFloor.FloorDist);
		switch (GravityMode) {
		case GRAVITY_XNEGATIVE:
		case GRAVITY_YNEGATIVE:
		case GRAVITY_ZNEGATIVE:
			PawnInitialFloorBaseUpComponent -= FloorDist;
			break;
		case GRAVITY_XPOSITIVE:
		case GRAVITY_YPOSITIVE:
		case GRAVITY_ZPOSITIVE:
			PawnInitialFloorBaseUpComponent += FloorDist;
			break;
		}
		
		StepTravelHeight = FMath::Max(StepTravelHeight - FloorDist, 0.f);

		const bool bHitVerticalFace = !IsWithinEdgeTolerance(InHit.Location, InHit.ImpactPoint, PawnRadius);
		if (!CurrentFloor.bLineTrace && !bHitVerticalFace)
		{
			PawnFloorPointUpComponent = CurrentFloor.HitResult.ImpactPoint.Z;
		}
		else
		{
			// Base floor point is the base of the capsule moved down by how far we are hovering over the surface we are hitting.
			switch (GravityMode) {
			case GRAVITY_XNEGATIVE:
			case GRAVITY_YNEGATIVE:
			case GRAVITY_ZNEGATIVE:
				PawnFloorPointUpComponent -= CurrentFloor.FloorDist;
				break;
			case GRAVITY_XPOSITIVE:
			case GRAVITY_YPOSITIVE:
			case GRAVITY_ZPOSITIVE:
				PawnFloorPointUpComponent += CurrentFloor.FloorDist;
				break;
			}
			//PawnFloorPointUpComponent -= CurrentFloor.FloorDist;
		}
	}

	// Scope our movement updates, and do not apply them until all intermediate moves are completed.
	FScopedMovementUpdate ScopedStepUpMovement(UpdatedComponent, EScopedUpdate::DeferredUpdates);

	// step up - treat as vertical wall
	FHitResult SweepUpHit(1.f);
	const FRotator PawnRotation = CharacterOwner->GetActorRotation();
	SafeMoveUpdatedComponent(-GravDir * StepTravelHeight, PawnRotation, true, SweepUpHit);

	// step fwd
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

	// If we hit something above us and also something ahead of us, we should notify about the upward hit as well.
	// The forward hit will be handled later (in the bSteppedOver case below).
	// In the case of hitting something above but not forward, we are not blocked from moving so we don't need the notification.
	if (SweepUpHit.bBlockingHit && Hit.bBlockingHit)
	{
		HandleImpact(SweepUpHit);
	}

	// Check result of forward movement
	if (Hit.bBlockingHit)
	{
		if (Hit.bStartPenetrating)
		{
			// Undo movement
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// pawn ran into a wall
		HandleImpact(Hit);
		if (IsFalling())
		{
			return true;
		}

		// adjust and try again
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
		if (IsFalling())
		{
			ScopedStepUpMovement.RevertMove();
			return false;
		}
	}

	// Step down
	SafeMoveUpdatedComponent(GravDir * (MaxStepHeight + MAX_FLOOR_DIST*2.f), CharacterOwner->GetActorRotation(), true, Hit);

	// If step down was initially penetrating abort the step up
	if (Hit.bStartPenetrating)
	{
		ScopedStepUpMovement.RevertMove();
		return false;
	}

	FStepDownResult StepDownResult;
	if (Hit.IsValidBlockingHit())
	{
		const FVector HitLocation = Hit.Location;

		// See if the downward move impacts on the lower capsule hemisphere
		float CurBaseLocation = (HitLocation.Z - PawnHalfHeight);
		float LowerImpactHeight = Hit.ImpactPoint.Z - CurBaseLocation;
		bool downwardMoveImpactsLowerCapsuleHemisphere = false;
		switch (GravityMode) {
		case GRAVITY_XNEGATIVE:
			CurBaseLocation = (HitLocation.X - PawnHalfHeight);
			LowerImpactHeight = Hit.ImpactPoint.X - CurBaseLocation;
			downwardMoveImpactsLowerCapsuleHemisphere = (LowerImpactHeight <= PawnRadius);
			break;
		case GRAVITY_XPOSITIVE:
			CurBaseLocation = (HitLocation.X + PawnHalfHeight);
			LowerImpactHeight = Hit.ImpactPoint.X + CurBaseLocation;
			downwardMoveImpactsLowerCapsuleHemisphere = (LowerImpactHeight <= PawnRadius);
			break;
		case GRAVITY_YNEGATIVE:
			CurBaseLocation = (HitLocation.Y - PawnHalfHeight);
			LowerImpactHeight = Hit.ImpactPoint.Y - CurBaseLocation;
			downwardMoveImpactsLowerCapsuleHemisphere = (LowerImpactHeight <= PawnRadius);
			break;
		case GRAVITY_YPOSITIVE:
			CurBaseLocation = (HitLocation.Y + PawnHalfHeight);
			LowerImpactHeight = Hit.ImpactPoint.Y + CurBaseLocation;
			downwardMoveImpactsLowerCapsuleHemisphere = (LowerImpactHeight <= PawnRadius);
			break;
		case GRAVITY_ZNEGATIVE:
			CurBaseLocation = (HitLocation.Z - PawnHalfHeight);
			LowerImpactHeight = Hit.ImpactPoint.Z - CurBaseLocation;
			downwardMoveImpactsLowerCapsuleHemisphere = (LowerImpactHeight <= PawnRadius);
			break;
		case GRAVITY_ZPOSITIVE:
			CurBaseLocation = (HitLocation.Z + PawnHalfHeight);
			LowerImpactHeight = Hit.ImpactPoint.Z + CurBaseLocation;
			downwardMoveImpactsLowerCapsuleHemisphere = (LowerImpactHeight <= PawnRadius);
			break;
		}
		if (downwardMoveImpactsLowerCapsuleHemisphere)
		{
			// See if this step sequence would have allowed us to travel higher than our max step height allows.
			float DeltaUpComponent = Hit.ImpactPoint.Z - PawnFloorPointUpComponent;
			switch (GravityMode) {
			case GRAVITY_XNEGATIVE:
				DeltaUpComponent = Hit.ImpactPoint.X - PawnFloorPointUpComponent;
				break;
			case GRAVITY_XPOSITIVE:
				DeltaUpComponent = Hit.ImpactPoint.X + PawnFloorPointUpComponent;
				break;
			case GRAVITY_YNEGATIVE:
				DeltaUpComponent = Hit.ImpactPoint.Y - PawnFloorPointUpComponent;
				break;
			case GRAVITY_YPOSITIVE:
				DeltaUpComponent = Hit.ImpactPoint.Y + PawnFloorPointUpComponent;
				break;
			case GRAVITY_ZNEGATIVE:
				DeltaUpComponent = Hit.ImpactPoint.Z - PawnFloorPointUpComponent;
				break;
			case GRAVITY_ZPOSITIVE:
				DeltaUpComponent = Hit.ImpactPoint.Z + PawnFloorPointUpComponent;
				break;
			}
			if (DeltaUpComponent > MaxStepHeight)
			{
				//UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("- Reject StepUp (too high Height %.3f) up from floor base %f to %f"), DeltaZ, PawnInitialFloorBaseZ, NewLocation.Z);
				ScopedStepUpMovement.RevertMove();
				return false;
			}
		}

		// Reject unwalkable surface normals here.
		if (!IsWalkable(Hit))
		{
			// Reject if normal is towards movement direction
			const bool bNormalTowardsMe = (Delta | Hit.ImpactNormal) < 0.f;
			if (bNormalTowardsMe)
			{
				//UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("- Reject StepUp (unwalkable normal %s opposed to movement)"), *Hit.ImpactNormal.ToString());
				ScopedStepUpMovement.RevertMove();
				return false;
			}

			// Also reject if we would end up being higher than our starting location by stepping down.
			// It's fine to step down onto an unwalkable normal below us, we will just slide off. Rejecting those moves would prevent us from being able to walk off the edge.
			bool bEndUpHigher = false;
			switch (GravityMode) {
			case GRAVITY_XNEGATIVE:
				bEndUpHigher = (HitLocation.X > OldLocation.X);
				break;
			case GRAVITY_XPOSITIVE:
				bEndUpHigher = (HitLocation.X < OldLocation.X);
				break;
			case GRAVITY_YNEGATIVE:
				bEndUpHigher = (HitLocation.Y > OldLocation.Y);
				break;
			case GRAVITY_YPOSITIVE:
				bEndUpHigher = (HitLocation.Y < OldLocation.Y);
				break;
			case GRAVITY_ZNEGATIVE:
				bEndUpHigher = (HitLocation.Z > OldLocation.Z);
				break;
			case GRAVITY_ZPOSITIVE:
				bEndUpHigher = (HitLocation.Z < OldLocation.Z);
				break;
			}
			if (bEndUpHigher)
			{
				//UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("- Reject StepUp (unwalkable normal %s above old position)"), *Hit.ImpactNormal.ToString());
				ScopedStepUpMovement.RevertMove();
				return false;
			}
		}

		// See if we can validate the floor as a result of this step down. In almost all cases this should succeed, and we can avoid computing the floor outside this method.
		if (OutStepDownResult != NULL)
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), StepDownResult.FloorResult, false, &Hit);

			// Reject unwalkable normals if we end up higher than our initial height.
			// It's fine to walk down onto an unwalkable surface, don't reject those moves.
			bool bEndUpHigher = false;
			switch (GravityMode) {
			case GRAVITY_XNEGATIVE:
				bEndUpHigher = (HitLocation.X > OldLocation.X);
				break;
			case GRAVITY_XPOSITIVE:
				bEndUpHigher = (HitLocation.X < OldLocation.X);
				break;
			case GRAVITY_YNEGATIVE:
				bEndUpHigher = (HitLocation.Y > OldLocation.Y);
				break;
			case GRAVITY_YPOSITIVE:
				bEndUpHigher = (HitLocation.Y < OldLocation.Y);
				break;
			case GRAVITY_ZNEGATIVE:
				bEndUpHigher = (HitLocation.Z > OldLocation.Z);
				break;
			case GRAVITY_ZPOSITIVE:
				bEndUpHigher = (HitLocation.Z < OldLocation.Z);
				break;
			}

			if (bEndUpHigher)
			{
				// We should reject the floor result if we are trying to step up an actual step where we are not able to perch (this is rare).
				// In those cases we should instead abort the step up and try to slide along the stair.
				if (!StepDownResult.FloorResult.bBlockingHit && StepSideUpComponent < MAX_STEP_SIDE_Z)
				{
					ScopedStepUpMovement.RevertMove();
					return false;
				}
			}

			StepDownResult.bComputedFloor = true;
		}
	}

	// Copy step down result.
	if (OutStepDownResult != NULL)
	{
		*OutStepDownResult = StepDownResult;
	}

	// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
	bJustTeleported |= !bMaintainHorizontalGroundVelocity;

	return true;
}


void UShooterCharacterMovement::HandleImpact(FHitResult const& Impact, float TimeSlice, const FVector& MoveDelta)
{
	UE_LOG(LogCharacterMovement, Verbose, TEXT("Handle Impact Called"));

	if (CharacterOwner)
	{
		CharacterOwner->MoveBlockedBy(Impact);
	}

	if (PathFollowingComp.IsValid())
	{	// Also notify path following!
		PathFollowingComp->OnMoveBlockedBy(Impact);
	}

	APawn* OtherPawn = Cast<APawn>(Impact.GetActor());
	if (OtherPawn)
	{
		NotifyBumpedPawn(OtherPawn);
	}

	if (bEnablePhysicsInteraction && Impact.Component != NULL && Impact.Component->IsAnySimulatingPhysics() && Impact.bBlockingHit)
	{
		FVector ForcePoint = Impact.ImpactPoint;

		FBodyInstance* BI = Impact.Component->GetBodyInstance(Impact.BoneName);

		float BodyMass = 1.0f;

		if (BI != NULL)
		{
			BodyMass = FMath::Max(BI->GetBodyMass(), 1.0f);

			FBox Bounds = BI->GetBodyBounds();

			FVector Center, Extents;
			Bounds.GetCenterAndExtents(Center, Extents);

			if (!Extents.IsNearlyZero())
			{
				ForcePoint.Z = Center.Z + Extents.Z * PushForcePointZOffsetFactor;
			}
		}

		FVector Force = Impact.ImpactNormal * -1.0f;
		Force.Normalize();

		float PushForceModificator = 1.0f;

		FVector CurrentVelocity = Impact.Component->GetPhysicsLinearVelocity();
		FVector VirtualVelocity = Acceleration.SafeNormal() * GetMaxSpeed();

		float Dot = 0.0f;

		if (bScalePushForceToVelocity && !CurrentVelocity.IsNearlyZero())
		{
			Dot = CurrentVelocity | VirtualVelocity;

			if (Dot > 0.0f && Dot < 1.0f)
			{
				PushForceModificator *= Dot;
			}
		}

		if (bPushForceScaledToMass)
		{
			PushForceModificator *= BodyMass;
		}

		Force *= PushForceModificator;

		if (CurrentVelocity.IsNearlyZero())
		{
			Force *= InitialPushForceFactor;
			Impact.Component->AddImpulseAtLocation(Force, ForcePoint, Impact.BoneName);
		}
		else
		{
			Force *= PushForceFactor;
			Impact.Component->AddForceAtLocation(Force, ForcePoint, Impact.BoneName);
		}
	}
}

FVector UShooterCharacterMovement::ConstrainInputAcceleration(const FVector& InputAcceleration) const
{
	FVector NewAccel = InputAcceleration;

	// walking or falling pawns ignore up/down sliding
	if (IsMovingOnGround() || IsFalling())
	{
		switch (GravityMode) {
		case GRAVITY_XNEGATIVE:
		case GRAVITY_XPOSITIVE:
			NewAccel.X = 0.f;
			break;
		case GRAVITY_YNEGATIVE:
		case GRAVITY_YPOSITIVE:
			NewAccel.Y = 0.f;
			break;
		case GRAVITY_ZNEGATIVE:
		case GRAVITY_ZPOSITIVE:
			NewAccel.Z = 0.f;
			break;
		}

	}

	return NewAccel;
}

void UShooterCharacterMovement::SmoothClientPosition(float DeltaSeconds)
{
	return;
	if (!CharacterOwner || GetNetMode() != NM_Client)
	{
		return;
	}

	FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
	if (ClientData && ClientData->bSmoothNetUpdates)
	{
		// smooth interpolation of mesh translation to avoid popping of other client pawns, unless driving or ragdoll or low tick rate
		if ((DeltaSeconds < ClientData->SmoothNetUpdateTime) && CharacterOwner->Mesh && !CharacterOwner->Mesh->IsSimulatingPhysics())
		{
			ClientData->MeshTranslationOffset = (ClientData->MeshTranslationOffset * (1.f - DeltaSeconds / ClientData->SmoothNetUpdateTime));
		}
		else
		{
			ClientData->MeshTranslationOffset = FVector::ZeroVector;
		}

		if (IsMovingOnGround())
		{
			// don't smooth Up Component position if walking on ground
			switch (GravityMode) {
			case GRAVITY_XNEGATIVE:
			case GRAVITY_XPOSITIVE:
				ClientData->MeshTranslationOffset.X = 0;
				break;
			case GRAVITY_YNEGATIVE:
			case GRAVITY_YPOSITIVE:
				ClientData->MeshTranslationOffset.Y = 0;
				break;
			case GRAVITY_ZNEGATIVE:
			case GRAVITY_ZPOSITIVE:
				ClientData->MeshTranslationOffset.Z = 0;
				break;
			}
			
		}

		if (CharacterOwner->Mesh)
		{
			const FVector NewRelTranslation = CharacterOwner->ActorToWorld().InverseTransformVector(ClientData->MeshTranslationOffset + CharacterOwner->GetBaseTranslationOffset());
			CharacterOwner->Mesh->SetRelativeLocation(NewRelTranslation);
		}
	}
}
/*
void UShooterCharacterMovement::CapsuleTouched(AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
check(bEnablePhysicsInteraction);

if (OtherComp != NULL && OtherComp->IsAnySimulatingPhysics())
{
const FVector OtherLoc = OtherComp->GetComponentLocation();
const FVector Loc = UpdatedComponent->GetComponentLocation();
FVector ImpulseDir = FVector(OtherLoc.X - Loc.X, OtherLoc.Y - Loc.Y, 0.25f).SafeNormal();
ImpulseDir = (ImpulseDir + Velocity.SafeNormal2D()) * 0.5f;
ImpulseDir.Normalize();

FName BoneName = NAME_None;
if (OtherBodyIndex != INDEX_NONE)
{
BoneName = ((USkinnedMeshComponent*)OtherComp)->GetBoneName(OtherBodyIndex);
}

float TouchForceFactorModified = TouchForceFactor;

if (bTouchForceScaledToMass)
{
FBodyInstance* BI = OtherComp->GetBodyInstance(BoneName);
TouchForceFactorModified *= BI ? BI->GetBodyMass() : 1.0f;
}

float ImpulseStrength = FMath::Clamp(Velocity.Size2D() * TouchForceFactorModified,
MinTouchForce > 0.0f ? MinTouchForce : -FLT_MAX,
MaxTouchForce > 0.0f ? MaxTouchForce : FLT_MAX);

FVector Impulse = ImpulseDir * ImpulseStrength;

OtherComp->AddImpulse(Impulse, BoneName);
}
}
*/

/*
void UShooterCharacterMovement::ApplyRepulsionForce(float DeltaTime)
{
FCollisionQueryParams QueryParams;
QueryParams.bReturnFaceIndex = false;
QueryParams.bReturnPhysicalMaterial = false;

const float CapsuleRadius = UpdatedComponent->GetCollisionShape().GetCapsuleRadius();
const float CapsuleHalfHeight = UpdatedComponent->GetCollisionShape().GetCapsuleHalfHeight();
const float RepulsionForceRadius = CapsuleRadius * 1.2f;
const float StopBodyDistance = 2.5f;

if (RepulsionForce > 0.0f)
{
const TArray<FOverlapInfo>& Overlaps = UpdatedComponent->GetOverlapInfos();

const FVector MyLocation = UpdatedComponent->GetComponentLocation();

for (int32 i = 0; i < Overlaps.Num(); ++i)
{
const FOverlapInfo& Overlap = Overlaps[i];

UPrimitiveComponent* OverlapComp = Overlap.Component.Get();
if (!OverlapComp || OverlapComp->Mobility < EComponentMobility::Movable) { continue; }

FName BoneName = NAME_None;
if (Overlap.BodyIndex != INDEX_NONE && OverlapComp->IsA(USkinnedMeshComponent::StaticClass()))
{
BoneName = ((USkinnedMeshComponent*)OverlapComp)->GetBoneName(Overlap.BodyIndex);
}

// Use the body instead of the component for cases where we have multi-body overlaps enabled
FBodyInstance* OverlapBody = OverlapComp->GetBodyInstance(BoneName);

if (!OverlapBody)
{
UE_LOG(LogCharacterMovement, Warning, TEXT("%s could not find overlap body for bone %s"), *GetName(), *BoneName.ToString());
continue;
}

// Early out if this is not a destructible and the body is not simulated
bool bIsCompDestructible = OverlapComp->IsA(UDestructibleComponent::StaticClass());
if (!bIsCompDestructible && !OverlapBody->IsInstanceSimulatingPhysics())
{
continue;
}


FTransform BodyTransform = OverlapBody->GetUnrealWorldTransform();

FVector BodyVelocity = OverlapBody->GetUnrealWorldVelocity();
FVector BodyLocation = BodyTransform.GetLocation();

// Trace to get the hit location on the capsule
FHitResult Hit;
bool bHasHit = UpdatedComponent->LineTraceComponent(Hit, BodyLocation,
FVector(MyLocation.X, MyLocation.Y, BodyLocation.Z),
QueryParams);

FVector HitLoc = Hit.ImpactPoint;
bool bIsPenetrating = Hit.bStartPenetrating || Hit.PenetrationDepth > 2.5f;

// If we didn't hit the capsule, we're inside the capsule
if (!bHasHit)
{
HitLoc = BodyLocation;
bIsPenetrating = true;
}

const float DistanceNow = (HitLoc - BodyLocation).SizeSquared2D();
const float DistanceLater = (HitLoc - (BodyLocation + BodyVelocity * DeltaTime)).SizeSquared2D();

if (BodyLocation.SizeSquared() > 0.1f && bHasHit && DistanceNow < StopBodyDistance && !bIsPenetrating)
{
OverlapBody->SetLinearVelocity(FVector(0.0f, 0.0f, 0.0f), false);
}
else if (DistanceLater <= DistanceNow || bIsPenetrating)
{
FVector ForceCenter(MyLocation.X, MyLocation.Y, bHasHit ? HitLoc.Z : MyLocation.Z);

if (!bHasHit)
{
ForceCenter.Z = FMath::Clamp(BodyLocation.Z, MyLocation.Z - CapsuleHalfHeight, MyLocation.Z + CapsuleHalfHeight);
}

OverlapBody->AddRadialForceToBody(ForceCenter, RepulsionForceRadius, RepulsionForce * Mass, ERadialImpulseFalloff::RIF_Constant);
}
}
}
}

*/

void UShooterCharacterMovement::AdjustProxyCapsuleSize()
{

	return;
	
}