#include "SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

//SEEK
//*******
// TODO: Do the Week01 assignment :^)

SteeringOutput Seek::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Output{};

	Output.LinearVelocity = Target.Position - Agent.GetPosition();

	DrawDebug(Agent);
	return Output;
}

void ISteeringBehavior::DrawDebug(const ASteeringAgent& Agent) const
{
	if (!Agent.GetDebugRenderingEnabled())
	{
		return;
	}
	FVector2D LinearVelocity = Target.Position - Agent.GetPosition();

	const FVector AgentPos{ Agent.GetPosition(), 0.0f };
	FVector Direction{ LinearVelocity.X, LinearVelocity.Y, 0.0f };

	const float LineLength = Agent.GetMaxLinearSpeed();
	FVector TargetPos = AgentPos;

	Direction = Direction.GetSafeNormal();
	TargetPos = AgentPos + Direction * LineLength;

	const FVector CirclePosition{ Target.Position, 0.0f };
	const FVector XAxis{ 1.0f, 0.0f, 0.0f };
	const FVector YAxis{ 0.0f, 1.0f, 0.0f };

	DrawDebugLine(Agent.GetWorld(), AgentPos, TargetPos, FColor::Green, false, -1.0f, 1, 5.0f);
	DrawDebugCircle(Agent.GetWorld(), CirclePosition, 20.0f, 12, FColor::Red, false, -1.0f, 1, 2.0f, XAxis, YAxis);
}

SteeringOutput Flee::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Output = Seek::CalculateSteering(DeltaT, Agent);
	Output.LinearVelocity *= -1.0f;
	DrawDebug(Agent);

	return Output;
}

SteeringOutput Arrive::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Output = Seek::CalculateSteering(DeltaT, Agent);

	if (m_originalMaxSpeed == -1)
	{
		m_originalMaxSpeed = Agent.GetMaxLinearSpeed();
	}

	constexpr float SlowRadius = 500.0f;
	const float DistanceToGo = Output.LinearVelocity.Size();

	if (DistanceToGo < TargetRadius)
	{
		Output.LinearVelocity = FVector2D::ZeroVector;
		Agent.SetMaxLinearSpeed(0.0f);
	}
	else if (DistanceToGo < SlowRadius)
	{
		const float DesiredSpeed = m_originalMaxSpeed * (DistanceToGo / SlowRadius);
		Agent.SetMaxLinearSpeed(DesiredSpeed);
	}
	else
	{
		Agent.SetMaxLinearSpeed(m_originalMaxSpeed);
	}

	const FVector XAxis{ 1.0f, 0.0f, 0.0f };
	const FVector YAxis{ 0.0f, 1.0f, 0.0f };
	const FVector AgentPos{ Agent.GetPosition(), 0.0f };

	if (Agent.GetDebugRenderingEnabled())
	{
		DrawDebugCircle(Agent.GetWorld(), AgentPos, SlowRadius, 12, FColor::Blue, false, -1.0f, 1, 2.0f, XAxis, YAxis);
		DrawDebugCircle(Agent.GetWorld(), AgentPos, TargetRadius, 12, FColor::Red, false, -1.0f, 1, 2.0f, XAxis, YAxis);
	}

	return Output;
}

