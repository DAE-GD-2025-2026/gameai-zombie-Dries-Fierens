#include "SpacePartitioning.h"

#include "VectorTypes.h"

// --- Cell ---
// ------------
Cell::Cell(float Left, float Bottom, float Width, float Height)
{
	BoundingBox.Min = { Left, Bottom };
	BoundingBox.Max = { BoundingBox.Min.X + Width, BoundingBox.Min.Y + Height };
}

std::vector<FVector2D> Cell::GetRectPoints() const
{
	const float left = BoundingBox.Min.X;
	const float bottom = BoundingBox.Min.Y;
	const float width = BoundingBox.Max.X - BoundingBox.Min.X;
	const float height = BoundingBox.Max.Y - BoundingBox.Min.Y;

	std::vector<FVector2D> rectPoints =
	{
		{ left , bottom  },
		{ left , bottom + height  },
		{ left + width , bottom + height },
		{ left + width , bottom  },
	};

	return rectPoints;
}

// --- Partitioned Space ---
// -------------------------
CellSpace::CellSpace(UWorld* pWorld, float Width, float Height, int Rows, int Cols, int MaxEntities)
	: pWorld{pWorld}
	, SpaceWidth{Width}
	, SpaceHeight{Height}
	, NrOfRows{Rows}
	, NrOfCols{Cols}
	, Neighbors(MaxEntities)
	, NrOfNeighbors{0}
{
	//calculate bounds of a cell
	CellWidth = Width / Cols;
	CellHeight = Height / Rows;

	// TODO create the cells
	for (int row = 0; row < Rows; ++row)
	{
		for (int col = 0; col < Cols; ++col)
		{
			float left = col * CellWidth;
			float bottom = row * CellHeight;
			Cells.emplace_back(left, bottom, CellWidth, CellHeight);
		}
	}
}

void CellSpace::AddAgent(ASteeringAgent& Agent)
{
	// TODO Add the agent to the correct cell
	int index = PositionToIndex(Agent.GetPosition());
	Cells[index].Agents.push_back(&Agent);
}

void CellSpace::UpdateAgentCell(ASteeringAgent& Agent, const FVector2D& OldPos)
{
	//TODO Check if the agent needs to be moved to another cell.
	//TODO Use the calculated index for oldPos and currentPos for this
	int oldIndex = PositionToIndex(OldPos);
	int newIndex = PositionToIndex(Agent.GetPosition());
	if (oldIndex != newIndex)
	{
		Cells[oldIndex].Agents.remove(&Agent);
		Cells[newIndex].Agents.push_back(&Agent);
	}
}

void CellSpace::RegisterNeighbors(ASteeringAgent& Agent, float QueryRadius)
{
	// TODO Register the neighbors for the provided agent
	// TODO Only check the cells that are within the radius of the neighborhood
	NrOfNeighbors = 0;
	FRect neighborhoodBox;
	float r = QueryRadius * 2;
	FVector2D pos = Agent.GetPosition() - FVector2D(QueryRadius, QueryRadius);
	neighborhoodBox.Min = pos;
	neighborhoodBox.Max = { neighborhoodBox.Min.X + r, neighborhoodBox.Min.Y + r};

	for (Cell& cell : Cells)
	{
		if (DoRectsOverlap(cell.BoundingBox, neighborhoodBox))
		{
			for (ASteeringAgent* agent : cell.Agents)
			{
				if (agent != &Agent && UE::Geometry::DistanceSquared(Agent.GetPosition(), agent->GetPosition()) <= QueryRadius * QueryRadius)
				{
					Neighbors[NrOfNeighbors++] = agent;
				}
			}
		}
	}
}

void CellSpace::EmptyCells()
{
	for (Cell& c : Cells)
		c.Agents.clear();
}

void CellSpace::RenderCells() const
{
	// TODO Render the cells with the number of agents inside of it
	for (const Cell& cell : Cells)
	{
        const FVector2D min2 = cell.BoundingBox.Min;
        const FVector2D max2 = cell.BoundingBox.Max;
		
        const FVector2D center2 = (min2 + max2) * 0.5f;
        const FVector center3(center2.X, center2.Y, 0.f);
		
        const float halfW = (max2.X - min2.X) * 0.5f;
        const float halfH = (max2.Y - min2.Y) * 0.5f;
		
        const FVector extent3(halfW, halfH, 5.f);

        DrawDebugBox(pWorld, center3, extent3, FColor::White);
		
		std::string agentCount = std::to_string(cell.Agents.size());
		FVector2D offset = FVector2D(5, 5);
		FVector textPos = FVector(cell.BoundingBox.Min + offset, 0);
		DrawDebugString(pWorld, textPos, agentCount.c_str());
	}
}

int CellSpace::PositionToIndex(FVector2D const & Pos) const
{
	// TODO Calculate the index of the cell based on the position
	int col = static_cast<int>(Pos.X / CellWidth);
	int row = static_cast<int>(Pos.Y / CellHeight);

	// Clamp to valid range
	col = std::clamp(col, 0, NrOfCols - 1);
	row = std::clamp(row, 0, NrOfRows - 1);

	return row * NrOfCols + col;
}

bool CellSpace::DoRectsOverlap(FRect const & RectA, FRect const & RectB)
{
	// Check if the rectangles are separated on either axis
	if (RectA.Max.X < RectB.Min.X || RectA.Min.X > RectB.Max.X) return false;
	if (RectA.Max.Y < RectB.Min.Y || RectA.Min.Y > RectB.Max.Y) return false;
    
	// If they are not separated, they must overlap
	return true;
}