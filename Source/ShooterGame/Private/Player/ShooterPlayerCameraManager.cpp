// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"

AShooterPlayerCameraManager::AShooterPlayerCameraManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NormalFOV = 90.0f;
	TargetingFOV = 60.0f;

	ViewPitchMin = 0.f;
	ViewPitchMax = 359.999f;
	ViewYawMin = 0.f;
	ViewYawMax = 359.999f;
	ViewRollMin = 0.f;
	ViewRollMax = 359.999f;

	bAlwaysApplyModifiers = true;
}

void AShooterPlayerCameraManager::UpdateCamera(float DeltaTime)
{
	AShooterCharacter* MyPawn = PCOwner ? Cast<AShooterCharacter>(PCOwner->GetPawn()) : NULL;
	if (MyPawn && MyPawn->IsFirstPerson())
	{
		const float TargetFOV = MyPawn->IsTargeting() ? TargetingFOV : NormalFOV;
		DefaultFOV = FMath::FInterpTo(DefaultFOV, TargetFOV, DeltaTime, 20.0f);
	}

	Super::UpdateCamera(DeltaTime);

	if (MyPawn && MyPawn->IsFirstPerson())
	{
		MyPawn->OnCameraUpdate(GetCameraLocation(), GetCameraRotation());
	}
}
