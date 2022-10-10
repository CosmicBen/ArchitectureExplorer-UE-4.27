// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VrHandController.h"
#include "VrCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UPostProcessComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UCurveFloat;
class USplineComponent;
class USplineMeshComponent;
//class AVrHandController;

UCLASS()
class ARCHITECTUREEXPLORER_API AVrCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVrCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY()
	UCameraComponent* Camera;

	UPROPERTY()
	AVrHandController* LeftController;

	UPROPERTY()
	AVrHandController* RightController;

	UPROPERTY()
	USceneComponent* VrRoot;

	UPROPERTY(VisibleAnywhere)
	USplineComponent* TeleportPath;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* DestinationMarker;

	UPROPERTY()
	UPostProcessComponent* PostProcessComponent;


	UPROPERTY(EditAnywhere)
	float TeleportProjectileRadius = 10.0f;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileSpeed = 800.0f;

	UPROPERTY(EditAnywhere)
	float TeleportSimluationTime = 1.0f;

	UPROPERTY(EditAnywhere)
	float TeleportFadeTime = 2.0f;

	UPROPERTY(EditAnywhere)
	FVector TeleportProjectionExtent = FVector(100.0f);

	UPROPERTY(EditAnywhere)
	UMaterialInterface* BlinkerMaterialBase;
	
	UPROPERTY()
	UMaterialInstanceDynamic* BlinkerMaterialInstance;

	UPROPERTY()
	TArray<USplineMeshComponent*> TeleportPathMeshPool;

	UPROPERTY(EditAnywhere)
	UCurveFloat* RadiusVsVelocity;

	UPROPERTY(EditDefaultsOnly)
	UStaticMesh* TeleportArchMesh;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* TeleportArchMaterial;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AVrHandController> HandControllerClass;

	bool FindTeleportDestination(TArray<FVector>& OutPath, FVector& OutLocation);
	void UpdateDestinationMarker();
	void UpdateBlinkers();
	void DrawTeleportPath(TArray<FVector> Path);
	void UpdateSpline(TArray<FVector> Path);

	FVector2D GetBlinkerCenter();

	void MoveForward(float Throttle);
	void MoveRight(float Throttle);
	
	void GripLeft()		{ if (LeftController) { LeftController->Grip(); } }
	void ReleaseLeft()	{ if (LeftController) { LeftController->Release(); } }
	void GripRight()	{ if (RightController) { RightController->Grip(); } }
	void ReleaseRight() { if (RightController) { RightController->Release(); } }

	void BeginTeleport();
	void FinishTeleport();

	void StartFade(float FromAlpha, float ToAlpha);
};
