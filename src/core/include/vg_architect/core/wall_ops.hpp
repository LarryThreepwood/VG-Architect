#pragma once

#include "vg_architect/core/model.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "vg_architect/core/wall_graph.hpp"
#include "vg_architect/core/half_edge_topology.hpp"
#include "vg_architect/core/room_detection.hpp"

namespace vg_architect::core
{
namespace detail
{
constexpr double kEpsilon = 0.001;
constexpr double kWallHeightCm = 300.0;
constexpr double kWallThicknessCm = 20.0;

struct Point
{
    double x = 0.0;
    double z = 0.0;
};

struct PointKey
{
    long long x = 0;
    long long z = 0;

    bool operator==(const PointKey& other) const
    {
        return x == other.x && z == other.z;
    }
};

struct PointKeyHasher
{
    std::size_t operator()(const PointKey& value) const
    {
        return static_cast<std::size_t>((value.x * 73856093LL) ^ (value.z * 19349663LL));
    }
};

inline bool AlmostEqual(double a, double b)
{
    return std::abs(a - b) <= kEpsilon;
}

inline PointKey ToPointKey(double x, double z)
{
    return PointKey{
        static_cast<long long>(std::llround(x / kEpsilon)),
        static_cast<long long>(std::llround(z / kEpsilon)),
    };
}

inline std::pair<PointKey, PointKey> CanonicalWallEndpoints(const Wall& wall)
{
    const PointKey a = ToPointKey(wall.startX, wall.startZ);
    const PointKey b = ToPointKey(wall.endX, wall.endZ);

    if (a.x < b.x)
    {
        return {a, b};
    }

    if (a.x > b.x)
    {
        return {b, a};
    }

    if (a.z <= b.z)
    {
        return {a, b};
    }

    return {b, a};
}

inline double SignedArea(const std::vector<Point>& polygon)
{
    if (polygon.size() < 3)
    {
        return 0.0;
    }

    double area = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i)
    {
        const Point& a = polygon[i];
        const Point& b = polygon[(i + 1) % polygon.size()];
        area += (a.x * b.z) - (b.x * a.z);
    }

    return area * 0.5;
}

inline Point PolygonCentroid(const std::vector<Point>& polygon)
{
    const double area = SignedArea(polygon);
    if (std::abs(area) <= std::numeric_limits<double>::epsilon())
    {
        return polygon.empty() ? Point{} : polygon.front();
    }

    double cx = 0.0;
    double cz = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i)
    {
        const Point& a = polygon[i];
        const Point& b = polygon[(i + 1) % polygon.size()];
        const double factor = (a.x * b.z) - (b.x * a.z);
        cx += (a.x + b.x) * factor;
        cz += (a.z + b.z) * factor;
    }

    return Point{cx / (6.0 * area), cz / (6.0 * area)};
}

inline bool PointInPolygon(const Point& point, const std::vector<Point>& polygon)
{
    if (polygon.size() < 3)
    {
        return false;
    }

    bool inside = false;
    for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
    {
        const Point& pi = polygon[i];
        const Point& pj = polygon[j];
        const bool intersects = ((pi.z > point.z) != (pj.z > point.z)) &&
                                (point.x < (pj.x - pi.x) * (point.z - pi.z) / ((pj.z - pi.z) + std::numeric_limits<double>::epsilon()) + pi.x);
        if (intersects)
        {
            inside = !inside;
        }
    }
    return inside;
}

