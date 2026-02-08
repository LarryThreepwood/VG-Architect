#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vg_architect::core
{
struct Wall
{
    std::string id;
    std::int32_t floorIndex = 0;
    double startX = 0.0;
    double startZ = 0.0;
    double endX = 0.0;
    double endZ = 0.0;
    double height = 0.0;
    double thickness = 0.0;
};

struct Room
{
    std::string id;
    std::int32_t floorIndex = 0;
    std::vector<std::string> boundaryWallIds;
};

struct Floor
{
    std::int32_t floorIndex = 0;
    std::vector<Wall> walls;
    std::vector<Room> rooms;
};

struct Project
{
    std::vector<Floor> floors;
};

} // namespace vg_architect::core
