#pragma once

#include <FierensDriesZombieRuntime/Movement/SteeringBehaviors/SteeringHelpers.h>
#include "Kismet/KismetMathLibrary.h"

const float DEBUG_ARROW_LENGTH = 100.f;

class ASteeringAgent;

// SteeringBehavior base, all steering behaviors should derive from this.
class ISteeringBehavior
{
public:
	ISteeringBehavior() = default;
	virtual ~ISteeringBehavior() = default;

	// Override to implement your own behavior
	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent & Agent) = 0;

	void SetTarget(const FTargetData& NewTarget) { Target = NewTarget; }
	
	template<class T, std::enable_if_t<std::is_base_of_v<ISteeringBehavior, T>>* = nullptr>
	T* As()
	{ return static_cast<T*>(this); }

protected:
	FTargetData Target;
};

class Seek : public ISteeringBehavior 
{
	public:
		Seek() = default;
		virtual ~Seek() override = default;

		virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};

class Flee : public ISteeringBehavior
{
public:
	Flee() = default;
	virtual ~Flee() override = default;

	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};

class Arrive : public ISteeringBehavior
{
public:
	Arrive() = default;
	virtual ~Arrive() override = default;

	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;

	void SetTargetRadius(float radius) { m_TargetRadius = radius; }
	void SetSlowRadius(float radius) { m_SlowRadius = radius; }
	void RestoreSpeed(ASteeringAgent& Agent);

protected:
	float m_TargetRadius = 3.f;
	float m_SlowRadius = 150.f;
	float m_OriginalMaxLinearSpeed = -1.f;
};

class Face : public ISteeringBehavior
{
public:
	Face() = default;
	virtual ~Face() override = default;

	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};

class Pursuit : public ISteeringBehavior
{
public:
	Pursuit() = default;
	virtual ~Pursuit() override = default;

	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};

class Evade : public ISteeringBehavior
{
public:
	Evade() = default;
	Evade(float radius);
	virtual ~Evade() override = default;

	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
private:
	float m_Radius{};
};

class Wander : public Seek
{
public:
	Wander() = default;
	virtual ~Wander() override = default;

	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
	
	void SetWanderOffset(float offset) { m_OffsetDistance = offset; }
	void SetWanderRadius(float radius) { m_Radius = radius; }
	void SetMaxAngleChange(float radians) { m_MaxAngleChange = radians; }
	void SetDebugColor(FColor debugColor) { m_DebugColor = debugColor; }
	
protected:
	float m_OffsetDistance = 6.f;
	float m_Radius = 4.f;
	float m_MaxAngleChange = FMath::DegreesToRadians(45);
	float m_WanderAngle = 0.f;
	FColor m_DebugColor = FColor::Blue;
};
