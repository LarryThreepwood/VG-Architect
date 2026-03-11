#pragma once

#include "vg_architect/core/model.hpp"
#include "vg_architect/core/wall_ops.hpp"

#include <string>
#include <vector>
#include <algorithm>
#include <map>

namespace vg_architect::core
{

struct GraphNode
{
    double x = 0.0;
    double z = 0.0;
    std::vector<int> incidentEdges;
};

struct GraphEdge
{
    int nodeA = -1;
    int nodeB = -1;
    std::string wallId;
};

struct WallGraph
{
    std::vector<GraphNode> nodes;
    std::vector<GraphEdge> edges;
};

inline WallGraph BuildWallGraph(const Project& project, std::int32_t floorIndex)
{
    WallGraph graph;

    const Floor* floor = GetFloor(project, floorIndex);
    if (!floor)
    {
        return graph;
    }

    // 1. Collect all unique vertices using ToPointKey for epsilon merging
    std::map<detail::PointKey, detail::Point, bool (*)(const detail::PointKey&, const detail::PointKey&)> uniquePoints(
        [](const detail::PointKey& a, const detail::PointKey& b) {
            if (a.x != b.x) return a.x < b.x;
            return a.z < b.z;
        }
    );

    for (const Wall& wall : floor->walls)
    {
        uniquePoints[detail::ToPointKey(wall.startX, wall.startZ)] = {wall.startX, wall.startZ};
        uniquePoints[detail::ToPointKey(wall.endX, wall.endZ)] = {wall.endX, wall.endZ};
    }

    // 2. Create nodes in deterministic order (sorted by PointKey x then z)
    std::vector<detail::PointKey> sortedKeys;
    for (const auto& pair : uniquePoints)
    {
        sortedKeys.push_back(pair.first);
    }

    std::map<detail::PointKey, int, bool (*)(const detail::PointKey&, const detail::PointKey&)> keyToNodeIndex(
        [](const detail::PointKey& a, const detail::PointKey& b) {
            if (a.x != b.x) return a.x < b.x;
            return a.z < b.z;
        }
    );

    for (size_t i = 0; i < sortedKeys.size(); ++i)
    {
        const auto& key = sortedKeys[i];
        const auto& point = uniquePoints[key];
        GraphNode node;
        node.x = point.x;
        node.z = point.z;
        graph.nodes.push_back(node);
        keyToNodeIndex[key] = static_cast<int>(i);
    }

    // 3. Create edges
    for (const Wall& wall : floor->walls)
    {
        int idxA = keyToNodeIndex[detail::ToPointKey(wall.startX, wall.startZ)];
        int idxB = keyToNodeIndex[detail::ToPointKey(wall.endX, wall.endZ)];

        GraphEdge edge;
        // Ensure canonical node ordering within edge for stability
        if (idxA < idxB)
        {
            edge.nodeA = idxA;
            edge.nodeB = idxB;
        }
        else
        {
            edge.nodeA = idxB;
            edge.nodeB = idxA;
        }
        edge.wallId = wall.id;
        graph.edges.push_back(edge);
    }

    // 4. Sort edges deterministically
    std::sort(graph.edges.begin(), graph.edges.end(), [](const GraphEdge& a, const GraphEdge& b) {
        if (a.nodeA != b.nodeA) return a.nodeA < b.nodeA;
        if (a.nodeB != b.nodeB) return a.nodeB < b.nodeB;
        return a.wallId < b.wallId;
    });

    // 5. Populate and sort incidentEdges for each node
    for (size_t i = 0; i < graph.edges.size(); ++i)
    {
        const auto& edge = graph.edges[i];
        graph.nodes[edge.nodeA].incidentEdges.push_back(static_cast<int>(i));
        graph.nodes[edge.nodeB].incidentEdges.push_back(static_cast<int>(i));
    }

    for (auto& node : graph.nodes)
    {
        std::sort(node.incidentEdges.begin(), node.incidentEdges.end());
    }

    return graph;
}

} // namespace vg_architect::core
