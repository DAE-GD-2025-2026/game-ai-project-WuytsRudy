#include "Flock.h"
#include "FlockingSteeringBehaviors.h"
#include "Shared/ImGuiHelpers.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"


Flock::Flock(
	UWorld* pWorld,
	TSubclassOf<ASteeringAgent> AgentClass,
	int FlockSize,
	float WorldSize,
	ASteeringAgent* const pAgentToEvade,
	bool bTrimWorld)
	: pWorld{pWorld}
	, FlockSize{ FlockSize }
	, pAgentToEvade{pAgentToEvade}
{
	Agents.SetNum(FlockSize);

    for (int i = 0; i < FlockSize; ++i)
    {
        if (pWorld && *AgentClass)
        {
            ASteeringAgent* pAgent = nullptr;
            for (int attempt = 0; attempt < 3 && !pAgent; ++attempt)
            {
                const FVector SpawnLoc = FVector(FMath::FRandRange(-WorldSize, WorldSize), FMath::FRandRange(-WorldSize, WorldSize), 100.f);
                pAgent = pWorld->SpawnActor<ASteeringAgent>(AgentClass, SpawnLoc, FRotator::ZeroRotator);
                if (!pAgent)
                {
                    continue;
                }
                pAgent->PrimaryActorTick.bCanEverTick = false;
                pAgent->SetActorTickEnabled(false);
                Agents[i] = pAgent;
            }
        }
    }

    pPartitionedSpace = std::make_unique<CellSpace>(pWorld, WorldSize * 2.f, WorldSize * 2.f, 10, 10, FlockSize + 5);
    OldPositions.SetNum(Agents.Num());
    for (int i = 0; i < Agents.Num(); ++i)
    {
        if (Agents[i])
        {
            pPartitionedSpace->AddAgent(*Agents[i]);
            OldPositions[i] = Agents[i]->GetPosition();
        }
    }

    // ensure agents respect the flock debug render default
    for (ASteeringAgent* pA : Agents)
    {
        if (pA)
            pA->SetDebugRenderingEnabled(DebugRenderSteering);
    }

    std::vector<BlendedSteering::WeightedBehavior> weights;


    {
        auto coh = std::make_unique<Cohesion>(this);
        auto sep = std::make_unique<Separation>(this);
        auto vel = std::make_unique<VelocityMatch>(this);
        auto seek = std::make_unique<Seek>();
        auto wander = std::make_unique<Wander>();
        wander->SetWanderOffset(50.f);
        wander->SetWanderRadius(30.f);


        OwnedBehaviors.emplace_back(std::move(coh));   
        OwnedBehaviors.emplace_back(std::move(sep));   
        OwnedBehaviors.emplace_back(std::move(vel));   
        OwnedBehaviors.emplace_back(std::move(seek));  
        OwnedBehaviors.emplace_back(std::move(wander));


        weights.push_back({ OwnedBehaviors[0].get(), 0.3f });
        weights.push_back({ OwnedBehaviors[1].get(), 0.5f });
        weights.push_back({ OwnedBehaviors[2].get(), 0.2f });
        weights.push_back({ OwnedBehaviors[3].get(), 0.0f });
        weights.push_back({ OwnedBehaviors[4].get(), 0.1f });
    }

    pBlendedSteering = std::make_unique<BlendedSteering>(weights);

    std::vector<ISteeringBehavior*> pri;
    if (pAgentToEvade)
    {
        auto pEvade = std::make_unique<Evade>();

        FTargetData t;
        t.Position = pAgentToEvade->GetPosition();
        t.LinearVelocity = pAgentToEvade->GetLinearVelocity();
        pEvade->SetTarget(t);
        pEvade->SetActivationDistance(EvadeDistance);

        ISteeringBehavior* pEvadeRaw = pEvade.get();
        pri.push_back(pEvadeRaw);
        OwnedBehaviors.emplace_back(std::move(pEvade));
        pEvadeBehavior = OwnedBehaviors.back().get();
    }
    pri.push_back(pBlendedSteering.get());

    pPrioritySteering = std::make_unique<PrioritySteering>(pri);
}

void Flock::SetEvaderPosition(const FVector2D& Pos)
{
    if (pAgentToEvade)
    {
        pAgentToEvade->SetPosition(Pos);
    }
}

Flock::~Flock()
{

    OwnedBehaviors.clear();
}

void Flock::SetAgentToEvade(ASteeringAgent* const AgentToEvade)
{
    pAgentToEvade = AgentToEvade;
    if (pAgentToEvade && !pEvadeBehavior)
    {
        auto pEvade = std::make_unique<Evade>();
        FTargetData t;
        t.Position = pAgentToEvade->GetPosition();
        t.LinearVelocity = pAgentToEvade->GetLinearVelocity();
        pEvade->SetTarget(t);
        pEvade->SetActivationDistance(EvadeDistance);

        ISteeringBehavior* pEvadeRaw = pEvade.get();
        OwnedBehaviors.emplace_back(std::move(pEvade));
        pEvadeBehavior = OwnedBehaviors.back().get();

        std::vector<ISteeringBehavior*> pri;
        pri.push_back(pEvadeBehavior);
        pri.push_back(pBlendedSteering.get());
        pPrioritySteering = std::make_unique<PrioritySteering>(pri);
    }
}

