// Fill out your copyright notice in the Description page of Project Settings.


#include "Level_PathfindingAStar.h"

#include "GraphTheory/Algorithms/AStar.h"
#include "GraphTheory/Algorithms/BFS.h"
#include "GraphTheory/Algorithms/Heuristics.h"
#include "Shared/GameAISpectator.h"

using namespace GameAI;

// Sets default values
ALevel_PathfindingAStar::ALevel_PathfindingAStar()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevel_PathfindingAStar::BeginPlay()
{
	Super::BeginPlay();
	
	// Disable trimworld
	TrimWorld->bShouldTrimWorld = false;
	
	// Make the view orthogonal for less perspective issues
	if (PlayerController = Cast<APlayerController>(GetWorld()->GetFirstLocalPlayerFromController()->PlayerController); PlayerController)
	{
		if (AGameAISpectator* Player = Cast<AGameAISpectator>(PlayerController->GetPawnOrSpectator()); Player)
		{
			Player->SetCameraProjection(ECameraProjectionMode::Orthographic);
		}
	}
	
	// Spawn the Agent
	Agent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, 
	FVector{0,0,90}, FRotator::ZeroRotator);
	Agent->SetDebugRenderingEnabled(false);
	Agent->SetSteeringBehavior(&PathFollow);
	
	// Create graph & renderer
	Renderer = new GraphRenderer{GetWorld()};
	GraphRenderOptions RenderOptions{};
	RenderOptions.bDrawConnectionWeights = false;
	RenderOptions.bDrawConnections = false;
	RenderOptions.bDrawNodeIds = false;
	RenderOptions.bDrawNodes = false;
	Renderer->SetRenderOptions(RenderOptions);
	
	NodeFactory = new TerrainNodeFactory{};
	TerrainGraph = new TerrainGridGraph{NodeFactory, 10, 10, 200.0f, 1.0f, 
		FVector2D{-1000.0f, -1000.0f}, false};
	
	CalculatePath();
}

void ALevel_PathfindingAStar::BeginDestroy()
{
	Super::BeginDestroy();
	
	delete Renderer;
	delete TerrainGraph;
	delete NodeFactory;
}

void ALevel_PathfindingAStar::BindLevelInputActions()
{
	Super::BindLevelInputActions();

	PlayerEnhancedInputComponent->BindAction(SetStartNodeAction, ETriggerEvent::Triggered, this, 
		&ALevel_PathfindingAStar::SetStartNodeId);
	PlayerEnhancedInputComponent->BindAction(SetEndNodeAction, ETriggerEvent::Triggered, this, 
		&ALevel_PathfindingAStar::SetEndNodeId);
}

