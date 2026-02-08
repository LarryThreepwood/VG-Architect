#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace vg_architect::core
{
struct Vec2
{
    double x = 0.0;
    double y = 0.0;
};

struct Wall
{
    std::string id;
    std::int32_t floorIndex = 0;
    Vec2 start;
    Vec2 end;
    double height = 3.0;
    double thickness = 0.2;
};

struct Room
{
    std::string id;
    std::int32_t floorIndex = 0;
    std::vector<Vec2> outerBoundary;
    std::vector<std::vector<Vec2>> holes;
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
