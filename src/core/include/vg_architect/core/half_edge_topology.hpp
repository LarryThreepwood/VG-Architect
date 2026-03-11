#pragma once

#include "vg_architect/core/wall_graph.hpp"
#include <vector>
#include <algorithm>
#include <cmath>

namespace vg_architect::core
{

struct HalfEdge
{
    int originNode = -1;
    int twin = -1;
    int next = -1;
    int prev = -1;
    int edgeIndex = -1;
};

struct HalfEdgeTopology
{
    std::vector<HalfEdge> halfEdges;
    std::vector<int> nodeFirstHalfEdge;
};

inline HalfEdgeTopology BuildHalfEdgeTopology(const WallGraph& graph)
{
    HalfEdgeTopology topology;
    topology.nodeFirstHalfEdge.assign(graph.nodes.size(), -1);

    for (int i = 0; i < static_cast<int>(graph.edges.size()); ++i) {
        const auto& edge = graph.edges[i];
        HalfEdge heAB{edge.nodeA, static_cast<int>(topology.halfEdges.size()) + 1, -1, -1, i};
        HalfEdge heBA{edge.nodeB, static_cast<int>(topology.halfEdges.size()), -1, -1, i};
        topology.halfEdges.push_back(heAB);
        topology.halfEdges.push_back(heBA);
    }

    for (int i = 0; i < static_cast<int>(graph.nodes.size()); ++i) {
        std::vector<int> outgoing;
        for (int heIdx = 0; heIdx < static_cast<int>(topology.halfEdges.size()); ++heIdx) {
            if (topology.halfEdges[heIdx].originNode == i) outgoing.push_back(heIdx);
        }
        if (outgoing.empty()) continue;

        std::sort(outgoing.begin(), outgoing.end(), [&](int a, int b) {
            auto getAngle = [&](int idx) {
                const auto& he = topology.halfEdges[idx];
                const auto& edge = graph.edges[he.edgeIndex];
                int other = (he.originNode == edge.nodeA) ? edge.nodeB : edge.nodeA;
                return std::atan2(graph.nodes[other].z - graph.nodes[i].z, graph.nodes[other].x - graph.nodes[i].x);
            };
            double angA = getAngle(a), angB = getAngle(b);
            if (std::abs(angA - angB) > 1e-9) return angA < angB;
            return graph.edges[topology.halfEdges[a].edgeIndex].wallIds[0] < graph.edges[topology.halfEdges[b].edgeIndex].wallIds[0];
        });

        topology.nodeFirstHalfEdge[i] = outgoing[0];
        for (size_t j = 0; j < outgoing.size(); ++j) {
            int currOut = outgoing[j];
            int prevOut = outgoing[(j + outgoing.size() - 1) % outgoing.size()];
            int incoming = topology.halfEdges[currOut].twin;
            topology.halfEdges[incoming].next = prevOut;
            topology.halfEdges[prevOut].prev = incoming;
        }
    }
    return topology;
}

} // namespace vg_architect::core
