#pragma once

#include <Movement/SteeringBehaviors/SteeringHelpers.h>
#include "Kismet/KismetMathLibrary.h"

#include "Math/UnrealMathUtility.h"

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
	void SetTargetRadius(float Radius) { TargetRadius = Radius; }
	
	template<class T, std::enable_if_t<std::is_base_of_v<ISteeringBehavior, T>>* = nullptr>
	T* As()
	{ return static_cast<T*>(this); }

protected:
	virtual void DrawDebug(const ASteeringAgent& Agent) const;

protected:
	FTargetData Target;

	float TargetRadius{ 50.f }; 
};

// Your own SteeringBehaviors should follow here...

class Seek : public ISteeringBehavior
{
	public:
		SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};

class Flee : public Seek
{
	public:
		SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};

class Arrive : public Seek
{
	public:
		SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
private:
	float m_originalMaxSpeed{ -1 };
};

class Face : public ISteeringBehavior
{
	public:
		SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};

class Wander : public Seek
{
public:
	void SetWanderOffset(float offset) { m_OffsetDistance = offset; };
	void SetWanderRadius(float radius) { m_Radius = radius; };
	void SetMaxAngleChange(float rad) { m_MaxAngleChange = rad; };

	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
private:
	float m_OffsetDistance = 150;
	float m_Radius = 100;
	float m_MaxAngleChange = FMath::DegreesToRadians(45);
	float m_WanderAngle = 0;
};

class Pursuit : public Seek
{
public:
	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};

class Evade : public Flee
{
public:
	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};