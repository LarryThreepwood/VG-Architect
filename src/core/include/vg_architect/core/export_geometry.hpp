#pragma once

#include "vg_architect/core/model.hpp"
#include "vg_architect/core/wall_ops.hpp"

#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <sstream>

namespace vg_architect::core
{

struct ExportedPoint
{
    double x = 0.0;
    double z = 0.0;
};

struct ExportedWall
{
    std::string id;
    ExportedPoint start;
    ExportedPoint end;
    double height = 0.0;
    double thickness = 0.0;
};

struct ExportedRoom
{
    std::string id;
    std::vector<ExportedPoint> boundaryVertices;
};

struct ExportedFloor
{
    std::int32_t floorIndex = 0;
    std::vector<ExportedWall> walls;
    std::vector<ExportedRoom> rooms;
};

struct ExportedProject
{
    std::vector<ExportedFloor> floors;
};

inline ExportedProject ExportProject(const Project& project)
{
    ExportedProject exported;

    std::vector<Floor> sortedFloors = project.floors;
    std::sort(sortedFloors.begin(), sortedFloors.end(), [](const Floor& a, const Floor& b)
    {
        return a.floorIndex < b.floorIndex;
    });

    for (const Floor& floor : sortedFloors)
    {
        ExportedFloor exportedFloor;
        exportedFloor.floorIndex = floor.floorIndex;

        std::vector<Wall> sortedWalls = floor.walls;
        std::sort(sortedWalls.begin(), sortedWalls.end(), [](const Wall& a, const Wall& b)
        {
            if (a.startX != b.startX) return a.startX < b.startX;
            if (a.startZ != b.startZ) return a.startZ < b.startZ;
            if (a.endX != b.endX) return a.endX < b.endX;
            if (a.endZ != b.endZ) return a.endZ < b.endZ;
            return a.id < b.id;
        });

        std::map<std::string, const Wall*> wallMap;
        for (const Wall& wall : floor.walls)
        {
            wallMap[wall.id] = &wall;
        }

        for (const Wall& sw : sortedWalls)
        {
            ExportedWall ew;
            ew.id = sw.id;
            ew.start = {sw.startX, sw.startZ};
            ew.end = {sw.endX, sw.endZ};
            ew.height = sw.height;
            ew.thickness = sw.thickness;
            exportedFloor.walls.push_back(ew);
        }

        for (const Room& room : floor.rooms)
        {
            ExportedRoom exportedRoom;

            std::vector<std::string> sortedWallIds = room.boundaryWallIds;
            std::sort(sortedWallIds.begin(), sortedWallIds.end());

            std::stringstream ss;
            for (size_t i = 0; i < sortedWallIds.size(); ++i) {
                ss << sortedWallIds[i] << (i == sortedWallIds.size() - 1 ? "" : "_");
            }
            exportedRoom.id = "room_" + ss.str();

            std::unordered_map<detail::PointKey, std::vector<detail::PointKey>, detail::PointKeyHasher> adj;
            std::unordered_map<detail::PointKey, detail::Point, detail::PointKeyHasher> keyToOrigPoint;
            std::vector<detail::PointKey> allKeys;
            for (const std::string& wallId : room.boundaryWallIds)
            {
                auto it = wallMap.find(wallId);
                if (it != wallMap.end())
                {
                    const Wall* w = it->second;
                    detail::PointKey pkA = detail::ToPointKey(w->startX, w->startZ);
                    detail::PointKey pkB = detail::ToPointKey(w->endX, w->endZ);

                    auto addAdj = [&](const detail::PointKey& from, const detail::PointKey& to) {
                        auto& list = adj[from];
                        if (std::find(list.begin(), list.end(), to) == list.end()) list.push_back(to);
                    };
                    addAdj(pkA, pkB);
                    addAdj(pkB, pkA);

                    keyToOrigPoint[pkA] = {w->startX, w->startZ};
                    keyToOrigPoint[pkB] = {w->endX, w->endZ};

                    bool foundA = false;
                    for (const auto& k : allKeys) if (k == pkA) { foundA = true; break; }
                    if (!foundA) allKeys.push_back(pkA);

                    bool foundB = false;
                    for (const auto& k : allKeys) if (k == pkB) { foundB = true; break; }
                    if (!foundB) allKeys.push_back(pkB);
                }
            }

            if (!allKeys.empty())
            {
                detail::PointKey startKey = allKeys[0];
                for (const auto& key : allKeys)
                {
                    if (key.x < startKey.x || (key.x == startKey.x && key.z < startKey.z))
                    {
                        startKey = key;
                    }
                }

                detail::PointKey currentKey = startKey;
                auto it_adj = adj.find(currentKey);
                if (it_adj != adj.end() && it_adj->second.size() >= 2)
                {
                    const auto& neighbors = it_adj->second;
                    detail::PointKey prevKey = neighbors[0];

                    std::vector<ExportedPoint> vertices;
                    for (size_t i = 0; i < 1000; ++i)
                    {
                        const auto& orig = keyToOrigPoint[currentKey];
                        vertices.push_back({orig.x, orig.z});

                        auto it_current = adj.find(currentKey);
                        if (it_current == adj.end() || it_current->second.size() < 2) break;

                        const auto& currentNeighbors = it_current->second;
                        detail::PointKey nextKey = (currentNeighbors[0] == prevKey) ? currentNeighbors[1] : currentNeighbors[0];
                        prevKey = currentKey;
                        currentKey = nextKey;
                        if (currentKey == startKey) break;
                    }
                    exportedRoom.boundaryVertices = std::move(vertices);
                }
            }

            exportedFloor.rooms.push_back(exportedRoom);
        }

        std::sort(exportedFloor.rooms.begin(), exportedFloor.rooms.end(), [](const ExportedRoom& a, const ExportedRoom& b)
        {
            if (a.boundaryVertices.empty() || b.boundaryVertices.empty()) return a.boundaryVertices.size() > b.boundaryVertices.size();
            if (a.boundaryVertices[0].x != b.boundaryVertices[0].x) return a.boundaryVertices[0].x < b.boundaryVertices[0].x;
            if (a.boundaryVertices[0].z != b.boundaryVertices[0].z) return a.boundaryVertices[0].z < b.boundaryVertices[0].z;
            return a.id < b.id;
        });

        exported.floors.push_back(exportedFloor);
    }

    return exported;
}

} // namespace vg_architect::core
