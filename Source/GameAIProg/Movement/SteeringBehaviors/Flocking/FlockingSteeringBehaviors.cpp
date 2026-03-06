#include "FlockingSteeringBehaviors.h"
#include "Flock.h"
#include "../SteeringAgent.h"
#include "../SteeringHelpers.h"
#include "DrawDebugHelpers.h"


//*******************
//COHESION (FLOCKING)
SteeringOutput Cohesion::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
    const FVector2D avgPos = pFlock->GetAverageNeighborPos();

    if (avgPos.IsNearlyZero())
        return SteeringOutput{FVector2D::ZeroVector, 0.f};

    FTargetData t;
    t.Position = avgPos;
    SetTarget(t);

    return Seek::CalculateSteering(deltaT, pAgent);
}

SteeringOutput Separation::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
    SteeringOutput out{ FVector2D::ZeroVector, 0.f };
    const TArray<ASteeringAgent*>& neighbors = pFlock->GetNeighbors();
    const int nr = pFlock->GetNrOfNeighbors();
    if (nr == 0) return out;

    const FVector2D myPos = pAgent.GetPosition();

    for (ASteeringAgent* nb : neighbors)
    {
        if (!nb || nb == &pAgent) continue;
        const FVector2D toNeighbor = myPos - nb->GetPosition();
        const float dist = toNeighbor.Size();
        if (dist <= KINDA_SMALL_NUMBER) continue;
        out.LinearVelocity += toNeighbor.GetSafeNormal() * (1.f / dist);
    }

    out.LinearVelocity *= pAgent.GetMaxLinearSpeed();
    return out;
}

SteeringOutput VelocityMatch::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
    const FVector2D avgVel = pFlock->GetAverageNeighborVelocity();
    SteeringOutput out;
    out.LinearVelocity = avgVel - pAgent.GetLinearVelocity();
    return out;
}
