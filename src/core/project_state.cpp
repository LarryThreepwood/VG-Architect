#include "vg_architect/core/project_state.hpp"

namespace vg_architect::core
{
Project CreateInitialProject()
{
    Project project;
    project.floors.push_back(Floor{.floorIndex = 0});
    return project;
}

} // namespace vg_architect::core