// Called every frame
void ALevel_PathfindingAStar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateImGui();

	if (PlayerController)
	{
		if (PlayerController->IsInputKeyDown(EKeys::I))
		{
			SetTileClear();
		}
		if (PlayerController->IsInputKeyDown(EKeys::O))
		{
			SetTileMud();
		}
		if (PlayerController->IsInputKeyDown(EKeys::P))
		{
			SetTileWater();
		}
	}

	Renderer->RenderGraph(*TerrainGraph);
	TerrainGraph->DebugDrawCells(GetWorld());
	TerrainGraph->DrawTerrain(GetWorld());

	for (int Row = 0; Row < TerrainGraph->GetRows(); ++Row)
	{
		for (int Col = 0; Col < TerrainGraph->GetColumns(); ++Col)
		{
			int NodeId = TerrainGraph->GetNodeId(Col, Row);
			GameAI::TerrainNode* Node = TerrainGraph->GetNodeAs<GameAI::TerrainNode>(NodeId);
			if (Node)
			{
				FVector2D NodePos = Node->GetPosition();
				FVector DrawPos(NodePos.X, NodePos.Y, 100.f);
				FColor DebugColor = FColor::White;
				FString TerrainText;

				switch (Node->GetType())
				{
				case GameAI::TerrainNode::Type::Clear:
					DebugColor = FColor::Green;
					TerrainText = TEXT("C");
					break;
				case GameAI::TerrainNode::Type::Mud:
					DebugColor = FColor::Yellow;
					TerrainText = TEXT("M");
					break;
				case GameAI::TerrainNode::Type::Water:
					DebugColor = FColor::Cyan;
					TerrainText = TEXT("W");
					break;
				}

				DrawDebugString(GetWorld(), DrawPos + FVector(0, 0, 20), *TerrainText, nullptr, DebugColor, 0.f, true, 0.8f);
			}
		}
	}

	if (bDrawDebugSearch)
	{
		for (auto* pNode : DebugOpenList)
		{
			FVector drawPos(pNode->GetPosition().X, pNode->GetPosition().Y, 100.f);
			DrawDebugSphere(GetWorld(), drawPos, 12.f, 6, FColor::Blue, false, -1.f);
		}

		for (auto* pNode : DebugClosedList)
		{
			FVector drawPos(pNode->GetPosition().X, pNode->GetPosition().Y, 100.f);
			DrawDebugSphere(GetWorld(), drawPos, 12.f, 6, FColor::Orange, false, -1.f);
		}
	}

	FVector mouseWorldPos(LatestMouseWorldPos.X, LatestMouseWorldPos.Y, 100.f);
	DrawDebugSphere(GetWorld(), mouseWorldPos, 8.f, 4, FColor::Yellow, false, -1.f);
}

void ALevel_PathfindingAStar::CalculatePath()
{
	if (PathStartNodeId != Graphs::InvalidNodeId
		&& PathEndNodeId != Graphs::InvalidNodeId
		&& PathStartNodeId != PathEndNodeId)
	{
		AStar pathfinder = AStar(TerrainGraph, HeuristicFunction);
		TerrainNode* const startNode = TerrainGraph->GetNodeAs<TerrainNode>(PathStartNodeId);
		TerrainNode* const endNode = TerrainGraph->GetNodeAs<TerrainNode>(PathEndNodeId);

		FoundPath = pathfinder.FindPath(startNode, endNode, DebugOpenList, DebugClosedList, GetWorld());
		UE_LOG(LogTemp, Log, TEXT("New path calculated using %hs"), typeid(pathfinder).name());

		if (!FoundPath.empty())
		{
			float PathCost = 0.f;
			for (int i = 0; i < FoundPath.size() - 1; ++i)
			{
				int FromId = FoundPath[i]->GetId();
				int ToId = FoundPath[i + 1]->GetId();
				auto* Connection = TerrainGraph->FindConnection(FromId, ToId);
				if (Connection)
				{
					PathCost += Connection->GetWeight();
				}
			}
			UE_LOG(LogTemp, Log, TEXT("Path found with %d nodes and total cost: %.2f"), FoundPath.size(), PathCost);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No path found between nodes %d and %d"), PathStartNodeId, PathEndNodeId);
		}

		UpdateAgentPath(FoundPath);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("No valid start & end node... Start: %d, End: %d"), PathStartNodeId, PathEndNodeId);
		FoundPath.clear();
		DebugOpenList.clear();
		DebugClosedList.clear();
	}

	std::vector<std::pair<int, FColor>> PathToHighlight{};
	PathToHighlight.push_back({PathStartNodeId, FColor::Green});
	if (!FoundPath.empty())
	{
		for (int Idx = 1; Idx < FoundPath.size() - 1; ++Idx)
		{
			PathToHighlight.push_back({FoundPath[Idx]->GetId(), FColor::Yellow});
		}
	}
	PathToHighlight.push_back({PathEndNodeId, FColor::Red});
	Renderer->SetHighlightedNodes(PathToHighlight);
}

void ALevel_PathfindingAStar::UpdateAgentPath(std::vector<Node*> const& Path)
{
	std::vector<FVector2D> pathPositions{};
	pathPositions.reserve(Path.size());
	for (Node* const pNode : Path)
	{
		pathPositions.emplace_back(pNode->GetPosition());
	}

	PathFollow.SetPath(pathPositions);
	if (pathPositions.size() > 0)
	{
		Agent->SetPosition(pathPositions[0]);
		Agent->SetMaxLinearSpeed(300.f);
	}
}

