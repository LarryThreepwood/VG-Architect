#include "catch.hpp"
#include "vg_architect/core/model.hpp"
#include "vg_architect/core/wall_ops.hpp"
#include "vg_architect/core/wall_graph.hpp"
#include "vg_architect/core/half_edge_topology.hpp"
#include "vg_architect/core/room_detection.hpp"

#include <cmath>

using namespace vg_architect::core;

// Helper to manually check invariants since we are testing edge cases
void CheckInvariants(const HalfEdgeTopology& topology) {
    for (int i = 0; i < (int)topology.halfEdges.size(); ++i) {
        const auto& he = topology.halfEdges[i];
        if (he.twin != -1) {
            REQUIRE(topology.halfEdges[he.twin].twin == i);
        }
        if (he.next != -1) {
            REQUIRE(topology.halfEdges[he.next].prev == i);
        }
        if (he.prev != -1) {
            REQUIRE(topology.halfEdges[he.prev].next == i);
        }
    }
}

TEST_CASE("Geometry Robustness Edge Cases", "[geometry][robustness]")
{
    Project project;
    project.floors.push_back({0, {}, {}});
    std::string err;

    SECTION("1. Nearly coincident vertices")
    {
        // Epsilon is 0.001. Points differing by 0.0001 should merge.
        // Create a square where one corner is slightly off.
        project.floors[0].walls.push_back({"w1", 0, 0.0, 0.0, 100.0, 0.0, 300, 20});
        project.floors[0].walls.push_back({"w2", 0, 100.0, 0.0, 100.0, 100.0, 300, 20});
        project.floors[0].walls.push_back({"w3", 0, 100.0, 100.0, 0.0001, 100.0, 300, 20});
        project.floors[0].walls.push_back({"w4", 0, 0.0, 100.0, 0.0, 0.0, 300, 20});

        RecomputeRooms(project, 0);
        WallGraph g = BuildWallGraph(project, 0);
        HalfEdgeTopology t = BuildHalfEdgeTopology(g);

        CheckInvariants(t);
        CHECK(project.floors[0].rooms.size() == 1);
        CHECK(g.nodes.size() == 4); // Should have merged (0,100) and (0.0001,100)
    }

    SECTION("2. Very small rooms")
    {
        // Enclosure much larger than epsilon but still 'small'
        double size = 0.1;
        project.floors[0].walls.push_back({"w1", 0, 0.0, 0.0, size, 0.0, 300, 20});
        project.floors[0].walls.push_back({"w2", 0, size, 0.0, size, size, 300, 20});
        project.floors[0].walls.push_back({"w3", 0, size, size, 0.0, size, 300, 20});
        project.floors[0].walls.push_back({"w4", 0, 0.0, size, 0.0, 0.0, 300, 20});

        RecomputeRooms(project, 0);
        CHECK(project.floors[0].rooms.size() == 1);
        CHECK(project.floors[0].rooms[0].boundaryWallIds.size() == 4);
    }

    SECTION("3. Collinear multi-segment walls")
    {
        // A single line (0,0)->(200,0) split into two segments
        // Square with one side split
        project.floors[0].walls.push_back({"w1a", 0, 0.0, 0.0, 50.0, 0.0, 300, 20});
        project.floors[0].walls.push_back({"w1b", 0, 50.0, 0.0, 100.0, 0.0, 300, 20});
        project.floors[0].walls.push_back({"w2", 0, 100.0, 0.0, 100.0, 100.0, 300, 20});
        project.floors[0].walls.push_back({"w3", 0, 100.0, 100.0, 0.0, 100.0, 300, 20});
        project.floors[0].walls.push_back({"w4", 0, 0.0, 100.0, 0.0, 0.0, 300, 20});

        RecomputeRooms(project, 0);
        CHECK(project.floors[0].rooms.size() == 1);
        CHECK(project.floors[0].rooms[0].boundaryWallIds.size() == 5);
    }

    SECTION("4. Duplicate wall segments")
    {
        // Injecting duplicates directly to see how graph handles them
        project.floors[0].walls.push_back({"w1", 0, 0.0, 0.0, 100.0, 0.0, 300, 20});
        project.floors[0].walls.push_back({"w1_dup", 0, 0.0, 0.0, 100.0, 0.0, 300, 20});
        project.floors[0].walls.push_back({"w2", 0, 100.0, 0.0, 100.0, 100.0, 300, 20});
        project.floors[0].walls.push_back({"w3", 0, 100.0, 100.0, 0.0, 100.0, 300, 20});
        project.floors[0].walls.push_back({"w4", 0, 0.0, 100.0, 0.0, 0.0, 300, 20});

        RecomputeRooms(project, 0);
        WallGraph g = BuildWallGraph(project, 0);
        HalfEdgeTopology t = BuildHalfEdgeTopology(g);
        CheckInvariants(t);

        // DEBUG
        // std::vector<FaceCycle> faces = ExtractFaces(g, t);
        // for(auto& f : faces) { printf("Face area: %f, edges: %zu\n", f.signedArea, f.halfEdges.size()); }

        // If duplicate walls exist, they form a zero-area face between them.
        // w1 and w1_dup form a loop: (0,0) -> (100,0) [w1] -> (0,0) [w1_dup]
        // This 'pinch' might be taking the half-edges that w4 and w2 need to form the large square.
        CHECK(project.floors[0].rooms.size() == 1);
    }

    SECTION("5. Reverse-order walls")
    {
        project.floors[0].walls.push_back({"w1", 0, 0.0, 0.0, 100.0, 0.0, 300, 20});
        project.floors[0].walls.push_back({"w1_rev", 0, 100.0, 0.0, 0.0, 0.0, 300, 20});
        project.floors[0].walls.push_back({"w2", 0, 100.0, 0.0, 100.0, 100.0, 300, 20});
        project.floors[0].walls.push_back({"w3", 0, 100.0, 100.0, 0.0, 100.0, 300, 20});
        project.floors[0].walls.push_back({"w4", 0, 0.0, 100.0, 0.0, 0.0, 300, 20});

        RecomputeRooms(project, 0);
        CHECK(project.floors[0].rooms.size() == 1);
    }

    SECTION("6. Floating-point noise")
    {
        project.floors[0].walls.push_back({"w1", 0, 0.00000000001, 0.0, 99.9999999999, 0.0, 300, 20});
        project.floors[0].walls.push_back({"w2", 0, 100.0, 0.0, 100.0000000001, 100.0, 300, 20});
        project.floors[0].walls.push_back({"w3", 0, 100.0, 100.0, 0.0, 100.0, 300, 20});
        project.floors[0].walls.push_back({"w4", 0, 0.0, 100.0, 0.0, 0.0, 300, 20});

        RecomputeRooms(project, 0);
        CHECK(project.floors[0].rooms.size() == 1);
    }

    SECTION("7. Large coordinate values")
    {
        double offset = 1000000.0;
        project.floors[0].walls.push_back({"w1", 0, offset, offset, offset + 100.0, offset, 300, 20});
        project.floors[0].walls.push_back({"w2", 0, offset + 100.0, offset, offset + 100.0, offset + 100.0, 300, 20});
        project.floors[0].walls.push_back({"w3", 0, offset + 100.0, offset + 100.0, offset, offset + 100.0, 300, 20});
        project.floors[0].walls.push_back({"w4", 0, offset, offset + 100.0, offset, offset, 300, 20});

        RecomputeRooms(project, 0);
        CHECK(project.floors[0].rooms.size() == 1);
    }

    SECTION("8. Multiple nested loops")
    {
        // Inner square: (25,25) to (75,75)
        project.floors[0].walls.push_back({"i1", 0, 25, 25, 75, 25, 300, 20});
        project.floors[0].walls.push_back({"i2", 0, 75, 25, 75, 75, 300, 20});
        project.floors[0].walls.push_back({"i3", 0, 75, 75, 25, 75, 300, 20});
        project.floors[0].walls.push_back({"i4", 0, 25, 75, 25, 25, 300, 20});

        // Outer square: (0,0) to (100,100)
        project.floors[0].walls.push_back({"o1", 0, 0, 0, 100, 0, 300, 20});
        project.floors[0].walls.push_back({"o2", 0, 100, 0, 100, 100, 300, 20});
        project.floors[0].walls.push_back({"o3", 0, 100, 100, 0, 100, 300, 20});
        project.floors[0].walls.push_back({"o4", 0, 0, 100, 0, 0, 300, 20});

        RecomputeRooms(project, 0);

        // The current planar face extractor SHOULD find two interior rooms.
        // One for the inner square, and one for the 'annulus' between inner and outer.
        // Wait, if they don't share walls, they are separate components.
        // (0,0)-(100,100) is one component. (25,25)-(75,75) is another.
        // Faces found will be:
        // 1. Inner room (pos area)
        // 2. Outer infinite face of inner component (neg area)
        // 3. Outer room (pos area)
        // 4. Outer infinite face of outer component (neg area)
        // ExtractFaces should find 4 cycles total if components are disjoint.
        // DetectRooms filters pos area. So it should find 2 rooms.

        CHECK(project.floors[0].rooms.size() == 2);
    }
}
