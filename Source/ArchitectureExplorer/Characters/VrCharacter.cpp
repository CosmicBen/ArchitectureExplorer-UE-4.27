// Fill out your copyright notice in the Description page of Project Settings.

#include "VrCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"

AVrCharacter::AVrCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	VrRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VrRoot"));
	VrRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VrRoot);

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(VrRoot);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessingComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
}

void AVrCharacter::BeginPlay()
{
	Super::BeginPlay();

	DestinationMarker->SetVisibility(false);
	
	if (BlinkerMaterialBase != NULL)
	{
		BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialInstance);
	}

	if (HandControllerClass == NULL) { return; }

	LeftController = GetWorld()->SpawnActor<AVrHandController>(HandControllerClass);
	if (LeftController != NULL)
	{
		LeftController->AttachToComponent(VrRoot, FAttachmentTransformRules::KeepRelativeTransform);
		LeftController->SetOwner(this);

		LeftController->SetHand(EControllerHand::Left);
	}

	RightController = GetWorld()->SpawnActor<AVrHandController>(HandControllerClass);
	if (RightController != NULL)
	{
		RightController->AttachToComponent(VrRoot, FAttachmentTransformRules::KeepRelativeTransform);
		RightController->SetOwner(this);

		RightController->SetHand(EControllerHand::Right);
	}

	if (LeftController) { LeftController->PairController(RightController); }
}

void AVrCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	NewCameraOffset = FVector::VectorPlaneProject(NewCameraOffset, GetActorUpVector());
	AddActorWorldOffset(NewCameraOffset);

	VrRoot->AddWorldOffset(-NewCameraOffset);

	UpdateDestinationMarker();
	UpdateBlinkers();
}

bool AVrCharacter::FindTeleportDestination(TArray<FVector>& OutPath, FVector& OutLocation)
{
	if (RightController == NULL) { return false; }

	FVector Start = RightController->GetActorLocation();
	FVector Look = RightController->GetActorForwardVector();

	FPredictProjectilePathParams Params(TeleportProjectileRadius, Start, Look * TeleportProjectileSpeed, TeleportSimluationTime, ECollisionChannel::ECC_Visibility, this);
	Params.bTraceComplex = true;
	FPredictProjectilePathResult Result;
	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);

	for (FPredictProjectilePathPointData PointData : Result.PathData)
	{
		OutPath.Add(PointData.Location);
	}
	
	FNavLocation NavLocation;
	bool bOnNavMesh = bHit && UNavigationSystemV1::GetCurrent(this)->ProjectPointToNavigation(Result.HitResult.Location, NavLocation, TeleportProjectionExtent);

	if (bOnNavMesh)
	{
		OutLocation = NavLocation.Location;
	}

	return bOnNavMesh;
}

void AVrCharacter::UpdateDestinationMarker()
{
	TArray<FVector> Path;
	FVector Location;

	if (FindTeleportDestination(Path, Location))
	{
		DestinationMarker->SetWorldLocation(Location);
		DestinationMarker->SetVisibility(true);

		DrawTeleportPath(Path);
	}
	else
	{
		DestinationMarker->SetVisibility(false);
		TArray<FVector> EmptyPath;
		DrawTeleportPath(EmptyPath);
	}
}

void AVrCharacter::UpdateBlinkers()
{
	if (RadiusVsVelocity == NULL) { return; }

	float Speed = GetVelocity().Size();
	float Radius = RadiusVsVelocity->GetFloatValue(Speed);
	BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Radius"), Radius);

	FVector2D Center = GetBlinkerCenter();
	BlinkerMaterialInstance->SetVectorParameterValue(TEXT("Center"), FLinearColor(Center.X, Center.Y, 0.0f));
}

