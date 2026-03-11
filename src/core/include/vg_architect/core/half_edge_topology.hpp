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

    // 1. Create two half-edges for each graph edge
    for (int i = 0; i < static_cast<int>(graph.edges.size()); ++i)
    {
        const auto& edge = graph.edges[i];

        // A -> B
        HalfEdge heAB;
        heAB.originNode = edge.nodeA;
        heAB.twin = static_cast<int>(topology.halfEdges.size()) + 1;
        heAB.edgeIndex = i;

        // B -> A
        HalfEdge heBA;
        heBA.originNode = edge.nodeB;
        heBA.twin = static_cast<int>(topology.halfEdges.size());
        heBA.edgeIndex = i;

        topology.halfEdges.push_back(heAB);
        topology.halfEdges.push_back(heBA);
    }

    // 2. Link each node to its incident half-edges and sort them angularly
    for (int i = 0; i < static_cast<int>(graph.nodes.size()); ++i)
    {
        const auto& node = graph.nodes[i];
        std::vector<int> incidentHalfEdges;
        for (int heIdx = 0; heIdx < static_cast<int>(topology.halfEdges.size()); ++heIdx)
        {
            if (topology.halfEdges[heIdx].originNode == i)
            {
                incidentHalfEdges.push_back(heIdx);
            }
        }

        if (incidentHalfEdges.empty()) continue;

        std::sort(incidentHalfEdges.begin(), incidentHalfEdges.end(), [&](int aIdx, int bIdx) {
            const auto& heA = topology.halfEdges[aIdx];
            const auto& heB = topology.halfEdges[bIdx];
            const auto& edgeA = graph.edges[heA.edgeIndex];
            const auto& edgeB = graph.edges[heB.edgeIndex];

            int destAIdx = (heA.originNode == edgeA.nodeA) ? edgeA.nodeB : edgeA.nodeA;
            int destBIdx = (heB.originNode == edgeB.nodeA) ? edgeB.nodeB : edgeB.nodeA;

            const auto& destA = graph.nodes[destAIdx];
            const auto& destB = graph.nodes[destBIdx];

            double angleA = std::atan2(destA.z - node.z, destA.x - node.x);
            double angleB = std::atan2(destB.z - node.z, destB.x - node.x);

            if (std::abs(angleA - angleB) > 1e-9)
            {
                return angleA < angleB;
            }

            return edgeA.wallId < edgeB.wallId;
        });

        topology.nodeFirstHalfEdge[i] = incidentHalfEdges[0];

        // Link next/prev around each node for face traversal
        // next(he) is the twin of the previous edge in the angular order at the origin vertex
        // actually for face extraction (right hand rule), next(he) is:
        // go to he.dest, find its twin in its angular order...
        // wait, let's just do next/prev links for face traversal in the next step.
        // the request says: "Link next and prev relationships between half-edges around each node."
        // at each vertex, we have a list of half-edges originating from it.
        // let's link he[j].prev = he[j-1].twin, he[j].next = he[j+1].twin... wait.
        // let's follow standard DCEL: face traversal is he.next = next half-edge in CCW face.

        for (size_t j = 0; j < incidentHalfEdges.size(); ++j)
        {
            int currentHEIdx = incidentHalfEdges[j];
            int nextHEIdxInVertexOrder = incidentHalfEdges[(j + 1) % incidentHalfEdges.size()];
            int prevHEIdxInVertexOrder = incidentHalfEdges[(j + incidentHalfEdges.size() - 1) % incidentHalfEdges.size()];

            // Standard face traversal logic:
            // next(he_twin) = the next half-edge in CCW order around the destination node
            topology.halfEdges[topology.halfEdges[currentHEIdx].twin].next = nextHEIdxInVertexOrder;
            topology.halfEdges[nextHEIdxInVertexOrder].prev = topology.halfEdges[currentHEIdx].twin;
        }
    }

    return topology;
}

} // namespace vg_architect::core
