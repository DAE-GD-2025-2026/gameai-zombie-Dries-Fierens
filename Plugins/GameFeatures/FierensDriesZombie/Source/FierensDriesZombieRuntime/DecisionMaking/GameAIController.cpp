// Fill out your copyright notice in the Description page of Project Settings.

#include "GameAIController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "FSM/FSMComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

namespace GameAIControllerKeys
{
	const FName TargetActorKey{TEXT("TargetActor")};
	const FName LastKnownTargetLocationKey{TEXT("LastKnownTargetLocation")};
}

// Sets default values
AGameAIController::AGameAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	BrainComponent = CreateDefaultSubobject<UFSMComponent>(TEXT("FSMComponent"));

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 500.f;
	SightConfig->LoseSightRadius = 600.f;
	SightConfig->PeripheralVisionAngleDegrees = 60.f;
	SightConfig->SetMaxAge(1.5f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 75.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

// Called when the game starts or when spawned
void AGameAIController::BeginPlay()
{
	Super::BeginPlay();

	InitFiniteStateMachine();

	if (AIPerceptionComponent != nullptr)
	{
		AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
			this,
			&AGameAIController::HandleTargetPerceptionUpdated);
	}
}

// Called every frame
void AGameAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGameAIController::InitFiniteStateMachine()
{
	UFSMComponent* FSMComp = FindComponentByClass<UFSMComponent>();
	if (ensure(FSMComp) && FSMBlackboardAsset != nullptr)
	{
		UBlackboardComponent* BlackboardComp = Blackboard;
		UseBlackboard(FSMBlackboardAsset, BlackboardComp);
		Blackboard = BlackboardComp;
	}
}

void AGameAIController::RunFiniteStateMachine()
{
	UFSMComponent* FSMComp = FindComponentByClass<UFSMComponent>();
	if (ensure(FSMComp))
	{
		FSMComp->StartLogic();
	}
}

void AGameAIController::ConfigureSight(float SightRadius, float LoseSightRadius, float PeripheralVisionAngleDegrees)
{
	if (SightConfig == nullptr || AIPerceptionComponent == nullptr)
	{
		return;
	}

	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	AIPerceptionComponent->RequestStimuliListenerUpdate();
}

AActor* AGameAIController::GetTargetActor() const
{
	const UBlackboardComponent* BlackboardComp = GetBlackboardComponent();
	if (BlackboardComp == nullptr)
	{
		return nullptr;
	}

	return Cast<AActor>(BlackboardComp->GetValueAsObject(GameAIControllerKeys::TargetActorKey));
}

FVector AGameAIController::GetLastKnownTargetLocation() const
{
	const UBlackboardComponent* BlackboardComp = GetBlackboardComponent();
	if (BlackboardComp == nullptr)
	{
		return FVector::ZeroVector;
	}

	return BlackboardComp->GetValueAsVector(GameAIControllerKeys::LastKnownTargetLocationKey);
}

void AGameAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Actor == nullptr)
	{
		return;
	}

	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();
	if (BlackboardComp == nullptr)
	{
		return;
	}

	const FVector SensedLocation =
		Stimulus.StimulusLocation.IsNearlyZero()
			? Actor->GetActorLocation()
			: Stimulus.StimulusLocation;

	if (Stimulus.WasSuccessfullySensed())
	{
		BlackboardComp->SetValueAsObject(GameAIControllerKeys::TargetActorKey, Actor);
		BlackboardComp->SetValueAsVector(GameAIControllerKeys::LastKnownTargetLocationKey, SensedLocation);
		return;
	}

	if (BlackboardComp->GetValueAsObject(GameAIControllerKeys::TargetActorKey) == Actor)
	{
		BlackboardComp->ClearValue(GameAIControllerKeys::TargetActorKey);
		BlackboardComp->SetValueAsVector(GameAIControllerKeys::LastKnownTargetLocationKey, SensedLocation);
	}
}