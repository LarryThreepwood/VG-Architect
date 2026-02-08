#include "vg_architect/core/project_state.hpp"

#include <iostream>

int main()
{
    const vg_architect::core::Project project = vg_architect::core::CreateInitialProject();

    std::cout << "VG Architect bootstrap initialized with " << project.floors.size() << " floor(s).\n";
    return 0;
}
