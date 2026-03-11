#include "catch.hpp"
#include "vg_architect/core/model.hpp"
#include "vg_architect/core/wall_graph.hpp"
#include "vg_architect/core/half_edge_topology.hpp"

using namespace vg_architect::core;

TEST_CASE("Half-Edge Topology Construction", "[topology]")
{
    Project project;
    project.floors.push_back({0, {}, {}});

    // Create a 100x100 square room
    // n0: (0,0), n1: (100,0), n2: (100,100), n3: (0,100)
    // w1: (0,0) to (100,0)
    // w2: (100,0) to (100,100)
    // w3: (100,100) to (0,100)
    // w4: (0,100) to (0,0)
    project.floors[0].walls.push_back({"w1", 0, 0.0, 0.0, 100.0, 0.0, 300.0, 20.0});
    project.floors[0].walls.push_back({"w2", 0, 100.0, 0.0, 100.0, 100.0, 300.0, 20.0});
    project.floors[0].walls.push_back({"w3", 0, 100.0, 100.0, 0.0, 100.0, 300.0, 20.0});
    project.floors[0].walls.push_back({"w4", 0, 0.0, 100.0, 0.0, 0.0, 300.0, 20.0});

    WallGraph graph = BuildWallGraph(project, 0);
    HalfEdgeTopology topology = BuildHalfEdgeTopology(graph);

    SECTION("Twin relationships")
    {
        for (size_t i = 0; i < topology.halfEdges.size(); ++i)
        {
            int twinIdx = topology.halfEdges[i].twin;
            CHECK(topology.halfEdges[twinIdx].twin == static_cast<int>(i));
            CHECK(topology.halfEdges[i].edgeIndex == topology.halfEdges[twinIdx].edgeIndex);
            CHECK(topology.halfEdges[i].originNode != topology.halfEdges[twinIdx].originNode);
        }
    }

    SECTION("Angular ordering around junction (T-junction)")
    {
        Project pT;
        pT.floors.push_back({0, {}, {}});
        // (100,0) is the center node
        pT.floors[0].walls.push_back({"wA", 0, 0.0, 0.0, 100.0, 0.0, 300.0, 20.0});      // angle PI (left)
        pT.floors[0].walls.push_back({"wB", 0, 100.0, 0.0, 200.0, 0.0, 300.0, 20.0});    // angle 0 (right)
        pT.floors[0].walls.push_back({"wC", 0, 100.0, 0.0, 100.0, 100.0, 300.0, 20.0});  // angle PI/2 (up)

        WallGraph gT = BuildWallGraph(pT, 0);
        HalfEdgeTopology tT = BuildHalfEdgeTopology(gT);

        // Find node (100,0)
        int centerIdx = -1;
        for (size_t i = 0; i < gT.nodes.size(); ++i) {
            if (gT.nodes[i].x == 100.0 && gT.nodes[i].z == 0.0) {
                centerIdx = static_cast<int>(i);
                break;
            }
        }

        // Get half-edges originating from centerIdx in order
        std::vector<int> incident;
        int start = tT.nodeFirstHalfEdge[centerIdx];
        int curr = start;
        do {
            incident.push_back(curr);
            // In BuildHalfEdgeTopology, next(twin(he)) is the next he around node.
            // But he.next is face traversal. Let's find neighbors by next around vertex.
            // My implementation links he_twin.next = next_he_around_origin_node.
            // So to traverse around vertex: curr = he_twin.next
            curr = tT.halfEdges[tT.halfEdges[curr].twin].next; // this is next he around centerIdx
        } while (curr != start);

        // Expected angles: 0 (right), PI/2 (up), PI (left)
        REQUIRE(incident.size() == 3);

        auto get_angle = [&](int heIdx) {
            const auto& he = tT.halfEdges[heIdx];
            const auto& edge = gT.edges[he.edgeIndex];
            int destIdx = (he.originNode == edge.nodeA) ? edge.nodeB : edge.nodeA;
            return std::atan2(gT.nodes[destIdx].z - gT.nodes[he.originNode].z,
                              gT.nodes[destIdx].x - gT.nodes[he.originNode].x);
        };

        CHECK(get_angle(incident[0]) == Approx(0.0));
        CHECK(get_angle(incident[1]) == Approx(M_PI/2.0));
        CHECK(get_angle(incident[2]) == Approx(M_PI));
    }
}