void ALevel_PathfindingAStar::UpdateImGui()
{
	#pragma region UI
	//UI
	{
		//Setup
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		//Elements
		ImGui::Text("CONTROLS");
		ImGui::Indent();
		ImGui::Text("LMB: Set Path Start");
		ImGui::Text("RMB: Set Path End");
		ImGui::Text("i: Set terrain to Clear");
		ImGui::Text("o: Set terrain to Mud");
		ImGui::Text("p: Set terrain to Water");
		ImGui::Unindent();

		/*Spacing*/ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();

		ImGui::Text("STATS");
		ImGui::Indent();
		ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		ImGui::Unindent();

		/*Spacing*/ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();

		ImGui::Text("A* Pathfinding");
		ImGui::Spacing();

		ImGui::Checkbox("Draw Debug Search", &bDrawDebugSearch);
		ImGui::Spacing();

		if (ImGui::Combo("Heuristic", &SelectedHeuristic, "Manhattan\0Euclidean\0SqEuclidean\0Octile\0Chebyshev", 5))
		{
			switch (SelectedHeuristic)
			{
			case 0:
				HeuristicFunction = HeuristicFunctions::Manhattan;
				break;
			case 1:
				HeuristicFunction = HeuristicFunctions::Euclidean;
				break;
			case 2:
				HeuristicFunction = HeuristicFunctions::SqEuclidean;
				break;
			case 3:
				HeuristicFunction = HeuristicFunctions::Octile;
				break;
			default:
			case 4:
				HeuristicFunction = HeuristicFunctions::Chebyshev;
				break;
			}
		}
		ImGui::Spacing();

		//End
		ImGui::End();
	}
#pragma endregion
}

void ALevel_PathfindingAStar::SetStartNodeId()
{
	int const NewStart = TerrainGraph->GetNodeIdAtPosition(FVector2D{LatestMouseWorldPos});
	if (NewStart >= 0 && NewStart != PathEndNodeId)
	{
		PathStartNodeId = NewStart;
		CalculatePath();
		UE_LOG(LogTemp, Log, TEXT("Start node set to: %d"), PathStartNodeId);
	}
}

void ALevel_PathfindingAStar::SetEndNodeId()
{
	int const NewEnd = TerrainGraph->GetNodeIdAtPosition(FVector2D{LatestMouseWorldPos});
	if (NewEnd >= 0 && NewEnd != PathStartNodeId)
	{
		PathEndNodeId = NewEnd;
		CalculatePath();
		UE_LOG(LogTemp, Log, TEXT("End node set to: %d"), PathEndNodeId);
	}
}

void ALevel_PathfindingAStar::SetTileClear()
{
	TerrainGraph->PaintNodeAtPosition(FVector2D{LatestMouseWorldPos}, TerrainNode::Type::Clear);
	CalculatePath();
	UE_LOG(LogTemp, Log, TEXT("Tile set to Clear at (%.0f, %.0f)"), LatestMouseWorldPos.X, LatestMouseWorldPos.Y);
}

void ALevel_PathfindingAStar::SetTileMud()
{
	TerrainGraph->PaintNodeAtPosition(FVector2D{LatestMouseWorldPos}, TerrainNode::Type::Mud);
	CalculatePath();
	UE_LOG(LogTemp, Log, TEXT("Tile set to Mud at (%.0f, %.0f)"), LatestMouseWorldPos.X, LatestMouseWorldPos.Y);
}

void ALevel_PathfindingAStar::SetTileWater()
{
	TerrainGraph->PaintNodeAtPosition(FVector2D{LatestMouseWorldPos}, TerrainNode::Type::Water);
	CalculatePath();
	UE_LOG(LogTemp, Log, TEXT("Tile set to Water at (%.0f, %.0f)"), LatestMouseWorldPos.X, LatestMouseWorldPos.Y);
}



