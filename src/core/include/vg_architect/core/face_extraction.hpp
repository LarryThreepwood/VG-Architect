#pragma once

#include "vg_architect/core/wall_graph.hpp"
#include "vg_architect/core/half_edge_topology.hpp"
#include <vector>

namespace vg_architect::core
{

struct FaceCycle
{
    std::vector<int> nodeIndices;
    std::vector<int> halfEdges;
    double signedArea = 0.0;
};

// Simple SignedArea for nodes
inline double CalcSignedArea(const std::vector<int>& nodes, const WallGraph& graph) {
    if (nodes.size() < 3) return 0.0;
    double area = 0.0;
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& a = graph.nodes[nodes[i]];
        const auto& b = graph.nodes[nodes[(i+1)%nodes.size()]];
        area += (a.x * b.z) - (b.x * a.z);
    }
    return area * 0.5;
}

inline std::vector<FaceCycle> ExtractFaces(const WallGraph& graph, const HalfEdgeTopology& topology)
{
    std::vector<FaceCycle> faces;
    std::vector<bool> visited(topology.halfEdges.size(), false);
    for (int i = 0; i < static_cast<int>(topology.halfEdges.size()); ++i) {
        if (visited[i]) continue;
        FaceCycle cycle;
        int curr = i;
        do {
            visited[curr] = true;
            cycle.halfEdges.push_back(curr);
            cycle.nodeIndices.push_back(topology.halfEdges[curr].originNode);
            curr = topology.halfEdges[curr].next;
        } while (curr != i);
        cycle.signedArea = CalcSignedArea(cycle.nodeIndices, graph);
        faces.push_back(std::move(cycle));
    }
    return faces;
}

} // namespace vg_architect::core
