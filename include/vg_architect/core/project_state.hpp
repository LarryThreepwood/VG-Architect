#pragma once

#include "vg_architect/core/types.hpp"

#include <cstdint>
#include <string>

namespace vg_architect::core
{
Project CreateInitialProject();

bool AddWall(Project& project, const Wall& wall, std::string& outError);
bool RemoveWall(Project& project, const std::string& wallId, std::string& outError);
void RecomputeRooms(Project& project, std::int32_t floorIndex);
const Floor* GetFloor(const Project& project, std::int32_t floorIndex);

} // namespace vg_architect::core
