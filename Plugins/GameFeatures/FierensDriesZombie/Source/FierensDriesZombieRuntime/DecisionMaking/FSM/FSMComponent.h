// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "CoreMinimal.h"
#include "BrainComponent.h"
#include "FSMComponent.generated.h"

class UBlackboardComponent;
class UFSMComponent;

namespace GameAI::FSM
{
	class State
	{
	public:
		explicit State(const FName InName = NAME_None);
		virtual ~State() = default;

		virtual void Enter();
		virtual void Tick(float DeltaTime);
		virtual void Exit();

		void Initialize(::UFSMComponent* InOwnerComponent, UBlackboardComponent* InBlackboard);
		void SetBlackboard(UBlackboardComponent* InBlackboard);

		::UFSMComponent* GetOwnerComponent() const;
		UBlackboardComponent* GetBlackboard() const;
		FName GetName() const;

	private:
		::UFSMComponent* OwnerComponent{nullptr}; // non-owning
		UBlackboardComponent* Blackboard{nullptr}; // non-owning
		FName Name{NAME_None};
	};

	struct Transition
	{
		State* From{nullptr}; // non-owning
		State* To{nullptr}; // non-owning
		std::function<bool()> EvalFunc;
	};

	class FSM
	{
	public:
		explicit FSM(::UFSMComponent* InOwnerComponent = nullptr);

		void SetOwnerComponent(::UFSMComponent* InOwnerComponent);
		void SetBlackboard(UBlackboardComponent* InBlackboard);

		void AddState(std::unique_ptr<State>&& NewState);
		void AddTransition(State* From, State* To, std::function<bool()> EvalFunc);
		void SetInitialState(State* NewInitialState);

		State* GetCurrentState() const;
		State* GetInitialState() const;

		void Start();
		void Stop();
		void Update(float DeltaTime);

	private:
		void ChangeState(State* NewState);

		::UFSMComponent* OwnerComponent{nullptr}; // non-owning
		UBlackboardComponent* Blackboard{nullptr}; // non-owning
		std::vector<std::unique_ptr<State>> States;
		std::vector<Transition> Transitions;
		State* InitialState{nullptr}; // non-owning
		State* CurrentState{nullptr}; // non-owning
	};
}

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FIERENSDRIESZOMBIERUNTIME_API UFSMComponent : public UBrainComponent
{
	GENERATED_BODY()

public:
	UFSMComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual void StartLogic() override;
	virtual void StopLogic(const FString& Reason) override;
	virtual bool IsRunning() const override;

	void AddState(std::unique_ptr<GameAI::FSM::State>&& NewState);
	void AddTransition(GameAI::FSM::State* From, GameAI::FSM::State* To, std::function<bool()> EvalFunc);
	void SetInitialState(GameAI::FSM::State* NewInitialState);

	GameAI::FSM::State* GetCurrentState() const;

protected:
	virtual void BeginPlay() override;

private:
	void RefreshBlackboard();

	std::unique_ptr<GameAI::FSM::FSM> FSMInstance;
	bool bIsRunning{false};
};