#include "vg_architect/core/project_state.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace vg_architect::core
{
namespace
{
constexpr double kAxisEpsilon = 0.001;
constexpr double kLengthEpsilon = 0.001;
constexpr double kOverlapEpsilon = 0.001;

struct PointKey
{
    long long x = 0;
    long long y = 0;

    bool operator==(const PointKey& other) const
    {
        return x == other.x && y == other.y;
    }
};

struct PointKeyHasher
{
    std::size_t operator()(const PointKey& value) const
    {
        return static_cast<std::size_t>((value.x * 73856093LL) ^ (value.y * 19349663LL));
    }
};

PointKey ToPointKey(const Vec2& point)
{
    return PointKey{
        .x = static_cast<long long>(std::llround(point.x / kAxisEpsilon)),
        .y = static_cast<long long>(std::llround(point.y / kAxisEpsilon)),
    };
}

bool AlmostEqual(double a, double b, double epsilon)
{
    return std::abs(a - b) <= epsilon;
}

bool IsAxisAligned(const Wall& wall)
{
    const bool alignedX = AlmostEqual(wall.start.y, wall.end.y, kAxisEpsilon);
    const bool alignedY = AlmostEqual(wall.start.x, wall.end.x, kAxisEpsilon);
    return alignedX || alignedY;
}

bool HasNonZeroLength(const Wall& wall)
{
    const double dx = wall.end.x - wall.start.x;
    const double dy = wall.end.y - wall.start.y;
    return std::sqrt(dx * dx + dy * dy) > kLengthEpsilon;
}

bool WallsOverlapCollinear(const Wall& a, const Wall& b)
{
    const bool aHorizontal = AlmostEqual(a.start.y, a.end.y, kAxisEpsilon);
    const bool bHorizontal = AlmostEqual(b.start.y, b.end.y, kAxisEpsilon);

    if (aHorizontal != bHorizontal)
    {
        return false;
    }

    if (aHorizontal)
    {
        if (!AlmostEqual(a.start.y, b.start.y, kAxisEpsilon))
        {
            return false;
        }

        const double aMin = std::min(a.start.x, a.end.x);
        const double aMax = std::max(a.start.x, a.end.x);
        const double bMin = std::min(b.start.x, b.end.x);
        const double bMax = std::max(b.start.x, b.end.x);
        const double overlapMin = std::max(aMin, bMin);
        const double overlapMax = std::min(aMax, bMax);
        return (overlapMax - overlapMin) > kOverlapEpsilon;
    }

    if (!AlmostEqual(a.start.x, b.start.x, kAxisEpsilon))
    {
        return false;
    }

    const double aMin = std::min(a.start.y, a.end.y);
    const double aMax = std::max(a.start.y, a.end.y);
    const double bMin = std::min(b.start.y, b.end.y);
    const double bMax = std::max(b.start.y, b.end.y);
    const double overlapMin = std::max(aMin, bMin);
    const double overlapMax = std::min(aMax, bMax);
    return (overlapMax - overlapMin) > kOverlapEpsilon;
}

bool PointInPolygon(const Vec2& point, const std::vector<Vec2>& polygon)
{
    if (polygon.size() < 3)
    {
        return false;
    }

    bool inside = false;
    for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
    {
        const Vec2& pi = polygon[i];
        const Vec2& pj = polygon[j];

        const bool intersects = ((pi.y > point.y) != (pj.y > point.y)) &&
                                (point.x < (pj.x - pi.x) * (point.y - pi.y) / ((pj.y - pi.y) + std::numeric_limits<double>::epsilon()) + pi.x);
        if (intersects)
        {
            inside = !inside;
        }
    }

    return inside;
}

double SignedArea(const std::vector<Vec2>& polygon)
{
    double area = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i)
    {
        const Vec2& a = polygon[i];
        const Vec2& b = polygon[(i + 1) % polygon.size()];
        area += (a.x * b.y) - (b.x * a.y);
    }
    return area * 0.5;
}

Vec2 PolygonCentroid(const std::vector<Vec2>& polygon)
{
    const double area = SignedArea(polygon);
    if (std::abs(area) <= std::numeric_limits<double>::epsilon())
    {
        return polygon.empty() ? Vec2{} : polygon.front();
    }

    double cx = 0.0;
    double cy = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i)
    {
        const Vec2& a = polygon[i];
        const Vec2& b = polygon[(i + 1) % polygon.size()];
        const double factor = (a.x * b.y) - (b.x * a.y);
        cx += (a.x + b.x) * factor;
        cy += (a.y + b.y) * factor;
    }

    const double denom = 6.0 * area;
    return Vec2{.x = cx / denom, .y = cy / denom};
}

} // namespace

