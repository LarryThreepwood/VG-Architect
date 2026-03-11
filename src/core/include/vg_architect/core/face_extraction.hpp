#pragma once

#include "vg_architect/core/wall_graph.hpp"
#include "vg_architect/core/half_edge_topology.hpp"
#include "vg_architect/core/wall_ops.hpp"

#include <vector>

namespace vg_architect::core
{

struct FaceCycle
{
    std::vector<detail::Point> vertices;
    std::vector<int> halfEdges;
    double signedArea = 0.0;
};

inline std::vector<FaceCycle> ExtractFaces(const WallGraph& graph, const HalfEdgeTopology& topology)
{
    std::vector<FaceCycle> faces;
    std::vector<bool> visited(topology.halfEdges.size(), false);

    for (int i = 0; i < static_cast<int>(topology.halfEdges.size()); ++i)
    {
        if (visited[i]) continue;

        FaceCycle cycle;
        int curr = i;
        do
        {
            visited[curr] = true;
            cycle.halfEdges.push_back(curr);

            int originIdx = topology.halfEdges[curr].originNode;
            cycle.vertices.push_back({graph.nodes[originIdx].x, graph.nodes[originIdx].z});

            curr = topology.halfEdges[curr].next;
        } while (curr != i);

        cycle.signedArea = detail::SignedArea(cycle.vertices);
        if (std::abs(cycle.signedArea) < 1e-9 && cycle.halfEdges.size() > 2) {
             // degenerate cycle, but let's keep it for now or debug
        }
        faces.push_back(std::move(cycle));
    }

    return faces;
}

} // namespace vg_architect::core
