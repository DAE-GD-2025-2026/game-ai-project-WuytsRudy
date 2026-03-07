#include "SpacePartitioning.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

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
	, NrOfNeighbors{0}
{
	Neighbors.SetNum(MaxEntities);
	
	//calculate bounds of a cell
	CellWidth = Width / Cols;
	CellHeight = Height / Rows;


	CellOrigin = FVector2D(-Width * 0.5f, -Height * 0.5f);

	Cells.clear();
	Cells.reserve(Rows * Cols);
	for (int r = 0; r < Rows; ++r)
	{
		for (int c = 0; c < Cols; ++c)
		{
			const float left = CellOrigin.X + c * CellWidth;
			const float bottom = CellOrigin.Y + r * CellHeight;
			Cells.emplace_back(left, bottom, CellWidth, CellHeight);
		}
	}
}

void CellSpace::AddAgent(ASteeringAgent& Agent)
{
	const int idx = PositionToIndex(Agent.GetPosition());
	if (idx < 0 || idx >= (int)Cells.size()) return;

	Cells[idx].Agents.push_back(&Agent);
}

void CellSpace::UpdateAgentCell(ASteeringAgent& Agent, const FVector2D& OldPos)
{
	const int oldIdx = PositionToIndex(OldPos);
	const int newIdx = PositionToIndex(Agent.GetPosition());

	if (oldIdx == newIdx) return;

	if (oldIdx >= 0 && oldIdx < (int)Cells.size())
	{
		auto &lst = Cells[oldIdx].Agents;
		for (auto it = lst.begin(); it != lst.end(); ++it)
		{
			if (*it == &Agent)
			{
				lst.erase(it);
				break;
			}
		}
	}

	if (newIdx >= 0 && newIdx < (int)Cells.size())
	{
		Cells[newIdx].Agents.push_back(&Agent);
	}
}

void CellSpace::RegisterNeighbors(ASteeringAgent& Agent, float QueryRadius)
{
	Neighbors.Empty();
	NrOfNeighbors = 0;

	if (!&Agent) return;

	pLastQueriedAgent = &Agent;
	LastQueryRadius = QueryRadius;

	const FVector2D myPos = Agent.GetPosition();

	FRect queryRect;
	queryRect.Min = myPos - FVector2D(QueryRadius, QueryRadius);
	queryRect.Max = myPos + FVector2D(QueryRadius, QueryRadius);

	for (const Cell& c : Cells)
	{
		if (!DoRectsOverlap(queryRect, c.BoundingBox)) continue;

		for (ASteeringAgent* other : c.Agents)
		{
			if (!other || other == &Agent) continue;

			const float dist = FVector2D::Distance(myPos, other->GetPosition());
			if (dist <= QueryRadius)
			{
				Neighbors.Add(other);
				NrOfNeighbors++;
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
	if (!pWorld) return;

	for (const Cell& c : Cells)
	{
		auto pts = c.GetRectPoints();
		for (int i = 0; i < 4; ++i)
		{
			const FVector A{ pts[i], 0.f };
			const FVector B{ pts[(i + 1) % 4], 0.f };
			DrawDebugLine(pWorld, A, B, FColor::White, false, -1.f, 0, 1.f);
		}

		const float cx = (c.BoundingBox.Min.X + c.BoundingBox.Max.X) * 0.5f;
		const float cy = (c.BoundingBox.Min.Y + c.BoundingBox.Max.Y) * 0.5f;
		FString txt = FString::Printf(TEXT("%d"), (int)c.Agents.size());
		DrawDebugString(pWorld, FVector{cx, cy, 0.f}, txt, nullptr, FColor::White, 0.f, false);
	}

	if (pLastQueriedAgent)
	{
		const FVector2D apos = pLastQueriedAgent->GetPosition();
		DrawDebugCircle(pWorld, FVector{apos, 0.f}, LastQueryRadius, 32, FColor::Yellow, false, -1.f, 0, 1.f, FVector(1,0,0), FVector(0,1,0), false);

		FRect qr;
		qr.Min = apos - FVector2D(LastQueryRadius, LastQueryRadius);
		qr.Max = apos + FVector2D(LastQueryRadius, LastQueryRadius);

		FVector2D bl = qr.Min;
		FVector2D tl = { qr.Min.X, qr.Max.Y };
		FVector2D tr = qr.Max;
		FVector2D br = { qr.Max.X, qr.Min.Y };
		DrawDebugLine(pWorld, FVector{bl,0.f}, FVector{tl,0.f}, FColor::Yellow, false, -1.f,0,1.f);
		DrawDebugLine(pWorld, FVector{tl,0.f}, FVector{tr,0.f}, FColor::Yellow, false, -1.f,0,1.f);
		DrawDebugLine(pWorld, FVector{tr,0.f}, FVector{br,0.f}, FColor::Yellow, false, -1.f,0,1.f);
		DrawDebugLine(pWorld, FVector{br,0.f}, FVector{bl,0.f}, FColor::Yellow, false, -1.f,0,1.f);

		for (ASteeringAgent* nb : Neighbors)
		{
			if (!nb) continue;
			DrawDebugLine(pWorld, FVector{apos,0.f}, FVector{nb->GetPosition(),0.f}, FColor::Blue, false, -1.f, 0, 1.f);
		}
	}
}

int CellSpace::PositionToIndex(FVector2D const & Pos) const
{

	const FVector2D local = Pos - CellOrigin;

	int col = (int)FMath::FloorToInt(local.X / CellWidth);
	int row = (int)FMath::FloorToInt(local.Y / CellHeight);


	col = FMath::Clamp(col, 0, NrOfCols - 1);
	row = FMath::Clamp(row, 0, NrOfRows - 1);

	return row * NrOfCols + col;
}

bool CellSpace::DoRectsOverlap(FRect const & RectA, FRect const & RectB)
{

	if (RectA.Max.X < RectB.Min.X || RectA.Min.X > RectB.Max.X) return false;
	if (RectA.Max.Y < RectB.Min.Y || RectA.Min.Y > RectB.Max.Y) return false;

	return true;
}