void Flock::Tick(float DeltaTime)
{
    for (ASteeringAgent* pAgent : Agents)
    {
        if (!pAgent) continue;

        if (pPartitionedSpace)
        {
            const int idx = Agents.IndexOfByKey(pAgent);
            if (idx != INDEX_NONE && OldPositions.IsValidIndex(idx))
            {
                pPartitionedSpace->UpdateAgentCell(*pAgent, OldPositions[idx]);
                OldPositions[idx] = pAgent->GetPosition();
            }
            pPartitionedSpace->RegisterNeighbors(*pAgent, NeighborhoodRadius);

            Neighbors = pPartitionedSpace->GetNeighbors();
            NrOfNeighbors = pPartitionedSpace->GetNrOfNeighbors();
        }
        else
        {
            RegisterNeighbors(pAgent);
        }

        if (pPrioritySteering)
        {
            pAgent->SetSteeringBehavior(pPrioritySteering.get());
        }

        if (pAgentToEvade && pEvadeBehavior)
        {
            FTargetData t;
            t.Position = pAgentToEvade->GetPosition();
            t.LinearVelocity = pAgentToEvade->GetLinearVelocity();

            const float dist = FVector2D::Distance(pAgent->GetPosition(), t.Position);
            if (dist <= EvadeDistance)
            {
                pEvadeBehavior->SetTarget(t);
            }
            else
            {
                pEvadeBehavior->SetTarget(t);
            }
        }

        pAgent->Tick(DeltaTime);

        if (pAgent->GetPosition().Size() > NeighborhoodRadius * 10.f)
        {
            FVector2D pos = pAgent->GetPosition();
            pos = pos.GetSafeNormal() * (NeighborhoodRadius * 5.f);
            pAgent->SetPosition(pos);
        }
    }
}

void Flock::RenderDebug()
{
    for (ASteeringAgent* pAgent : Agents)
    {
        if (!pAgent) continue;
        if (!DebugRenderSteering) continue;
        if (!pAgent->GetDebugRenderingEnabled()) continue;

        const FVector Pos{ pAgent->GetPosition(), 0.f };
        DrawDebugSphere(pWorld, Pos, 10.f, 8, FColor::Green, false, 0.0f, 0, 1.f);
    }

    if (DebugRenderNeighborhood)
        RenderNeighborhood();

    if (DebugRenderPartitions && pPartitionedSpace)
    {
        pPartitionedSpace->RenderCells();
    }
}

