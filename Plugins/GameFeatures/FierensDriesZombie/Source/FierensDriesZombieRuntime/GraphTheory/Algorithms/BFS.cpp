#include "BFS.h"

#include <map>
#include <queue>

#include "FierensDriesZombieRuntime/Shared/Graph/Graph.h"

using namespace GameAI;

BFS::BFS(Graph* const pGraph)
	: pGraph(pGraph)
{
}

// TODO Breath First Search Algorithm searches for a path from the startNode to the destinationNode
std::vector<Node*> BFS::FindPath(Node* const pStartNode, Node* const pDestinationNode) const
{
	//Containers
	std::queue<Node*> openList{};
	std::map<Node*, Node*> closedList{}; // Water doesn't have any "connection" so isn't a path!!!

	//Kickstart
	openList.push(pStartNode);

	// While we still have nodes to process, and we didn't early out, we are going to check nodes
	while (!openList.empty()) 
	{
		//Get Node to check
		Node* const pCurrentNode = openList.front();
		openList.pop();

		// Early out if reached destination
		if (pCurrentNode == pDestinationNode)
			break;

		// Loop over all the connection of the current node
		for (Connection* const pConnection : pGraph->FindConnectionsFrom(pCurrentNode->GetId()))
		{
			// Get node of the connection
			Node* const pNextNode = pGraph->GetNode(pConnection->GetToId()).get();

			// Check if this node is already closed list - no double checking
			if (closedList.find(pNextNode) == closedList.end()) 
			{
				openList.push(pNextNode);
				closedList[pNextNode] = pCurrentNode;
			}
		}
	}

	// Backtrack to get final path
	std::vector<Node*> path{};
	Node* pCurrentNode = pDestinationNode;

	while (pCurrentNode != pStartNode)
	{
		path.push_back(pCurrentNode);
		pCurrentNode = closedList.at(pCurrentNode);
	}
	path.push_back(pStartNode);
	std::reverse(path.begin(), path.end());

	return path;
}
