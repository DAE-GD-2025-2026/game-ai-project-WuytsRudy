
#include "CombinedSteeringBehaviors.h"
#include <algorithm>
#include "../SteeringAgent.h"

#include "DrawDebugHelpers.h"

BlendedSteering::BlendedSteering(const std::vector<WeightedBehavior>& WeightedBehaviors)
	:WeightedBehaviors(WeightedBehaviors)
{};

//****************
//BLENDED STEERING
SteeringOutput BlendedSteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput blended = {};

    float totalWeight = 0.f;
    for (const WeightedBehavior& wb : WeightedBehaviors)
    {
        if (wb.pBehavior == nullptr || wb.Weight == 0.f)
            continue;

        SteeringOutput out = wb.pBehavior->CalculateSteering(DeltaT, Agent);

        blended.LinearVelocity += out.LinearVelocity * wb.Weight;
        blended.AngularVelocity += out.AngularVelocity * wb.Weight;
        totalWeight += wb.Weight;
    }

    if (totalWeight > 0.f)
    {
        blended.LinearVelocity /= totalWeight;
        blended.AngularVelocity /= totalWeight;
        blended.IsValid = true;
    }
    else
    {
        blended.IsValid = false;
    }

    if (Agent.GetDebugRenderingEnabled())
    {
        const FVector AgentPos{ Agent.GetPosition(), 0.0f };
        FVector Dir{ blended.LinearVelocity.X, blended.LinearVelocity.Y, 0.0f };
        Dir = Dir.GetSafeNormal();
        const float LineLength = Agent.GetMaxLinearSpeed();
        const FVector End = AgentPos + Dir * LineLength;
        DrawDebugLine(Agent.GetWorld(), AgentPos, End, FColor::Green, false, 0.0f, 1, 3.0f);
    }

    return blended;
}

float* BlendedSteering::GetWeight(ISteeringBehavior* const SteeringBehavior)
{
	auto it = find_if(WeightedBehaviors.begin(),
		WeightedBehaviors.end(),
		[SteeringBehavior](const WeightedBehavior& Elem)
		{
			return Elem.pBehavior == SteeringBehavior;
		}
	);

	if(it!= WeightedBehaviors.end())
		return &it->Weight;
	
	return nullptr;
}

//*****************
//PRIORITY STEERING
SteeringOutput PrioritySteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Steering = {};

	for (ISteeringBehavior* const pBehavior : m_PriorityBehaviors)
	{
		Steering = pBehavior->CalculateSteering(DeltaT, Agent);

		if (Steering.IsValid)
			break;
	}

	//If non of the behavior return a valid output, last behavior is returned
	return Steering;
}