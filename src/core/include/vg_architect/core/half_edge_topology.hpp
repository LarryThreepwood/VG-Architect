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

        // Link next/prev around each node for face traversal.
        // In a planar graph, if we are at vertex V, having arrived via half-edge H1 (pointing to V),
        // the "next" half-edge in the face traversal is the outgoing half-edge H2 (originating at V)
        // that is immediately *clockwise* from the twin of H1.

        // My incidentHalfEdges is sorted CCW (atan2 returns -PI to PI).
        // So for a given he pointing to this node, its twin is in this list.
        // The CCW "next" face edge is the *previous* he in the CCW angular order.

        for (size_t j = 0; j < incidentHalfEdges.size(); ++j)
        {
            int currentOutgoingHEIdx = incidentHalfEdges[j];
            int prevOutgoingHEIdx = incidentHalfEdges[(j + incidentHalfEdges.size() - 1) % incidentHalfEdges.size()];

            // The face traversal moves from an incoming edge to an outgoing edge.
            // If he_incoming = twin(currentOutgoingHEIdx), then the next edge in the face
            // is the outgoing edge that is immediately CCW around the vertex.
            // Wait, standard face traversal (inner) uses "left turns", which corresponds
            // to picking the next edge in CCW order at the destination vertex.

            int incomingHEIdx = topology.halfEdges[currentOutgoingHEIdx].twin;
            int nextOutgoingInFace = prevOutgoingHEIdx; // Right-hand rule (CW order around vertex)

            topology.halfEdges[incomingHEIdx].next = nextOutgoingInFace;
            topology.halfEdges[nextOutgoingInFace].prev = incomingHEIdx;
        }
    }

    return topology;
}

} // namespace vg_architect::core
