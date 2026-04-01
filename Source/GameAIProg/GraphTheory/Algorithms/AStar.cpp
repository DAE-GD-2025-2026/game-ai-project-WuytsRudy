#include "AStar.h"

#include <algorithm>
#include "DrawDebugHelpers.h"

using namespace GameAI;

AStar::AStar(Graph* const pGraph, HeuristicFunctions::Heuristic hFunction)
	: pGraph(pGraph)
	, HeuristicFunction(hFunction)
{
}

std::vector<Node*> AStar::FindPath(Node* const pStartNode, Node* const pGoalNode)
{
	std::vector<Node*> openDebug, closedDebug;
	return FindPath(pStartNode, pGoalNode, openDebug, closedDebug, nullptr);
}

std::vector<Node*> AStar::FindPath(Node* const pStartNode, Node* const pGoalNode, 
	std::vector<Node*>& outOpenListNodes, std::vector<Node*>& outClosedListNodes, UWorld* pDebugWorld)
{
	std::vector<Node*> path{};

	if (!pStartNode || !pGoalNode || pStartNode == pGoalNode)
		return path;

	std::vector<NodeRecord> openList{};
	std::vector<NodeRecord> closedList{};
	NodeRecord currentRecord{};

	NodeRecord startRecord{};
	startRecord.pNode = pStartNode;
	startRecord.pConnection = nullptr;
	startRecord.costSoFar = 0.f;
	startRecord.estimatedTotalCost = GetHeuristicCost(pStartNode, pGoalNode);
	openList.push_back(startRecord);

	while (!openList.empty())
	{
		auto it = std::min_element(openList.begin(), openList.end());
		currentRecord = *it;
		openList.erase(it);

		if (pDebugWorld)
		{
			FVector drawPos(currentRecord.pNode->GetPosition().X, currentRecord.pNode->GetPosition().Y, 100.f);
			DrawDebugSphere(pDebugWorld, drawPos, 15.f, 8, FColor::Cyan, false, 0.05f);
		}

		if (currentRecord.pNode == pGoalNode)
		{
			break;
		}

		auto connections = pGraph->FindConnectionsFrom(currentRecord.pNode->GetId());

		for (auto pConnection : connections)
		{
			Node* pNextNode = pGraph->GetNode(pConnection->GetToId()).get();
			float nextGCost = currentRecord.costSoFar + pConnection->GetWeight();

			auto closedListIt = std::find_if(closedList.begin(), closedList.end(),
				[pNextNode](const NodeRecord& record) { return record.pNode == pNextNode; });

			if (closedListIt != closedList.end())
			{
				if (closedListIt->costSoFar <= nextGCost)
					continue;

				closedList.erase(closedListIt);
			}

			auto openListIt = std::find_if(openList.begin(), openList.end(),
				[pNextNode](const NodeRecord& record) { return record.pNode == pNextNode; });

			if (openListIt != openList.end())
			{
				if (openListIt->costSoFar <= nextGCost)
					continue;

				openList.erase(openListIt);
			}

			NodeRecord newRecord{};
			newRecord.pNode = pNextNode;
			newRecord.pConnection = pConnection;
			newRecord.costSoFar = nextGCost;
			newRecord.estimatedTotalCost = nextGCost + GetHeuristicCost(pNextNode, pGoalNode);

			openList.push_back(newRecord);

			if (pDebugWorld)
			{
				FVector fromPos(currentRecord.pNode->GetPosition().X, currentRecord.pNode->GetPosition().Y, 100.f);
				FVector toPos(pNextNode->GetPosition().X, pNextNode->GetPosition().Y, 100.f);
				DrawDebugLine(pDebugWorld, fromPos, toPos, FColor::Green, false, 0.05f, 0, 1.f);
			}
		}

		closedList.push_back(currentRecord);

		if (pDebugWorld)
		{
			FVector drawPos(currentRecord.pNode->GetPosition().X, currentRecord.pNode->GetPosition().Y, 100.f);
			DrawDebugSphere(pDebugWorld, drawPos, 20.f, 8, FColor::Red, false, 0.05f);
		}
	}

	if (currentRecord.pNode != pGoalNode)
	{
		outOpenListNodes.clear();
		outClosedListNodes.clear();
		return path;
	}

	while (currentRecord.pNode != pStartNode)
	{
		path.push_back(currentRecord.pNode);

		int fromNodeId = currentRecord.pConnection->GetFromId();
		auto closedListIt = std::find_if(closedList.begin(), closedList.end(),
			[fromNodeId](const NodeRecord& record) { return record.pNode->GetId() == fromNodeId; });

		if (closedListIt == closedList.end())
			break;

		currentRecord = *closedListIt;
	}

	path.push_back(pStartNode);
	std::reverse(path.begin(), path.end());

	outOpenListNodes.clear();
	outClosedListNodes.clear();
	for (const auto& record : openList)
		outOpenListNodes.push_back(record.pNode);
	for (const auto& record : closedList)
		outClosedListNodes.push_back(record.pNode);

	return path;
}

float AStar::GetHeuristicCost(Node* const pStartNode, Node* const pEndNode) const
{
	FVector2D toDestination = pGraph->GetNode(pEndNode->GetId())->GetPosition() - pGraph->GetNode(pStartNode->GetId())->GetPosition();
	return HeuristicFunction(abs(toDestination.X), abs(toDestination.Y));
}