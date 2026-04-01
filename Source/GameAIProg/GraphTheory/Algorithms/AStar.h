#pragma once

#include <vector>
#include "Shared/Graph/Graph.h"
#include "Heuristics.h"

class UWorld;

namespace GameAI
{
	class AStar
	{
	public:
		AStar(Graph* const pGraph, HeuristicFunctions::Heuristic hFunction);

		struct NodeRecord final
		{
			Node* pNode = nullptr;
			Connection* pConnection = nullptr;
			float costSoFar = 0.f;
			float estimatedTotalCost = 0.f;

			bool operator==(const NodeRecord& other) const
			{
				return pNode == other.pNode
					&& pConnection == other.pConnection
					&& costSoFar == other.costSoFar
					&& estimatedTotalCost == other.estimatedTotalCost;
			};

			bool operator<(const NodeRecord& other) const
			{
				return estimatedTotalCost < other.estimatedTotalCost;
			};
		};

		std::vector<Node*> FindPath(Node* const pStartNode, Node* const pDestinationNode);
		std::vector<Node*> FindPath(Node* const pStartNode, Node* const pDestinationNode, 
			std::vector<Node*>& outOpenListNodes, std::vector<Node*>& outClosedListNodes, UWorld* pDebugWorld = nullptr);

	private:
		float GetHeuristicCost(Node* const pStartNode, Node* const pEndNode) const;

		Graph* pGraph;
		HeuristicFunctions::Heuristic HeuristicFunction;
	};
}
