// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Movement component meant for use with Pawns.
 */

#pragma once
#include "ShooterCharacterMovement.generated.h"

UENUM(BlueprintType)
enum SBGravityMode
{
	GRAVITY_XNEGATIVE         UMETA(DisplayName = "X Negative"),
	GRAVITY_XPOSITIVE         UMETA(DisplayName = "X Positive"),
	GRAVITY_YNEGATIVE         UMETA(DisplayName = "Y Negative"),
	GRAVITY_YPOSITIVE         UMETA(DisplayName = "Y Positive"),
	GRAVITY_ZNEGATIVE         UMETA(DisplayName = "Z Negative"),
	GRAVITY_ZPOSITIVE         UMETA(DisplayName = "Z Positive"),
};

UCLASS()
class UShooterCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

	virtual float GetMaxSpeed() const override;
	/* everything after here is my own movement component made for multiple gravity configurations*/
	UPROPERTY()
		TEnumAsByte<enum SBGravityMode> GravityMode;

	UPROPERTY()
		float WalkableFloorComponent;//In parent class, was WalkableFloorZ

	virtual bool DoJump(bool bReplayingMoves) override;
	virtual void JumpOff(AActor* MovementBaseActor) override;
	virtual void SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode = 0) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	//virtual void PerformAirControl(FVector Direction, float ZDiff) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//virtual void SimulateRootMotion(float DeltaSeconds, const FTransform & LocalRootMotionTransform) override; Header code shows that this function is not virtual.
	//We need to ask if making it virtual in the original UE4 source code is okay.
	virtual void SimulateMovement(float DeltaSeconds) override;
	virtual void UpdateBasedRotation(FRotator &FinalRotation, const FRotator& ReducedRotation) override;//This function seemed a bit confusing as to whether or not we need
	//to re implement with gravity configuration consideration. For now, ill keep it implemented as is, but I have a feeling it may have to changed.
	virtual void PerformMovement(float DeltaSeconds) override;
	//virtual void Crouch(bool bClientSimulation) override;
	//virtual void UnCrouch(bool bClientSimulation) override;//Im keeping the original implementation for now. Many calculations seem to be performed with a Z axis taken into
	//account, however it is true that it refers to the collision component, and this collision component is transformed by the actor itself, therefore the transformation
	//is always going to be relative. We may not need to fiddle with this function.
	FVector GetPenetrationAdjustment(const FHitResult& Hit) const override;
	virtual float SlideAlongSurface(const FVector& Delta, float Time, const FVector& InNormal, FHitResult &Hit, bool bHandleImpact) override;//Super version is called and may need to be overriden as well -_- TODO

	FVector GDSafeNormal2D(const FVector inVector) const;
	float GDSize2D(const FVector inVector) const;
	float GDSizeSquared2D(const FVector inVector) const;
	FVector GDClampSize2D(FVector inVector, float min, float max) const;
	FVector GDClampMaxSize2D(FVector inVector, float maxSize) const;
	bool GDIsWithinEdgeTolerance(const FVector& CapsuleLocation, const FVector& TestImpactPoint, const float CapsuleRadius) const;

	virtual void TwoWallAdjust(FVector &Delta, const FHitResult& Hit, const FVector &OldHitNormal) const override;//Super version is called and may need to be overriden as well -_- TODO
	virtual FVector ComputeSlideVector(const FVector& InDelta, const float Time, const FVector& Normal, const FHitResult& Hit) const override;//TODO
	virtual FVector AdjustUpperHemisphereImpact(const FVector& Delta, const FHitResult& Hit) const override;//TODO
	virtual float ImmersionDepth() override;//TODO
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) override; //TODO
	virtual void PhysFlying(float deltaTime, int32 Iterations) override; //TODO
	virtual void PhysSwimming(float deltaTime, int32 Iterations) override; //TODO
	//virtual void StartSwimming(FVector OldLocation, FVector OldVelocity, float timeTick, float remainingTime, int32 Iterations) const override; // TODO
	virtual void PhysFalling(float DeltaTime, int32 Iterations) override;//TODO
	//virtual FVector GetLedgeMove(const FVector& OldLocation, const FVector& Delta, const FVector& GravDir) const override;//TODO
	virtual void StartFalling(int32 Iterations, float remainingTime, float timeTick, const FVector& Delta, const FVector& subLoc) override;//TODO
	virtual FVector ComputeGroundMovementDelta(const FVector& Delta, const FHitResult& RampHit, const bool bHitFromLineTrace) const override; //TODO
	virtual void MoveAlongFloor(const FVector& InVelocity, const float DeltaSeconds, FStepDownResult* OutStepDownResult) override; //TODO
	virtual void MaintainHorizontalGroundVelocity() override; //TODO
	virtual void PhysWalking(float DeltaTime, int32 Iterations) override; //TODO
	virtual void AdjustFloorHeight() override;//TODO
	virtual void PhysicsRotation(float DeltaTime) override; //TODO
	virtual void PhysicsVolumeChanged(APhysicsVolume* NewVolume) override; //TODO
	virtual bool ShouldJumpOutOfWater(FVector& JumpDir) override; //TODO
	virtual bool CheckWaterJump(FVector CheckPoint, FVector& WallNormal) override; //TODO
	//virtual void AddMomentum(FVector const& Momentum, FVector const& LocationToApplyMomentum, bool bMassIndependent) override; //TODO
	virtual void MoveSmooth(const FVector& InVelocity, const float DeltaSeconds, FStepDownResult* OutStepDownResult) override; //TODO
	virtual bool IsWalkable(const FHitResult& Hit) const override;//TODO
	virtual void ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult = NULL) const override;//TODO
	virtual bool IsValidLandingSpot(const FVector& CapsuleLocation, const FHitResult& Hit) const override; //TODO
	virtual bool ShouldComputePerchResult(const FHitResult& InHit, bool bCheckRadius) const override; //TODO
	virtual bool ComputePerchResult(const float TestRadius, const FHitResult& InHit, const float InMaxFloorDist, FFindFloorResult& OutPerchFloorResult) const override;//TODO
	virtual bool StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult &InHit, FStepDownResult* OutStepDownResult = NULL) override; //TODO
	virtual void HandleImpact(FHitResult const& Impact, float TimeSlice = 0.f, const FVector& MoveDelta = FVector::ZeroVector) override; // TODO
	virtual FVector ConstrainInputAcceleration(const FVector& InputAcceleration) const override; //TODO
	virtual void SmoothClientPosition(float DeltaSeconds) override; //TODO
	//virtual void CapsuleTouched(AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;//TODO
	//virtual void ApplyRepulsionForce(float DeltaTime) override; //TODO

	virtual void AdjustProxyCapsuleSize(void) override;
	//There are still many networking functions that are not Gravity Direction Agnostic, notably compression of rotations around certain axis. More work needs to be done here for sure.


	//Accesor functions (NO override)
public:
	void setGravityMode(SBGravityMode mode);
};

