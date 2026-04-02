#pragma once
#include <vector>

#include "NavGraphPathfinding.h"
#include "Movement/Pathfinding/Navmesh/TriPolygon.h"
#include "Shared/Graph/Graph.h"
#include "Shared/Graph/NavGraph/NavGraphNode.h"

namespace GameAI
{
	class SSFA final
{
public:
	//=== SSFA Functions ===
	//--- References ---
	//http://digestingduck.blogspot.be/2010/03/simple-stupid-funnel-algorithm.html
	//https://gamedev.stackexchange.com/questions/68302/how-does-the-simple-stupid-funnel-algorithm-work
	static std::vector<NavLine> FindPortals(std::vector<Node*> const & Path, TriPolygon const & NavPoly)
	{
		//Container
		std::vector<NavLine> Portals = {};
		
		//For each node received, get it's corresponding line
		
			//Redetermine it's "orientation" based on the required path (left-right vs right-left) - p1 should be right point

			//Store portal

		//Add degenerate portal to force end evaluation

		return Portals;
	}

	static std::vector<FVector2D> OptimizePortals( std::vector<NavLine> const & Portals, TriPolygon const & NavPoly)
	{
      (void)NavPoly;
		std::vector<FVector2D> Path{};
		//P1 == right point of portal, P2 == left point of portal
		if (Portals.empty())
		{
			return Path;
		}

		auto const Cross2D = [](FVector2D const& A, FVector2D const& B)
		{
			return A.X * B.Y - A.Y * B.X;
		};

		auto const TriArea2 = [&](FVector2D const& A, FVector2D const& B, FVector2D const& C)
		{
			return Cross2D(B - A, C - A);
		};

		FVector2D PortalApex = Portals[0].P1;
		FVector2D PortalRight = Portals[0].P1;
		FVector2D PortalLeft = Portals[0].P2;

		int ApexIndex = 0;
		int RightIndex = 0;
		int LeftIndex = 0;

		Path.push_back(PortalApex);

		for (int PortalIdx = 1; PortalIdx < static_cast<int>(Portals.size()); ++PortalIdx)
		{
			FVector2D const NewRight = Portals[PortalIdx].P1;
			FVector2D const NewLeft = Portals[PortalIdx].P2;
		
			//--- RIGHT CHECK ---
			//1. See if moving funnel inwards - RIGHT
			if (TriArea2(PortalApex, PortalRight, NewRight) <= 0.f)
			{
				if (PortalApex.Equals(PortalRight, KINDA_SMALL_NUMBER) || TriArea2(PortalApex, PortalLeft, NewRight) > 0.f)
				{
					PortalRight = NewRight;
					RightIndex = PortalIdx;
				}
			
				//2. See if new line degenerates a line segment - RIGHT
				else
				{
					//Leftleg becomes new apex point
					Path.push_back(PortalLeft);
					PortalApex = PortalLeft;
					ApexIndex = LeftIndex;

					//Calculate new legs (if not the end)
					PortalLeft = PortalApex;
					PortalRight = PortalApex;
					LeftIndex = ApexIndex;
					RightIndex = ApexIndex;
					PortalIdx = ApexIndex;
					continue;
				}
			}

			//--- LEFT CHECK ---
			//1. See if moving funnel inwards - LEFT
			if (TriArea2(PortalApex, PortalLeft, NewLeft) >= 0.f)
			{
				if (PortalApex.Equals(PortalLeft, KINDA_SMALL_NUMBER) || TriArea2(PortalApex, PortalRight, NewLeft) < 0.f)
				{
					PortalLeft = NewLeft;
					LeftIndex = PortalIdx;
				}

				//2. See if new line degenerates a line segment - LEFT
				else
				{
					//Rightleg becomes new apex point
					Path.push_back(PortalRight);
					PortalApex = PortalRight;
					ApexIndex = RightIndex;

					//Calculate new legs (if not the end)
					PortalLeft = PortalApex;
					PortalRight = PortalApex;
					LeftIndex = ApexIndex;
					RightIndex = ApexIndex;
					PortalIdx = ApexIndex;
					continue;
				}
			}
		}

		// Add last path point
		FVector2D const LastPoint = Portals.back().P1;
		if (Path.empty() || !Path.back().Equals(LastPoint, KINDA_SMALL_NUMBER))
		{
			Path.push_back(LastPoint);
		}

		return Path;
	}
private:
	SSFA() {};
	~SSFA() {};
};
}