Project CreateInitialProject()
{
    Project project;
    project.floors.push_back(Floor{.floorIndex = 0});
    return project;
}

const Floor* GetFloor(const Project& project, std::int32_t floorIndex)
{
    const auto it = std::find_if(
        project.floors.begin(),
        project.floors.end(),
        [floorIndex](const Floor& floor)
        {
            return floor.floorIndex == floorIndex;
        });

    return it == project.floors.end() ? nullptr : &(*it);
}

bool AddWall(Project& project, const Wall& wall, std::string& outError)
{
    outError.clear();

    if (wall.id.empty())
    {
        outError = "Wall id cannot be empty.";
        return false;
    }

    if (!IsAxisAligned(wall))
    {
        outError = "Wall must be axis-aligned.";
        return false;
    }

    if (!HasNonZeroLength(wall))
    {
        outError = "Wall must have non-zero length.";
        return false;
    }

    for (const Floor& floor : project.floors)
    {
        for (const Wall& existingWall : floor.walls)
        {
            if (existingWall.id == wall.id)
            {
                outError = "Wall id already exists.";
                return false;
            }
        }
    }

    auto floorIt = std::find_if(
        project.floors.begin(),
        project.floors.end(),
        [&wall](const Floor& floor)
        {
            return floor.floorIndex == wall.floorIndex;
        });

    if (floorIt == project.floors.end())
    {
        project.floors.push_back(Floor{.floorIndex = wall.floorIndex});
        floorIt = std::prev(project.floors.end());
    }

    for (const Wall& existingWall : floorIt->walls)
    {
        if (WallsOverlapCollinear(existingWall, wall))
        {
            outError = "Wall overlaps an existing wall on the same floor.";
            return false;
        }
    }

    floorIt->walls.push_back(wall);
    return true;
}

bool RemoveWall(Project& project, const std::string& wallId, std::string& outError)
{
    outError.clear();

    for (Floor& floor : project.floors)
    {
        const auto it = std::find_if(
            floor.walls.begin(),
            floor.walls.end(),
            [&wallId](const Wall& wall)
            {
                return wall.id == wallId;
            });

        if (it != floor.walls.end())
        {
            floor.walls.erase(it);
            return true;
        }
    }

    outError = "Wall id not found.";
    return false;
}

