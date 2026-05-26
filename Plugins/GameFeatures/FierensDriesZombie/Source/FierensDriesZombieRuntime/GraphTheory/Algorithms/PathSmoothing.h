#pragma once
#include <vector>

#include "NavGraphPathfinding.h"
#include "VectorTypes.h"
#include "FierensDriesZombieRuntime/Movement/Pathfinding/Navmesh/TriPolygon.h"
#include "FierensDriesZombieRuntime/Shared/Graph/Graph.h"
#include "FierensDriesZombieRuntime/Shared/Graph/NavGraph/NavGraphNode.h"

namespace GameAI
{
	class SSFA final
{
public:
	//=== SSFA Functions ===
	//--- References ---
	//http://digestingduck.blogspot.be/2010/03/simple-stupid-funnel-algorithm.html
	//https://gamedev.stackexchange.com/questions/68302/how-does-the-simple-stupid-funnel-algorithm-work
	static std::vector<NavLine> FindPortals(std::vector<Node*> const& Path, TriPolygon const& NavPoly)
	{
		//Container
		std::vector<NavLine> Portals{};
		
		Portals.push_back(NavLine(Path[0]->GetPosition(), Path[0]->GetPosition()));
		
		//For each node received, get it's corresponding line
		for (size_t i = 1; i < Path.size() - 1; ++i)
		{
			//Local variables
			auto pNode = static_cast<NavGraphNode*>(Path[i]);
			auto pEdge = NavPoly.GetEdges()[pNode->GetEdgeIdx()];

			const FVector edgeP1 = pEdge.GetP1(NavPoly);
			const FVector edgeP2 = pEdge.GetP2(NavPoly);

			const FVector2D portalP1{edgeP1.X, edgeP1.Y};
			const FVector2D portalP2{edgeP2.X, edgeP2.Y};

			//Redetermine it's "orientation" based on the required path (left-right vs right-left) - p1 should be right point
			const FVector2D centerLine = (portalP1 + portalP2) / 2.0f;
			const FVector2D previousPosition = Path[i - 1]->GetPosition();

			const FVector2D toCenter = centerLine - previousPosition;
			const FVector2D toP1 = portalP1 - previousPosition;
			
			const float cross = Cross(toCenter, toP1);

			NavLine portalLine{};
			if (cross > 0.0f)
				portalLine = NavLine(portalP2, portalP1);
			else
				portalLine = NavLine(portalP1, portalP2);

			//Store portal
			Portals.push_back(portalLine);
		}
		
		//Add degenerate portal to force end evaluation
		Portals.push_back(NavLine(Path[Path.size() - 1]->GetPosition(), Path[Path.size() - 1]->GetPosition()));

		return Portals;
	}
		
	static std::vector<FVector2D> OptimizePortals(std::vector<NavLine> const& Portals, TriPolygon const& NavPoly)
	{
		//P1 == right point of portal, P2 == left point of portal
		std::vector<FVector2D> Path{};
		const unsigned int amtPortals{ static_cast<unsigned int>(Portals.size()) };

		if (amtPortals == 0)
			return Path;

		int apexIdx{ 0 }, leftLegIdx{ 1 }, rightLegIdx{ 1 };

		FVector2D apexPos = Portals[apexIdx].P1;
		Path.push_back(apexPos);

		FVector2D rightLeg = Portals[rightLegIdx].P1 - apexPos;
		FVector2D leftLeg = Portals[leftLegIdx].P2 - apexPos;

		for (unsigned int portalIdx = 1; portalIdx < amtPortals; ++portalIdx)
		{
			const auto& portal = Portals[portalIdx];

			//--- RIGHT CHECK ---
			FVector2D newRightLeg = portal.P1 - apexPos;

			// Moving inwards on the right = CCW
			if (Cross(rightLeg, newRightLeg) > 0)
			{
				// Crossed over the left leg
				if (Cross(leftLeg, newRightLeg) > 0)
				{
					apexPos += leftLeg;
					apexIdx = leftLegIdx;

					portalIdx = leftLegIdx + 1;
					leftLegIdx = portalIdx;
					rightLegIdx = portalIdx;

					Path.push_back(apexPos);

					if (portalIdx < amtPortals)
					{
						rightLeg = Portals[rightLegIdx].P1 - apexPos;
						leftLeg = Portals[leftLegIdx].P2 - apexPos;
						continue;
					}
				}
				else
				{
					rightLeg = newRightLeg;
					rightLegIdx = portalIdx;
				}
			}

			//--- LEFT CHECK ---
			FVector2D newLeftLeg = portal.P2 - apexPos;

			// Moving inwards on the left = CW
			if (Cross(leftLeg, newLeftLeg) < 0)
			{
				// Crossed over the right leg
				if (Cross(rightLeg, newLeftLeg) < 0)
				{
					apexPos += rightLeg;
					apexIdx = rightLegIdx;

					portalIdx = rightLegIdx + 1;
					leftLegIdx = portalIdx;
					rightLegIdx = portalIdx;

					Path.push_back(apexPos);

					if (portalIdx < amtPortals)
					{
						rightLeg = Portals[rightLegIdx].P1 - apexPos;
						leftLeg = Portals[leftLegIdx].P2 - apexPos;
						continue;
					}
				}
				else
				{
					leftLeg = newLeftLeg;
					leftLegIdx = portalIdx;
				}
			}
		}

		// Add last path point
		Path.push_back(Portals.back().P1);

		return Path;
	}
		
	static float Cross(const FVector2D& v1, const FVector2D& v2)
	{
		return v1.X * v2.Y - v1.Y * v2.X; 
	};
private:
	SSFA() {};
	~SSFA() {};
};
}
