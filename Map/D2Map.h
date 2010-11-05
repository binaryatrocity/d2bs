#pragma once

#include "Map.h"

#include <list>

#include "D2Structs.h"
#include "D2Ptrs.h"

namespace Mapping
{

enum ExitType
{
	Linkage,
	Tile
};

struct Exit
{
public:
	DWORD TargetLevel;
	Point Location;
	ExitType Type;
	DWORD TileId;

	Exit(Point location, DWORD levelId, ExitType type, DWORD tileId) :
		TargetLevel(levelId), Location(location), Type(type), TileId(tileId) {}
};

typedef std::vector<Exit> ExitArray;

class D2Map : public Map
{
private:
	enum CollisionFlag {
		None				= 0x0000,
		BlockWalk			= 0x0001,
		BlockLineOfSight	= 0x0002,
		Wall				= 0x0004,
		BlockPlayer			= 0x0008,
		AlternateTile		= 0x0010,
		Blank				= 0x0020,
		Missile				= 0x0040,
		Player				= 0x0080,
		NPCLocation			= 0x0100,
		Item				= 0x0200,
		Object				= 0x0400,
		ClosedDoor			= 0x0800,
		NPCCollision		= 0x1000,
		FriendlyNPC			= 0x2000,
		Unknown				= 0x4000,
		DeadBody			= 0x8000, // also portal
		ThickenedWall		= 0xFEFE,
		Avoid				= 0xFFFF
	};

	Act* act;
	const Level* level;
	int width, height;
	int posX, posY;
	Matrix<CollisionFlag>* mapPoints;
	CRITICAL_SECTION* lock;

	typedef std::list<Room2*> RoomList;

	void Build(void);
	inline void AddRoomData(Room2* room) const { D2COMMON_AddRoomData(act, level->dwLevelNo, room->dwPosX, room->dwPosY, room->pRoom1); }
	inline void RemoveRoomData(Room2* room) const { D2COMMON_RemoveRoomData(act, level->dwLevelNo, room->dwPosX, room->dwPosY, room->pRoom1); }

	void AddRoom(Room2* const room, RoomList& rooms, UnitAny* player);
	void AddCollisionMap(const CollMap* const map);
	void SetCollisionData(int x, int y, WORD value);
	bool IsGap(int x, int y, bool abs) const;
	void FillGaps(void);
	void ShrinkMap(void);
	void ThickenWalls(void);

public:
	D2Map(const Level* level);
	~D2Map(void);

	void Dump(const char* file, const PointList& points) const;

	Point AbsToRelative(const Point& point) const;
	Point RelativeToAbs(const Point& point) const;

	inline int GetWidth() const { return width; }
	inline int GetHeight() const { return height; }

	int GetMapData(const Point& point, bool abs = true) const;
	bool IsValidPoint(const Point& point, bool abs = true) const;

	void GetExits(ExitArray& exits) const;

	bool SpaceHasFlag(int flag, const Point& point, bool abs = true) const;
	bool PathHasFlag(int flag, const PointList& points, bool abs = true) const;

	bool SpaceIsWalkable(const Point& point, bool abs = true) const;
	bool SpaceHasLineOfSight(const Point& point, bool abs = true) const;
	bool SpaceIsInvalid(const Point& point, bool abs = true) const;

	bool PathIsWalkable(const PointList& points, bool abs = true) const;
	bool PathHasLineOfSight(const PointList& points, bool abs = true) const;
};

}
