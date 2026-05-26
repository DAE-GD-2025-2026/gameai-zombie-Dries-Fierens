#include "NavGraphPathfinding.h"

#include "AStar.h"
#include "PathSmoothing.h"
#include "VectorTypes.h"
#include "FierensDriesZombieRuntime/Shared/Graph/NavGraph/NavGraph.h"
#include "FierensDriesZombieRuntime/Shared/Graph/NavGraph/NavGraphNode.h"

using namespace GameAI;

std::vector<FVector2D> NavMeshPathfinding::FindPath(const FVector2D& startPos, const FVector2D& endPos,
	NavGraph* const pNavGraph, std::vector<FVector2D>& debugNodePositions, std::vector<NavLine>& debugPortals) 
{
	//Create the path to return
	std::vector<FVector2D> finalPath{};

	//Get the start and endTriangle
	auto* pNavMeshPolygon = pNavGraph->GetNavPolygon();
	const TriPolygon::Triangle* startTriangle = pNavMeshPolygon->GetTriangleAtPosition(startPos, true);
	const TriPolygon::Triangle* endTriangle = pNavMeshPolygon->GetTriangleAtPosition(endPos, true);
	
	//If we don't have a valid startTriangle or endTriangle -> return empty path
	if (!startTriangle || !endTriangle)
		return finalPath;
	
	//If the startTriangle and endTriangle are the same -> return straight line path
	if (startTriangle == endTriangle)
	{
		finalPath.push_back(startPos);
		finalPath.push_back(endPos);
		return finalPath;
	}
	
	//We have valid start/end triangles and they are not the same
	//=> Start looking for a path
	//Copy the graph
	auto clonedGraph = pNavGraph->Clone();
	
	//Create Extra node for the Start Node (Agent's position)
	auto startNode = std::make_unique<NavGraphNode>(startPos, -1);
	int startNodeId = clonedGraph->AddNode(std::move(startNode));
	
	for (const auto& edge : startTriangle->GetEdges())
	{
		const auto edgeIdx = pNavMeshPolygon->FindEdgeIndex(edge);
		if (!edgeIdx.has_value())
		{
			continue;
		}

		const int connectedNodeId = pNavGraph->GetNodeIdFromEdgeIndex(edgeIdx.value());
		if (connectedNodeId != Graphs::InvalidNodeId)
		{
			const float cost = (startPos - clonedGraph->GetNode(connectedNodeId)->GetPosition()).Length();

			auto connection = std::make_unique<Connection>(startNodeId, connectedNodeId);
			connection->SetWeight(cost);
			clonedGraph->AddConnection(std::move(connection));
		}
	}
	
	//Create extra node for the endNode
	auto endNode = std::make_unique<NavGraphNode>(endPos, -1);
	int endNodeId = clonedGraph->AddNode(std::move(endNode));

	for (const auto& edge : endTriangle->GetEdges())
	{
		const auto edgeIdx = pNavMeshPolygon->FindEdgeIndex(edge);
		if (!edgeIdx.has_value())
		{
			continue;
		}

		const int connectedNodeId = pNavGraph->GetNodeIdFromEdgeIndex(edgeIdx.value());
		if (connectedNodeId != Graphs::InvalidNodeId)
		{
			const float cost = (endPos - clonedGraph->GetNode(connectedNodeId)->GetPosition()).Length();

			auto connection = std::make_unique<Connection>(connectedNodeId, endNodeId);
			connection->SetWeight(cost);
			clonedGraph->AddConnection(std::move(connection));
		}
	}
	
	//Run A star on new graph
	auto graph = clonedGraph.get();
	HeuristicFunctions::Heuristic m_heuristicFunction = HeuristicFunctions::Euclidean;
	auto pathfinder = AStar(graph, m_heuristicFunction);

	Node* pStartGraphNode = clonedGraph->GetNode(startNodeId).get();
	Node* pEndGraphNode = clonedGraph->GetNode(endNodeId).get();

	std::vector<Node*> nodes = pathfinder.FindPath(pStartGraphNode, pEndGraphNode);
	
	//Debug Visualisation
	for (const auto& node : nodes)
	{
		debugNodePositions.push_back(node->GetPosition());
		finalPath.push_back(node->GetPosition());
	}
	
	// Extra: Run optimiser on new graph (First check if everything works without SSFA!)
	debugPortals = SSFA::FindPortals(nodes, *pNavGraph->GetNavPolygon());
	finalPath = SSFA::OptimizePortals(debugPortals, *pNavGraph->GetNavPolygon());
	
	return finalPath;
}

std::vector<FVector2D> NavMeshPathfinding::FindPath(const FVector2D& startPos, const FVector2D& endPos, NavGraph* const pNavGraph)
{
	std::vector<FVector2D> debugNodePositions{};
	std::vector<NavLine> debugPortals{};

	return FindPath(startPos, endPos, pNavGraph, debugNodePositions, debugPortals);
}