inline bool WallsOverlapCollinear(const Wall& a, const Wall& b)
{
    const bool aAlongX = AlmostEqual(a.startZ, a.endZ);
    const bool bAlongX = AlmostEqual(b.startZ, b.endZ);
    const bool aAlongZ = AlmostEqual(a.startX, a.endX);
    const bool bAlongZ = AlmostEqual(b.startX, b.endX);

    if (aAlongX && bAlongX)
    {
        if (!AlmostEqual(a.startZ, b.startZ))
        {
            return false;
        }

        const double aMin = std::min(a.startX, a.endX);
        const double aMax = std::max(a.startX, a.endX);
        const double bMin = std::min(b.startX, b.endX);
        const double bMax = std::max(b.startX, b.endX);
        const double overlapMin = std::max(aMin, bMin);
        const double overlapMax = std::min(aMax, bMax);
        return (overlapMax - overlapMin) > kEpsilon;
    }

    if (aAlongZ && bAlongZ)
    {
        if (!AlmostEqual(a.startX, b.startX))
        {
            return false;
        }

        const double aMin = std::min(a.startZ, a.endZ);
        const double aMax = std::max(a.startZ, a.endZ);
        const double bMin = std::min(b.startZ, b.endZ);
        const double bMax = std::max(b.startZ, b.endZ);
        const double overlapMin = std::max(aMin, bMin);
        const double overlapMax = std::min(aMax, bMax);
        return (overlapMax - overlapMin) > kEpsilon;
    }

    return false;
}

} // namespace detail

inline const Floor* GetFloor(const Project& project, std::int32_t floorIndex)
{
    const auto it = std::find_if(project.floors.begin(), project.floors.end(), [floorIndex](const Floor& floor)
    {
        return floor.floorIndex == floorIndex;
    });

    return it == project.floors.end() ? nullptr : &(*it);
}

inline bool AddWall(Project& project, const Wall& wall, std::string& outError)
{
    outError.clear();

    auto floorIt = std::find_if(project.floors.begin(), project.floors.end(), [&wall](const Floor& floor)
    {
        return floor.floorIndex == wall.floorIndex;
    });

    if (floorIt == project.floors.end())
    {
        outError = "Floor index not found.";
        return false;
    }

    if (!detail::AlmostEqual(wall.height, detail::kWallHeightCm))
    {
        outError = "Wall height must be 300.0 cm.";
        return false;
    }

    if (!detail::AlmostEqual(wall.thickness, detail::kWallThicknessCm))
    {
        outError = "Wall thickness must be 20.0 cm.";
        return false;
    }

    const double dx = wall.endX - wall.startX;
    const double dz = wall.endZ - wall.startZ;
    const bool alongX = std::abs(dz) <= detail::kEpsilon;
    const bool alongZ = std::abs(dx) <= detail::kEpsilon;

    if (!(alongX || alongZ))
    {
        outError = "Wall must be axis-aligned on X or Z.";
        return false;
    }

    const double length = std::sqrt((dx * dx) + (dz * dz));
    if (length <= detail::kEpsilon)
    {
        outError = "Wall length must be non-zero.";
        return false;
    }

    for (const Wall& existingWall : floorIt->walls)
    {
        if (detail::WallsOverlapCollinear(existingWall, wall))
        {
            outError = "Wall overlaps an existing wall on the same floor.";
            return false;
        }
    }

    floorIt->walls.push_back(wall);
    return true;
}

inline bool RemoveWall(Project& project, const std::string& wallId, std::string& outError)
{
    outError.clear();

    for (Floor& floor : project.floors)
    {
        const auto it = std::find_if(floor.walls.begin(), floor.walls.end(), [&wallId](const Wall& wall)
        {
            return wall.id == wallId;
        });

        if (it != floor.walls.end())
        {
            floor.walls.erase(it);
            floor.rooms.clear();
            return true;
        }
    }

    outError = "Wall id not found.";
    return false;
}

inline void RecomputeRooms(Project& project, std::int32_t floorIndex)
{
    auto floorIt = std::find_if(project.floors.begin(), project.floors.end(), [floorIndex](const Floor& floor)
    {
        return floor.floorIndex == floorIndex;
    });

    if (floorIt == project.floors.end())
    {
        return;
    }

    Floor& floor = *floorIt;
    floor.rooms.clear();

    const auto graph = BuildWallGraph(project, floorIndex);
    const auto topology = BuildHalfEdgeTopology(graph);
    const auto detectedRooms = DetectRooms(graph, topology);

    for (const auto& dr : detectedRooms)
    {
        Room room;
        room.id = dr.id;
        room.floorIndex = floorIndex;
        room.boundaryWallIds = dr.wallIds;
        floor.rooms.push_back(std::move(room));
    }
}

} // namespace vg_architect::core
