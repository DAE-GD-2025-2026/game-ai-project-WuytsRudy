#include "NavGraphPathfinding.h"

#include "AStar.h"
#include "Heuristics.h"
#include "PathSmoothing.h"
#include "VectorTypes.h"
#include "Shared/Graph/NavGraph/NavGraph.h"
#include "Shared/Graph/NavGraph/NavGraphNode.h"

using namespace GameAI;

std::vector<FVector2D> NavMeshPathfinding::FindPath(const FVector2D& startPos, const FVector2D& endPos,
	NavGraph* const pNavGraph, std::vector<FVector2D>& debugNodePositions, std::vector<NavLine>& debugPortals) 
{
	//Create the path to return
	std::vector<FVector2D> finalPath{};
	debugNodePositions.clear();
	debugPortals.clear();

	if (!pNavGraph || !pNavGraph->GetNavPolygon())
	{
		return finalPath;
	}

	TriPolygon const& NavPoly = *pNavGraph->GetNavPolygon();
	FVector2D CorrectedStart = startPos;
	FVector2D CorrectedEnd = endPos;

	//Get the start and endTriangle
	TriPolygon::Triangle const* pStartTriangle = NavPoly.GetTriangleAtPosition(CorrectedStart, true);
	if (!pStartTriangle)
	{
		pStartTriangle = NavPoly.GetClosestTriangleToPosition(startPos, CorrectedStart);
	}

	TriPolygon::Triangle const* pEndTriangle = NavPoly.GetTriangleAtPosition(CorrectedEnd, true);
	if (!pEndTriangle)
	{
		pEndTriangle = NavPoly.GetClosestTriangleToPosition(endPos, CorrectedEnd);
	}

	if (!pStartTriangle || !pEndTriangle)
	{
		return finalPath;
	}

	if (pStartTriangle == pEndTriangle)
	{
		finalPath.push_back(CorrectedStart);
		finalPath.push_back(CorrectedEnd);
		debugNodePositions = finalPath;
		return finalPath;
	}

	//We have valid start/end triangles and they are not the same
	//=> Start looking for a path
	//Copy the graph
	std::unique_ptr<NavGraph> PathGraph = pNavGraph->Clone();
	if (!PathGraph)
	{
		return finalPath;
	}

	//Create Extra node for the Start Node (Agent's position
	int const StartNodeId = PathGraph->AddNode(std::make_unique<NavGraphNode>(CorrectedStart, Graphs::InvalidNodeId));
	for (TriPolygon::Edge const& Edge : pStartTriangle->GetEdges())
	{
		int const EdgeIdx = NavPoly.FindEdgeIndex(Edge).value_or(Graphs::InvalidNodeId);
		if (EdgeIdx == Graphs::InvalidNodeId)
		{
			continue;
		}

		int const NodeId = PathGraph->GetNodeIdFromEdgeIndex(EdgeIdx);
		if (NodeId != Graphs::InvalidNodeId)
		{
			PathGraph->AddConnection(StartNodeId, NodeId);
		}
	}

	//Create extra node for the endNode
	int const EndNodeId = PathGraph->AddNode(std::make_unique<NavGraphNode>(CorrectedEnd, Graphs::InvalidNodeId));
	for (TriPolygon::Edge const& Edge : pEndTriangle->GetEdges())
	{
		int const EdgeIdx = NavPoly.FindEdgeIndex(Edge).value_or(Graphs::InvalidNodeId);
		if (EdgeIdx == Graphs::InvalidNodeId)
		{
			continue;
		}

		int const NodeId = PathGraph->GetNodeIdFromEdgeIndex(EdgeIdx);
		if (NodeId != Graphs::InvalidNodeId)
		{
			PathGraph->AddConnection(EndNodeId, NodeId);
		}
	}

	PathGraph->SetConnectionCostsToDistances();

	//Run A star on new graph
	AStar Pathfinder{PathGraph.get(), HeuristicFunctions::Euclidean};
	std::vector<Node*> Nodes = Pathfinder.FindPath(PathGraph->GetNode(StartNodeId).get(), PathGraph->GetNode(EndNodeId).get());
	if (Nodes.empty())
	{
		return finalPath;
	}

	//Debug Visualisation
	debugNodePositions.reserve(Nodes.size());
	for (Node const* pNode : Nodes)
	{
		debugNodePositions.push_back(pNode->GetPosition());
	}

	// Extra: Run optimiser on new graph (First check if everything works without SSFA!)
    debugPortals = SSFA::FindPortals(Nodes, NavPoly);
	finalPath = SSFA::OptimizePortals(debugPortals, NavPoly);
	if (finalPath.empty())
	{
		finalPath = debugNodePositions;
	}
	
	return finalPath;
}

std::vector<FVector2D> NavMeshPathfinding::FindPath(const FVector2D& startPos, const FVector2D& endPos, NavGraph* const pNavGraph)
{
	std::vector<FVector2D> debugNodePositions{};
	std::vector<NavLine> debugPortals{};

	return FindPath(startPos, endPos, pNavGraph, debugNodePositions, debugPortals);
}