#pragma once

#include "vg_architect/core/wall_graph.hpp"
#include "vg_architect/core/half_edge_topology.hpp"
#include "vg_architect/core/face_extraction.hpp"

#include <string>
#include <vector>
#include <algorithm>

namespace vg_architect::core
{

struct DetectedRoom
{
    std::string id;
    std::vector<std::string> wallIds;
    std::vector<int> nodeIndices;
    double area = 0.0;
};

inline std::vector<DetectedRoom> DetectRooms(const WallGraph& graph, const HalfEdgeTopology& topology)
{
    std::vector<FaceCycle> cycles = ExtractFaces(graph, topology);
    std::vector<DetectedRoom> validRooms;
    for (const auto& cycle : cycles) {
        if (cycle.signedArea > 1e-6 && cycle.nodeIndices.size() >= 3) {
            DetectedRoom room;
            room.nodeIndices = cycle.nodeIndices;
            room.area = cycle.signedArea;
            for (int heIdx : cycle.halfEdges) {
                const auto& edge = graph.edges[topology.halfEdges[heIdx].edgeIndex];
                for (const auto& wid : edge.wallIds) {
                    room.wallIds.push_back(wid);
                }
            }
            std::sort(room.wallIds.begin(), room.wallIds.end());
            room.wallIds.erase(std::unique(room.wallIds.begin(), room.wallIds.end()), room.wallIds.end());
            validRooms.push_back(std::move(room));
        }
    }

    std::sort(validRooms.begin(), validRooms.end(), [&](const DetectedRoom& a, const DetectedRoom& b) {
        auto getCentroid = [&](const std::vector<int>& nodes) {
            double cx = 0, cz = 0;
            for (int n : nodes) { cx += graph.nodes[n].x; cz += graph.nodes[n].z; }
            return std::pair<double, double>{cx / nodes.size(), cz / nodes.size()};
        };
        auto ca = getCentroid(a.nodeIndices), cb = getCentroid(b.nodeIndices);
        if (std::abs(ca.second - cb.second) > 1e-6) return ca.second < cb.second;
        return ca.first < cb.first;
    });

    for (size_t i = 0; i < validRooms.size(); ++i) validRooms[i].id = "room_" + std::to_string(i);
    return validRooms;
}

} // namespace vg_architect::core