void RecomputeRooms(Project& project, std::int32_t floorIndex)
{
    auto floorIt = std::find_if(
        project.floors.begin(),
        project.floors.end(),
        [floorIndex](const Floor& floor)
        {
            return floor.floorIndex == floorIndex;
        });

    if (floorIt == project.floors.end())
    {
        return;
    }

    Floor& floor = *floorIt;
    floor.rooms.clear();

    struct LoopData
    {
        std::vector<Vec2> vertices;
        std::vector<std::string> wallIds;
        Vec2 samplePoint;
        double absArea = 0.0;
        int depth = 0;
        bool isInterior = false;
        int parent = -1;
    };

    std::unordered_map<PointKey, std::vector<std::pair<PointKey, std::size_t>>, PointKeyHasher> adjacency;
    std::unordered_map<PointKey, Vec2, PointKeyHasher> keyToPoint;

    for (std::size_t wallIndex = 0; wallIndex < floor.walls.size(); ++wallIndex)
    {
        const Wall& wall = floor.walls[wallIndex];
        const PointKey a = ToPointKey(wall.start);
        const PointKey b = ToPointKey(wall.end);
        adjacency[a].push_back({b, wallIndex});
        adjacency[b].push_back({a, wallIndex});
        keyToPoint[a] = wall.start;
        keyToPoint[b] = wall.end;
    }

    std::vector<bool> usedWall(floor.walls.size(), false);
    std::vector<LoopData> loops;

    for (std::size_t startWallIndex = 0; startWallIndex < floor.walls.size(); ++startWallIndex)
    {
        if (usedWall[startWallIndex])
        {
            continue;
        }

        const Wall& startWall = floor.walls[startWallIndex];
        PointKey current = ToPointKey(startWall.start);
        PointKey next = ToPointKey(startWall.end);
        PointKey start = current;

        std::vector<Vec2> polygon;
        std::vector<std::string> wallIds;
        std::unordered_set<std::size_t> localUsed;

        polygon.push_back(keyToPoint[current]);

        bool validLoop = true;
        while (true)
        {
            std::size_t edgeIndex = static_cast<std::size_t>(-1);
            bool foundCurrentEdge = false;
            for (const auto& candidate : adjacency[current])
            {
                if (candidate.first == next)
                {
                    edgeIndex = candidate.second;
                    foundCurrentEdge = true;
                    break;
                }
            }

            if (!foundCurrentEdge || localUsed.contains(edgeIndex))
            {
                validLoop = false;
                break;
            }

            localUsed.insert(edgeIndex);
            wallIds.push_back(floor.walls[edgeIndex].id);

            current = next;
            polygon.push_back(keyToPoint[current]);

            if (current == start)
            {
                break;
            }

            const auto& neighbors = adjacency[current];
            if (neighbors.size() != 2)
            {
                validLoop = false;
                break;
            }

            const PointKey n0 = neighbors[0].first;
            const PointKey n1 = neighbors[1].first;
            if (n0 == next)
            {
                next = n1;
            }
            else if (n1 == next)
            {
                next = n0;
            }
            else
            {
                validLoop = false;
                break;
            }
        }

        if (!validLoop || polygon.size() < 4)
        {
            continue;
        }

        for (std::size_t idx : localUsed)
        {
            usedWall[idx] = true;
        }

        polygon.pop_back();

        const double area = SignedArea(polygon);
        if (std::abs(area) <= kLengthEpsilon)
        {
            continue;
        }

        if (area < 0.0)
        {
            std::reverse(polygon.begin(), polygon.end());
            std::reverse(wallIds.begin(), wallIds.end());
        }

        loops.push_back(LoopData{
            .vertices = std::move(polygon),
            .wallIds = std::move(wallIds),
            .samplePoint = {},
            .absArea = std::abs(area),
            .depth = 0,
            .isInterior = false,
            .parent = -1,
        });

        loops.back().samplePoint = PolygonCentroid(loops.back().vertices);
    }

    for (std::size_t i = 0; i < loops.size(); ++i)
    {
        int containerCount = 0;
        double parentArea = std::numeric_limits<double>::max();
        int parentIndex = -1;

        for (std::size_t j = 0; j < loops.size(); ++j)
        {
            if (i == j)
            {
                continue;
            }

            if (PointInPolygon(loops[i].samplePoint, loops[j].vertices))
            {
                ++containerCount;
                if (loops[j].absArea < parentArea)
                {
                    parentArea = loops[j].absArea;
                    parentIndex = static_cast<int>(j);
                }
            }
        }

        loops[i].depth = containerCount + 1;
        loops[i].isInterior = (loops[i].depth % 2) == 1;
        loops[i].parent = parentIndex;
    }

    int roomCounter = 0;
    for (std::size_t i = 0; i < loops.size(); ++i)
    {
        if (!loops[i].isInterior)
        {
            continue;
        }

        Room room;
        room.id = "room_" + std::to_string(floorIndex) + "_" + std::to_string(roomCounter++);
        room.floorIndex = floorIndex;
        room.outerBoundary = loops[i].vertices;
        room.boundaryWallIds = loops[i].wallIds;

        for (std::size_t j = 0; j < loops.size(); ++j)
        {
            if (loops[j].isInterior)
            {
                continue;
            }

            if (loops[j].parent == static_cast<int>(i))
            {
                std::vector<Vec2> hole = loops[j].vertices;
                if (SignedArea(hole) > 0.0)
                {
                    std::reverse(hole.begin(), hole.end());
                }
                room.holes.push_back(std::move(hole));
            }
        }

        floor.rooms.push_back(std::move(room));
    }
}

} // namespace vg_architect::core
