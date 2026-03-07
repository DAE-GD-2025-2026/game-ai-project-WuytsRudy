#pragma once

// Toggle this define to enable/disable spatial partitioning
// #define GAMEAI_USE_SPACE_PARTITIONING

#include "FlockingSteeringBehaviors.h"
#include "Movement/SteeringBehaviors/SteeringAgent.h"
#include "Movement/SteeringBehaviors/SteeringHelpers.h"
#include "Movement/SteeringBehaviors/CombinedSteering/CombinedSteeringBehaviors.h"
#include <memory>
#include <vector>
#include "imgui.h"
#include "../SpacePartitioning/SpacePartitioning.h"

class Flock final
{
public:
	Flock(
	UWorld* pWorld,
	TSubclassOf<ASteeringAgent> AgentClass,
	int FlockSize = 10, 
	float WorldSize = 100.f, 
	ASteeringAgent* const pAgentToEvade = nullptr, 
	bool bTrimWorld = false);

	~Flock();

	void Tick(float DeltaTime);
	void RenderDebug();
	void ImGuiRender(ImVec2 const& WindowPos, ImVec2 const& WindowSize);

    void RegisterNeighbors(ASteeringAgent* const Agent);
    int GetNrOfNeighbors() const { return NrOfNeighbors; }
    const TArray<ASteeringAgent*>& GetNeighbors() const { return Neighbors; }

	FVector2D GetAverageNeighborPos() const;
	FVector2D GetAverageNeighborVelocity() const;

	void SetTarget_Seek(FSteeringParams const & Target);

    void SetAgentToEvade(ASteeringAgent* const AgentToEvade);

    void SetEvadeDistance(float Dist) { EvadeDistance = Dist; }
    float GetEvadeDistance() const { return EvadeDistance; }

    void SetEvaderPosition(const FVector2D& Pos);

private:
	// For debug rendering purposes
	UWorld* pWorld{nullptr};
	
	int FlockSize{0};
	TArray<ASteeringAgent*> Agents{};
    // Partitioning (runtime switchable)
    std::unique_ptr<CellSpace> pPartitionedSpace{};
    TArray<FVector2D> OldPositions{};

    // Neighbor list used by behaviors
    TArray<ASteeringAgent*> Neighbors{};
	
	float NeighborhoodRadius{200.f};
	int NrOfNeighbors{0};

	ASteeringAgent* pAgentToEvade{nullptr};
	
	std::unique_ptr<BlendedSteering> pBlendedSteering{};
	std::unique_ptr<PrioritySteering> pPrioritySteering{};

    // Owned behaviors are stored with RAII to avoid raw owning pointers
    std::vector<std::unique_ptr<ISteeringBehavior>> OwnedBehaviors{};
    // Non-owning observer to the evade behavior (owned in OwnedBehaviors)
    ISteeringBehavior* pEvadeBehavior{ nullptr };

    float EvadeDistance{ 500.f };

	// UI and rendering
	bool DebugRenderSteering{false};
	bool DebugRenderNeighborhood{true};
	bool DebugRenderPartitions{true};
    bool UseSpacePartitioning{true};
    float PartitionWorldSize{1000.f};

	void RenderNeighborhood();
};
