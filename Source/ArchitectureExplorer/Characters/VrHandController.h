// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MotionControllerComponent.h"

#include "VrHandController.generated.h"

//class UMotionControllerComponent;
class UHapticFeedbackEffect_Base;

UCLASS()
class ARCHITECTUREEXPLORER_API AVrHandController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AVrHandController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere)
	UMotionControllerComponent* MotionController;

	UPROPERTY(EditDefaultsOnly)
	UHapticFeedbackEffect_Base* HapticEffect;

	bool bCanClimb = false;
	bool bIsClimbing = false;
	FVector ClimbingStartLocation;
	AVrHandController* OtherController;

	UFUNCTION()
	void ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	
	UFUNCTION()
	void ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor);

	bool CanClimb() const;

public:
	UFUNCTION()
	void SetHand(EControllerHand Hand);

	UFUNCTION()
	void PairController(AVrHandController* Controller);

	UFUNCTION()
	void Grip();

	UFUNCTION()
	void Release();
};
