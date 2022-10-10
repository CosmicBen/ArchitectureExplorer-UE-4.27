// Fill out your copyright notice in the Description page of Project Settings.

#include "VrHandController.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

AVrHandController::AVrHandController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	MotionController->bDisplayDeviceModel = true;

	SetRootComponent(MotionController);
}

void AVrHandController::BeginPlay()
{
	Super::BeginPlay();

	OnActorBeginOverlap.AddDynamic(this, &AVrHandController::ActorBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &AVrHandController::ActorEndOverlap);
}

void AVrHandController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsClimbing)
	{
		FVector HandControllerDelta = GetActorLocation() - ClimbingStartLocation;
		GetAttachParentActor()->AddActorWorldOffset(-HandControllerDelta);
		//SetActorLocation(ClimbingStartLocation);
	}
}

void AVrHandController::SetHand(EControllerHand Hand)
{
	MotionController->SetTrackingSource(Hand);
}

void AVrHandController::PairController(AVrHandController* Controller)
{
	OtherController = Controller;
	OtherController->OtherController = this;
}

void AVrHandController::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bool bNewCanClimb = CanClimb();

	if (bNewCanClimb != bCanClimb)
	{
		APawn* Pawn = Cast<APawn>(GetAttachParentActor());
		if (Pawn)
		{
			APlayerController* Controller = Cast<APlayerController>(Pawn->GetController());
			if (Controller)
			{
				Controller->PlayHapticEffect(HapticEffect, MotionController->GetTrackingSource());
			}
		}
	}

	bCanClimb = bNewCanClimb;
}

void AVrHandController::ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bCanClimb = CanClimb();
}

bool AVrHandController::CanClimb() const
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (OverlappingActor->ActorHasTag(TEXT("Climbable")))
		{
			return true;
		}
	}

	return false;
}

void AVrHandController::Grip()
{
	if (!bCanClimb) { return; }

	if (!bIsClimbing)
	{
		ClimbingStartLocation = GetActorLocation();
		bIsClimbing = true;

		OtherController->bIsClimbing = false;

		ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
		if (Character != NULL)
		{
			Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
		}
	}
}

void AVrHandController::Release()
{
	if (bIsClimbing)
	{
		bIsClimbing = false;

		ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
		if (Character != NULL)
		{
			Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		}
	}
}