#include "NavGraph.h"

#include "NavGraphNode.h"

GameAI::NavGraph::NavGraph(std::unique_ptr<TriPolygon> && NavPoly)
	: Graph{false}
	, pNavPoly{std::move(NavPoly)}
{
	CreateNavigationGraph();
}

GameAI::NavGraph::NavGraph(const NavGraph& Other)
	: Graph(false)
{
	Nodes.reserve(Other.Nodes.size());
	for (std::unique_ptr<Node> const & OtherNode : Other.Nodes)
	{
		Nodes.push_back(std::make_unique<NavGraphNode>(*dynamic_cast<NavGraphNode*>(OtherNode.get())));
	}
        
	Connections.reserve(Other.Connections.size());
	for (std::unique_ptr<Connection> const & OtherConnection : Other.Connections)
	{
		Connections.push_back(std::make_unique<Connection>(*OtherConnection.get()));
	}
}

std::unique_ptr<GameAI::NavGraph> GameAI::NavGraph::Clone() const
{
	return std::make_unique<NavGraph>(*this);
}

int GameAI::NavGraph::GetNodeIdFromEdgeIndex(int EdgeIdx) const
{
	if (EdgeIdx >= 0)
	{
		for (auto const & pNode : Nodes)
		{
			if (reinterpret_cast<NavGraphNode*>(pNode.get())->GetEdgeIdx() == EdgeIdx)
			{
				return pNode->GetId();
			}
		}
	}
	
	return Graphs::InvalidNodeId;
}

void GameAI::NavGraph::CreateNavigationGraph()
{
	//1. Go over all the edges of the navigation mesh and create nodes
 auto const& Edges = pNavPoly->GetEdges();
	auto const& Triangles = pNavPoly->GetTriangles();

	auto GetTrianglesFromEdgeIndex = [&](int EdgeIdx)
	{
		std::vector<int> Result{};
		Result.reserve(2);

		for (int TriIdx = 0; TriIdx < static_cast<int>(Triangles.size()); ++TriIdx)
		{
			if (Triangles[TriIdx].HasEdge(Edges[EdgeIdx]))
			{
				Result.push_back(TriIdx);
			}
		}

		return Result;
	};

	for (int EdgeIdx = 0; EdgeIdx < static_cast<int>(Edges.size()); ++EdgeIdx)
	{
		auto const ConnectedTriangles = GetTrianglesFromEdgeIndex(EdgeIdx);
		if (ConnectedTriangles.size() != 2)
		{
			continue;
		}

		FVector const P1 = Edges[EdgeIdx].GetP1(*pNavPoly);
		FVector const P2 = Edges[EdgeIdx].GetP2(*pNavPoly);
		FVector2D const MidPoint{(P1 + P2) * 0.5f};

		AddNode(std::make_unique<NavGraphNode>(MidPoint, EdgeIdx));
	}

	//2. Create connections now that every node is created	
		//2 valid nodes -> 1 connection
		//3 valid nodes -> 3 connections
	for (TriPolygon::Triangle const& Triangle : Triangles)
	{
		std::vector<int> NodeIds{};
		NodeIds.reserve(3);

		for (TriPolygon::Edge const& Edge : Triangle.GetEdges())
		{
			int const EdgeIdx = pNavPoly->FindEdgeIndex(Edge).value_or(Graphs::InvalidNodeId);
			if (EdgeIdx == Graphs::InvalidNodeId)
			{
				continue;
			}

			int const NodeId = GetNodeIdFromEdgeIndex(EdgeIdx);
			if (NodeId != Graphs::InvalidNodeId)
			{
				NodeIds.push_back(NodeId);
			}
		}

		for (int i = 0; i < static_cast<int>(NodeIds.size()); ++i)
		{
			for (int j = i + 1; j < static_cast<int>(NodeIds.size()); ++j)
			{
				AddConnection(NodeIds[i], NodeIds[j]);
			}
		}
	}
		
	//3. Set the connections cost to the actual distance
   SetConnectionCostsToDistances();
}
