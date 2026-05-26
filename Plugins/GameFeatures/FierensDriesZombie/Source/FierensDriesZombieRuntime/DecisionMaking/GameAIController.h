// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GameAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBlackboardData;

UCLASS()
class FIERENSDRIESZOMBIERUNTIME_API AGameAIController : public AAIController
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|FSM")
	TObjectPtr<UBlackboardData> FSMBlackboardAsset{nullptr};

	AGameAIController();

	virtual void Tick(float DeltaTime) override;

	void RunFiniteStateMachine();
	void ConfigureSight(float SightRadius, float LoseSightRadius, float PeripheralVisionAngleDegrees);

	AActor* GetTargetActor() const;
	FVector GetLastKnownTargetLocation() const;

protected:
	virtual void BeginPlay() override;
	void InitFiniteStateMachine();

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
	UPROPERTY(VisibleAnywhere, Category="AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent{nullptr};

	UPROPERTY(VisibleAnywhere, Category="AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig{nullptr};
};