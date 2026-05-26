// Fill out your copyright notice in the Description page of Project Settings.

#include "FSMComponent.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

namespace GameAI::FSM
{
	State::State(const FName InName)
		: Name(InName)
	{
	}

	void State::Enter()
	{
	}

	void State::Tick(float DeltaTime)
	{
	}

	void State::Exit()
	{
	}

	void State::Initialize(::UFSMComponent* InOwnerComponent, UBlackboardComponent* InBlackboard)
	{
		OwnerComponent = InOwnerComponent;
		Blackboard = InBlackboard;
	}

	void State::SetBlackboard(UBlackboardComponent* InBlackboard)
	{
		Blackboard = InBlackboard;
	}

	::UFSMComponent* State::GetOwnerComponent() const
	{
		return OwnerComponent;
	}

	UBlackboardComponent* State::GetBlackboard() const
	{
		return Blackboard;
	}

	FName State::GetName() const
	{
		return Name;
	}

	FSM::FSM(::UFSMComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

	void FSM::SetOwnerComponent(::UFSMComponent* InOwnerComponent)
	{
		OwnerComponent = InOwnerComponent;

		for (const std::unique_ptr<State>& StatePtr : States)
		{
			StatePtr->Initialize(OwnerComponent, Blackboard);
		}
	}

	void FSM::SetBlackboard(UBlackboardComponent* InBlackboard)
	{
		Blackboard = InBlackboard;

		for (const std::unique_ptr<State>& StatePtr : States)
		{
			StatePtr->SetBlackboard(Blackboard);
		}
	}

	void FSM::AddState(std::unique_ptr<State>&& NewState)
	{
		if (!NewState)
		{
			return;
		}

		NewState->Initialize(OwnerComponent, Blackboard);

		if (InitialState == nullptr)
		{
			InitialState = NewState.get();
		}

		States.push_back(std::move(NewState));
	}

	void FSM::AddTransition(State* From, State* To, std::function<bool()> EvalFunc)
	{
		if (From == nullptr || To == nullptr || !EvalFunc)
		{
			return;
		}

		Transitions.push_back(Transition{From, To, std::move(EvalFunc)});
	}

	void FSM::SetInitialState(State* NewInitialState)
	{
		InitialState = NewInitialState;
	}

	State* FSM::GetCurrentState() const
	{
		return CurrentState;
	}

	State* FSM::GetInitialState() const
	{
		return InitialState;
	}

	void FSM::Start()
	{
		if (CurrentState != nullptr)
		{
			return;
		}

		if (InitialState == nullptr && !States.empty())
		{
			InitialState = States.front().get();
		}

		ChangeState(InitialState);
	}

	void FSM::Stop()
	{
		if (CurrentState != nullptr)
		{
			CurrentState->Exit();
			CurrentState = nullptr;
		}
	}

	void FSM::Update(float DeltaTime)
	{
		if (CurrentState == nullptr)
		{
			return;
		}

		for (const Transition& Transition : Transitions)
		{
			if (Transition.From == CurrentState && Transition.EvalFunc())
			{
				ChangeState(Transition.To);
				break;
			}
		}

		if (CurrentState != nullptr)
		{
			CurrentState->Tick(DeltaTime);
		}
	}

	void FSM::ChangeState(State* NewState)
	{
		if (NewState == CurrentState)
		{
			return;
		}

		if (CurrentState != nullptr)
		{
			CurrentState->Exit();
		}

		CurrentState = NewState;

		if (CurrentState != nullptr)
		{
			CurrentState->Enter();
		}
	}
}

// Sets default values for this component's properties
UFSMComponent::UFSMComponent()
	: FSMInstance(std::make_unique<GameAI::FSM::FSM>(this))
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFSMComponent::AddState(std::unique_ptr<GameAI::FSM::State>&& NewState)
{
	if (!ensure(FSMInstance != nullptr))
	{
		return;
	}

	RefreshBlackboard();
	FSMInstance->AddState(std::move(NewState));
}

void UFSMComponent::AddTransition(GameAI::FSM::State* From, GameAI::FSM::State* To, std::function<bool()> EvalFunc)
{
	if (!ensure(FSMInstance != nullptr))
	{
		return;
	}

	FSMInstance->AddTransition(From, To, std::move(EvalFunc));
}

void UFSMComponent::SetInitialState(GameAI::FSM::State* NewInitialState)
{
	if (!ensure(FSMInstance != nullptr))
	{
		return;
	}

	FSMInstance->SetInitialState(NewInitialState);
}

// Called when the game starts
void UFSMComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshBlackboard();
}

// Called every frame
void UFSMComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsRunning || !FSMInstance)
	{
		return;
	}

	FSMInstance->Update(DeltaTime);
}

void UFSMComponent::StartLogic()
{
	Super::StartLogic();

	if (!ensure(FSMInstance != nullptr))
	{
		return;
	}

	RefreshBlackboard();
	bIsRunning = true;
	FSMInstance->Start();
}

void UFSMComponent::StopLogic(const FString& Reason)
{
	Super::StopLogic(Reason);

	bIsRunning = false;

	if (FSMInstance)
	{
		FSMInstance->Stop();
	}
}

bool UFSMComponent::IsRunning() const
{
	return bIsRunning;
}

GameAI::FSM::State* UFSMComponent::GetCurrentState() const
{
	return FSMInstance ? FSMInstance->GetCurrentState() : nullptr;
}

void UFSMComponent::RefreshBlackboard()
{
	if (!FSMInstance)
	{
		return;
	}

	UBlackboardComponent* BlackboardComponent = nullptr;

	if (AAIController* AIController = Cast<AAIController>(GetOwner()))
	{
		BlackboardComponent = AIController->GetBlackboardComponent();
	}

	FSMInstance->SetOwnerComponent(this);
	FSMInstance->SetBlackboard(BlackboardComponent);
}