void Flock::ImGuiRender(ImVec2 const& WindowPos, ImVec2 const& WindowSize)
{
#ifdef PLATFORM_WINDOWS
#pragma region UI
	//UI
	{
		//Setup
		bool bWindowActive = true;
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", &bWindowActive, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		//Elements
		ImGui::Text("CONTROLS");
		ImGui::Indent();
		ImGui::Text("LMB: place target");
		ImGui::Text("RMB: move cam.");
		ImGui::Text("Scrollwheel: zoom cam.");
		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("STATS");
		ImGui::Indent();
		ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("Flocking");
		ImGui::Spacing();

	  if (ImGui::Checkbox("Debug render steering", &DebugRenderSteering))
	  {
	      for (ASteeringAgent* pAgent : Agents)
	      {
	          if (pAgent)
	              pAgent->SetDebugRenderingEnabled(DebugRenderSteering);
	      }
      if (pAgentToEvade)
      {
          pAgentToEvade->SetDebugRenderingEnabled(DebugRenderSteering);
      }
	  }
  ImGui::Checkbox("Debug render neighborhood", &DebugRenderNeighborhood);
    if (ImGui::Checkbox("Use spatial partitioning", &UseSpacePartitioning))
	{
		if (!UseSpacePartitioning)
		{
			pPartitionedSpace.reset();
		}
		else
		{
            const float worldSize = PartitionWorldSize;
            pPartitionedSpace = std::make_unique<CellSpace>(pWorld, worldSize * 2.f, worldSize * 2.f, 10, 10, FlockSize + 5);

			OldPositions.SetNum(Agents.Num());
			for (int i = 0; i < Agents.Num(); ++i)
			{
				if (Agents[i])
				{
					pPartitionedSpace->AddAgent(*Agents[i]);
					OldPositions[i] = Agents[i]->GetPosition();
				}
			}
		}
	}
	ImGui::Checkbox("Debug render partitions", &DebugRenderPartitions);
  ImGui::Spacing();
  ImGui::Text("Evade");
  ImGui::Indent();
  float ed = EvadeDistance;
  if (ImGuiHelpers::ImGuiSliderFloatWithSetter("Evade Range", ed, 0.f, 2000.f, [this](float InVal) { this->SetEvadeDistance(InVal); }, "%.0f")) { }

  if (pAgentToEvade)
  {
      bool bShow = pAgentToEvade->GetDebugRenderingEnabled();
      if (ImGui::Checkbox("Show evader", &bShow))
      {
          pAgentToEvade->SetDebugRenderingEnabled(bShow);
      }

      FVector2D pos = pAgentToEvade->GetPosition();
      float x = pos.X;
      float y = pos.Y;
      if (ImGuiHelpers::ImGuiSliderFloatWithSetter("Evader X", x, -5000.f, 5000.f, [this](float InVal) { if (this->pAgentToEvade) { FVector2D p = this->pAgentToEvade->GetPosition(); p.X = InVal; this->SetEvaderPosition(p); } }, "%.0f")) {}
      if (ImGuiHelpers::ImGuiSliderFloatWithSetter("Evader Y", y, -5000.f, 5000.f, [this](float InVal) { if (this->pAgentToEvade) { FVector2D p = this->pAgentToEvade->GetPosition(); p.Y = InVal; this->SetEvaderPosition(p); } }, "%.0f")) {}
  }
  else
  {
      ImGui::Text("No evader assigned");
  }

  ImGui::Unindent();

		ImGui::Text("Behavior Weights");
		ImGui::Spacing();

  if (pBlendedSteering)
  {
      auto &wb = pBlendedSteering->GetWeightedBehaviorsRef();
      for (int i = 0; i < (int)wb.size(); ++i)
      {
          FString label;
          switch(i)
          {
          case 0: label = TEXT("Cohesion weight"); break;
          case 1: label = TEXT("Separation weight"); break;
          case 2: label = TEXT("Velocity Match weight"); break;
          case 3: label = TEXT("Seek weight"); break;
          case 4: label = TEXT("Wander weight"); break;
          default: label = FString::Printf(TEXT("Behavior %d weight"), i); break;
          }
          float val = wb[i].Weight;
          ImGuiHelpers::ImGuiSliderFloatWithSetter(TCHAR_TO_ANSI(*label), val, 0.f, 2.f,
              [&wb, i](float InVal) { wb[i].Weight = InVal; }, "%.2f");
      }
  }
		ImGui::End();
	}
#pragma endregion
#endif
}

void Flock::RenderNeighborhood()
{
    if (Agents.Num() == 0) return;
    ASteeringAgent* pAgent = Agents[0];
    if (!pAgent) return;

    RegisterNeighbors(pAgent);

    for (ASteeringAgent* nb : Neighbors)
    {
        if (!nb) continue;
        const FVector A{ pAgent->GetPosition(), 0.f };
        const FVector B{ nb->GetPosition(), 0.f };
        DrawDebugLine(pWorld, A, B, FColor::Blue, false, -1.f, 0, 1.f);
    }
}

#ifndef GAMEAI_USE_SPACE_PARTITIONING
void Flock::RegisterNeighbors(ASteeringAgent* const pAgent)
{
    Neighbors.Empty();
    NrOfNeighbors = 0;

    if (!pAgent) return;

    const FVector2D myPos = pAgent->GetPosition();

    for (ASteeringAgent* other : Agents)
    {
        if (!other || other == pAgent) continue;

        const float dist = FVector2D::Distance(myPos, other->GetPosition());
        if (dist <= NeighborhoodRadius)
        {
            Neighbors.Add(other);
            NrOfNeighbors++;
        }
    }
}
#endif

FVector2D Flock::GetAverageNeighborPos() const
{
	FVector2D avgPosition = FVector2D::ZeroVector;
    if (NrOfNeighbors == 0) return avgPosition;

    for (ASteeringAgent* nb : Neighbors)
    {
        if (!nb) continue;
        avgPosition += nb->GetPosition();
    }

    avgPosition /= (float)NrOfNeighbors;
    return avgPosition;
}

FVector2D Flock::GetAverageNeighborVelocity() const
{
	FVector2D avgVelocity = FVector2D::ZeroVector;
    if (NrOfNeighbors == 0) return avgVelocity;

    for (ASteeringAgent* nb : Neighbors)
    {
        if (!nb) continue;
        avgVelocity += nb->GetLinearVelocity();
    }

    avgVelocity /= (float)NrOfNeighbors;
    return avgVelocity;
}

void Flock::SetTarget_Seek(FSteeringParams const& Target)
{
    if (!pBlendedSteering) return;

    auto &wb = pBlendedSteering->GetWeightedBehaviorsRef();
    for (auto &w : wb)
    {
        if (w.pBehavior)
        {
            w.pBehavior->SetTarget(Target);
        }
    }
}

