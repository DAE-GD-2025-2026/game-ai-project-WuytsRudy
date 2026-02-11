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
	FVector2D LinearVelocity = Target.Position - Agent.GetPosition();

	const FVector AgentPos{ Agent.GetPosition(), 0.0f };
	FVector Direction{ LinearVelocity.X, LinearVelocity.Y, 0.0f };

	constexpr float LineLength = 300.0f;
	FVector TargetPos = AgentPos;

	Direction = Direction.GetSafeNormal();
	TargetPos = AgentPos + Direction * LineLength;

	const FVector CirclePosition{ Target.Position, 0.0f };
	const FVector XAxis{ 1.0f, 0.0f, 0.0f };
	const FVector YAxis{ 0.0f, 1.0f, 0.0f };

	DrawDebugLine(Agent.GetWorld(), AgentPos, TargetPos, FColor::Green, false, -1.0f, 1, 5.0f);
	DrawDebugCircle(Agent.GetWorld(), CirclePosition, 20.0f, 12, FColor::Red, false, -1.0f, 1, 2.0f, XAxis, YAxis);
}
