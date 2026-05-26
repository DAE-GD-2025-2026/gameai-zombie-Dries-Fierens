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
	, pNavPoly(Other.pNavPoly ? std::make_unique<TriPolygon>(*Other.pNavPoly) : nullptr)
{
	Nodes.reserve(Other.Nodes.size());
	for (std::unique_ptr<Node> const & OtherNode : Other.Nodes)
	{
		Nodes.push_back(std::make_unique<NavGraphNode>(*static_cast<NavGraphNode*>(OtherNode.get())));
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
			if (static_cast<NavGraphNode*>(pNode.get())->GetEdgeIdx() == EdgeIdx)
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
	// Create node here
	const auto& edges = pNavPoly->GetEdges();
	for (size_t edgeIdx = 0; edgeIdx < edges.size(); ++edgeIdx)
	{
		const auto& edge = edges[edgeIdx];
		const auto& triangles = pNavPoly->GetTriangles();
		
		int connectedTriangleCount = 0;
		for (const auto& triangle : triangles)
		{
			if (triangle.HasEdge(edge))
			{
				++connectedTriangleCount;
			}
		}
		
		// Check if the line is connected to another triangle
		if (connectedTriangleCount > 1)
		{
			// Add a new NavGraphNode to the Graph
			FVector p1 = edge.GetP1(*pNavPoly.get());
			FVector p2 = edge.GetP2(*pNavPoly.get());
			FVector2D middlePoint = FVector2D((p1 + p2) / 2.0f);
			AddNode(std::make_unique<NavGraphNode>(middlePoint, static_cast<int>(edgeIdx)));
		}
	}

	//2. Create connections now that every node is created	
	//2 valid nodes -> 1 connection
	//3 valid nodes -> 3 connections
	const auto& triangles = pNavPoly->GetTriangles();
	for (const auto& triangle : triangles)
	{
		std::vector<int> nodeIds;

		// Loop over the edge indexes of the triangle
		for (const auto& edge : triangle.GetEdges())
		{
			const auto edgeIdx = pNavPoly->FindEdgeIndex(edge);
			if (!edgeIdx.has_value())
			{
				continue;
			}
			
			int nodeId = GetNodeIdFromEdgeIndex(edgeIdx.value());
			if (nodeId != Graphs::InvalidNodeId)
			{
				nodeIds.push_back(nodeId);
			}
		}

		// Create connections based on the number of valid nodes
		if (nodeIds.size() == 2)
		{
			AddConnection(std::make_unique<Connection>(nodeIds[0], nodeIds[1]));
		}
		else if (nodeIds.size() == 3)
		{
			AddConnection(std::make_unique<Connection>(nodeIds[0], nodeIds[1]));
			AddConnection(std::make_unique<Connection>(nodeIds[1], nodeIds[2]));
			AddConnection(std::make_unique<Connection>(nodeIds[2], nodeIds[0]));
		}
	}

	//3. Set the connections cost to the actual distance
	SetConnectionCostsToDistances();
}
