#pragma once

#include "vg_architect/core/model.hpp"
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

inline const Floor* InternalGetFloor(const Project& project, std::int32_t floorIndex)
{
    const auto it = std::find_if(project.floors.begin(), project.floors.end(), [floorIndex](const Floor& floor)
    {
        return floor.floorIndex == floorIndex;
    });

    return it == project.floors.end() ? nullptr : &(*it);
}

inline WallGraph BuildWallGraph(const Project& project, std::int32_t floorIndex)
{
    WallGraph graph;
    const Floor* floor = InternalGetFloor(project, floorIndex);
    if (!floor) return graph;

    struct PK { long long x, z; bool operator<(const PK& o) const { if(x!=o.x) return x<o.x; return z<o.z; } };
    auto toPK = [](double x, double z) {
        return PK{static_cast<long long>(std::llround(x / 0.001)), static_cast<long long>(std::llround(z / 0.001))};
    };

    std::map<PK, std::pair<double, double>> uniquePoints;
    for (const auto& w : floor->walls) {
        uniquePoints[toPK(w.startX, w.startZ)] = {w.startX, w.startZ};
        uniquePoints[toPK(w.endX, w.endZ)] = {w.endX, w.endZ};
    }

    std::map<PK, int> keyToIdx;
    int idx = 0;
    for (const auto& p : uniquePoints) {
        graph.nodes.push_back({p.second.first, p.second.second, {}});
        keyToIdx[p.first] = idx++;
    }

    for (const auto& w : floor->walls) {
        int a = keyToIdx[toPK(w.startX, w.startZ)];
        int b = keyToIdx[toPK(w.endX, w.endZ)];
        GraphEdge edge;
        if (a < b) { edge.nodeA = a; edge.nodeB = b; }
        else { edge.nodeA = b; edge.nodeB = a; }
        edge.wallId = w.id;
        graph.edges.push_back(edge);
    }

    std::sort(graph.edges.begin(), graph.edges.end(), [](const GraphEdge& a, const GraphEdge& b) {
        if (a.nodeA != b.nodeA) return a.nodeA < b.nodeA;
        if (a.nodeB != b.nodeB) return a.nodeB < b.nodeB;
        return a.wallId < b.wallId;
    });

    for (size_t i = 0; i < graph.edges.size(); ++i) {
        graph.nodes[graph.edges[i].nodeA].incidentEdges.push_back(static_cast<int>(i));
        graph.nodes[graph.edges[i].nodeB].incidentEdges.push_back(static_cast<int>(i));
    }
    for (auto& n : graph.nodes) std::sort(n.incidentEdges.begin(), n.incidentEdges.end());

    return graph;
}

} // namespace vg_architect::core