SteeringOutput Face::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Output{};
	Agent.SetIsAutoOrienting(false);

	// NO MOVE ONLY FACE
	const FVector2D ToTarget = Target.Position - Agent.GetPosition();

	if (ToTarget.IsNearlyZero())
		return Output;

	const float DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(ToTarget.Y, ToTarget.X));
	const float CurrentYaw = Agent.GetActorRotation().Yaw;

	const float AngleDiff = FMath::FindDeltaAngleDegrees(CurrentYaw, DesiredYaw);

	constexpr float TargetAngleTolerance = 1.0f; 
	constexpr float SlowAngleRadius = 30.0f;     

	const float MaxAngularSpeed = Agent.GetMaxAngularSpeed();

	float DesiredAngularSpeed = 0.0f;
	const float AbsDiff = FMath::Abs(AngleDiff);

	if (AbsDiff <= TargetAngleTolerance)
	{
		DesiredAngularSpeed = 0.0f;
	}
	else if (AbsDiff >= SlowAngleRadius)
	{
		DesiredAngularSpeed = FMath::Sign(AngleDiff) * MaxAngularSpeed;
	}
	else
	{
		DesiredAngularSpeed = (AngleDiff / SlowAngleRadius) * MaxAngularSpeed;
	}

	Output.LinearVelocity = FVector2D::ZeroVector;
	Output.AngularVelocity = DesiredAngularSpeed;

	{
		const FVector AgentPos{ Agent.GetPosition(), 0.0f };
		const float OrientationRad = FMath::DegreesToRadians(CurrentYaw);
		const float LineLen = 50.0f;
		const FVector Dir{ FMath::Cos(OrientationRad), FMath::Sin(OrientationRad), 0.0f };
		const FVector End = AgentPos + Dir * LineLen;
		if (Agent.GetDebugRenderingEnabled())
		{
			DrawDebugLine(Agent.GetWorld(), AgentPos, End, FColor::Yellow, false, -1.0f, 1, 3.0f);
		}
	}

	return Output;
}

SteeringOutput Wander::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	m_WanderAngle = FMath::FRandRange(m_WanderAngle - m_MaxAngleChange, m_WanderAngle + m_MaxAngleChange);
	const FVector AForward = Agent.GetActorForwardVector() * m_OffsetDistance;
	const FVector2D AForward2D{ AForward };
	const FVector2D Center = Agent.GetPosition() + AForward2D;
	
	FVector2D PointOnCircle{};
	PointOnCircle.X = m_Radius * cos(m_WanderAngle);
	PointOnCircle.Y = m_Radius * sin(m_WanderAngle);
	Target.Position = Center + PointOnCircle;

	FVector FCenter{Center, 0};
	const FVector XAxis{ 1.0f, 0.0f, 0.0f };
	const FVector YAxis{ 0.0f, 1.0f, 0.0f };
	if (Agent.GetDebugRenderingEnabled())
	{
		DrawDebugCircle(Agent.GetWorld(), FCenter, m_Radius, 12, FColor::Blue, false, -1.0f, 1, 2.0f, XAxis, YAxis);
	}

	return Seek::CalculateSteering(DeltaT, Agent);
}

SteeringOutput Pursuit::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Output{};

	const FVector2D AgentPos = Agent.GetPosition();
	const FVector2D ToTarget = Target.Position - AgentPos;
	const float Distance = ToTarget.Size();

	if (ToTarget.IsNearlyZero())
	{
		return Output;
	}
	const float AgentMaxSpeed = Agent.GetMaxLinearSpeed();
	float PredictionTime = 0.0f;
	if (AgentMaxSpeed > 0.0f)
	{
		PredictionTime = Distance / AgentMaxSpeed;
	}

	const FVector2D PredictedPosition = Target.Position + Target.LinearVelocity * PredictionTime;
	const FVector2D OriginalTargetPos = Target.Position;
	Target.Position = PredictedPosition;

	Output = Seek::CalculateSteering(DeltaT, Agent);

	Target.Position = OriginalTargetPos;

	return Output;
}

SteeringOutput Evade::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Output{};

	const FVector2D AgentPos = Agent.GetPosition();
	const FVector2D ToTarget = Target.Position - AgentPos;
	const float Distance = ToTarget.Size();

	if (ToTarget.IsNearlyZero())
	{
		return Output;
	}

	const float AgentMaxSpeed = Agent.GetMaxLinearSpeed();
	float PredictionTime = 0.0f;
	if (AgentMaxSpeed > 0.0f)
	{
		PredictionTime = Distance / AgentMaxSpeed;
	}

	const FVector2D PredictedPosition = Target.Position + Target.LinearVelocity * PredictionTime;
	const FVector2D OriginalTargetPos = Target.Position;
	Target.Position = PredictedPosition;

	// Use Flee so the agent moves away from the predicted future position
	Output = Flee::CalculateSteering(DeltaT, Agent);

	Target.Position = OriginalTargetPos;

	return Output;
}