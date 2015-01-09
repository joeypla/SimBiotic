// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"

AShooterBot::AShooterBot(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
	AIControllerClass = AShooterAIController::StaticClass();

	UpdatePawnMeshes();

	bUseControllerRotationYaw = true;
	GravityMode = GRAVITY_ZNEGATIVE;

	IsBot = true;
}

bool AShooterBot::IsFirstPerson() const
{
	return false;
}

void AShooterBot::FaceRotation(FRotator NewRotation, float DeltaTime)
{
	IsBot = true;
	FRotator CurrentRotation = FMath::RInterpTo(GetActorRotation(), NewRotation, DeltaTime, 8.0f);
	CurrentRotation.Roll = 0.0f;
	CurrentRotation.Pitch = 0.0f;
	SetActorRotation(CurrentRotation);

	//SetFullControlRotation(CurrentRotation);
	//Super::FaceRotation(CurrentRotation, DeltaTime);
}