void AVrCharacter::DrawTeleportPath(TArray<FVector> Path)
{
	UpdateSpline(Path);

	for (USplineMeshComponent* SplineMesh : TeleportPathMeshPool)
	{
		SplineMesh->SetVisibility(false);
	}

	int32 SegmentNum = Path.Num() - 1;
	for (int32 i = 0; i < SegmentNum; ++i)
	{
		USplineMeshComponent* SplineMesh;

		if (TeleportPathMeshPool.Num() <= i)
		{
			SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetMobility(EComponentMobility::Movable);
			SplineMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);
			SplineMesh->SetStaticMesh(TeleportArchMesh);
			SplineMesh->SetMaterial(0, TeleportArchMaterial);
			SplineMesh->RegisterComponent();
			TeleportPathMeshPool.Add(SplineMesh);
		}

		SplineMesh = TeleportPathMeshPool[i];
		SplineMesh->SetVisibility(true);

		FVector StartPosition, StartTangent, EndPosition, EndTangent;
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, StartPosition, StartTangent);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i+1, EndPosition, EndTangent);
		SplineMesh->SetStartAndEnd(StartPosition, StartTangent, EndPosition, EndTangent);
	}
}

void AVrCharacter::UpdateSpline(TArray<FVector> Path)
{
	TeleportPath->ClearSplinePoints(false);

	for (int32 i = 0; i < Path.Num(); ++i)
	{
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[i]);
		FSplinePoint Point(i, LocalPosition, ESplinePointType::Curve);
		TeleportPath->AddPoint(Point, false);
	}

	TeleportPath->UpdateSpline();
}

FVector2D AVrCharacter::GetBlinkerCenter()
{
	FVector2D ScreenStationaryLocation = FVector2D(0.5f);
	FVector MovementDirection = GetVelocity().GetSafeNormal();
	if (MovementDirection.IsNearlyZero())
	{
		return ScreenStationaryLocation;
	}

	FVector WorldStationaryLocation;
	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) > 0.0f)
	{
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000.0f;
	}
	else
	{
		WorldStationaryLocation = Camera->GetComponentLocation() - MovementDirection * 1000.0f;
	}
	
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC == NULL)
	{
		return ScreenStationaryLocation;
	}

	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);
	PC->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenStationaryLocation);
	ScreenStationaryLocation.X /= SizeX;
	ScreenStationaryLocation.Y /= SizeY;
	
	return ScreenStationaryLocation;
}

// Called to bind functionality to input
void AVrCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Move_Y"), this, &AVrCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Move_X"), this, &AVrCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Teleport"), EInputEvent::IE_Pressed, this, &AVrCharacter::BeginTeleport);
	PlayerInputComponent->BindAction(TEXT("GripLeft"), EInputEvent::IE_Pressed, this, &AVrCharacter::GripLeft);
	PlayerInputComponent->BindAction(TEXT("GripLeft"), EInputEvent::IE_Released, this, &AVrCharacter::ReleaseLeft);
	PlayerInputComponent->BindAction(TEXT("GripRight"), EInputEvent::IE_Pressed, this, &AVrCharacter::GripRight);
	PlayerInputComponent->BindAction(TEXT("GripRight"), EInputEvent::IE_Released, this, &AVrCharacter::ReleaseRight);
}

void AVrCharacter::MoveForward(float Throttle)
{
	AddMovementInput(Throttle * Camera->GetForwardVector());
}

void AVrCharacter::MoveRight(float Throttle)
{
	AddMovementInput(Throttle * Camera->GetRightVector());
}

void AVrCharacter::BeginTeleport()
{
	StartFade(0.0f, 1.0f);

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVrCharacter::FinishTeleport, TeleportFadeTime, false);
}

void AVrCharacter::FinishTeleport()
{
	FVector Destination = DestinationMarker->GetComponentLocation();
	Destination += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * GetActorUpVector();
	SetActorLocation(Destination);

	StartFade(1.0f, 0.0f);
}

void AVrCharacter::StartFade(float FromAlpha, float ToAlpha)
{
	APlayerController* PC = Cast<APlayerController>(GetController());

	if (PC != NULL)
	{
		PC->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportFadeTime, FLinearColor::Black, false, true);
	}
}