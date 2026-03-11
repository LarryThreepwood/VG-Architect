#include "catch.hpp"
#include "vg_architect/core/model.hpp"
#include "vg_architect/core/wall_graph.hpp"

using namespace vg_architect::core;

TEST_CASE("WallGraph Construction", "[graph]")
{
    Project project;
    project.floors.push_back({0, {}, {}});

    // Create a T-junction to test node merging and high degree
    // w1: (0,0) to (100,0)
    // w2: (100,0) to (200,0)
    // w3: (100,0) to (100,100)
    project.floors[0].walls.push_back({"w1", 0, 0.0, 0.0, 100.0, 0.0, 300.0, 20.0});
    project.floors[0].walls.push_back({"w2", 0, 100.0, 0.0, 200.0, 0.0, 300.0, 20.0});
    project.floors[0].walls.push_back({"w3", 0, 100.0, 0.0, 100.0, 100.0, 300.0, 20.0});

    WallGraph graph = BuildWallGraph(project, 0);

    SECTION("Node merging and ordering")
    {
        // Unique nodes should be: (0,0), (100,0), (200,0), (100,100)
        // Sorted: (0,0), (100,0), (100,100), (200,0)
        REQUIRE(graph.nodes.size() == 4);
        CHECK(graph.nodes[0].x == 0.0);
        CHECK(graph.nodes[0].z == 0.0);
        CHECK(graph.nodes[1].x == 100.0);
        CHECK(graph.nodes[1].z == 0.0);
        CHECK(graph.nodes[2].x == 100.0);
        CHECK(graph.nodes[2].z == 100.0);
        CHECK(graph.nodes[3].x == 200.0);
        CHECK(graph.nodes[3].z == 0.0);
    }

    SECTION("Edge ordering and adjacency")
    {
        // Node indices:
        // n0: (0,0)
        // n1: (100,0)
        // n2: (100,100)
        // n3: (200,0)

        // Edges:
        // w1: n0-n1
        // w2: n1-n3
        // w3: n1-n2

        // Edge sorting (nodeA, nodeB):
        // 1. (0, 1) -> e0 (w1)
        // 2. (1, 2) -> e1 (w3)
        // 3. (1, 3) -> e2 (w2)

        REQUIRE(graph.edges.size() == 3);
        CHECK(graph.edges[0].wallIds[0] == "w1");
        CHECK(graph.edges[1].wallIds[0] == "w3");
        CHECK(graph.edges[2].wallIds[0] == "w2");

        // Incident edges:
        // n0: {0}
        // n1: {0, 1, 2}  (T-junction)
        // n2: {1}
        // n3: {2}

        CHECK(graph.nodes[1].incidentEdges.size() == 3);
        CHECK(graph.nodes[1].incidentEdges[0] == 0);
        CHECK(graph.nodes[1].incidentEdges[1] == 1);
        CHECK(graph.nodes[1].incidentEdges[2] == 2);
    }

    SECTION("Epsilon merging")
    {
        Project p2;
        p2.floors.push_back({0, {}, {}});
        // (0,0) to (100,0)
        p2.floors[0].walls.push_back({"w1", 0, 0.0, 0.0, 100.0, 0.0, 300.0, 20.0});
        // (100.0001, 0) to (200,0) -- should merge with (100,0)
        p2.floors[0].walls.push_back({"w2", 0, 100.0001, 0.0, 200.0, 0.0, 300.0, 20.0});

        WallGraph g2 = BuildWallGraph(p2, 0);
        REQUIRE(g2.nodes.size() == 3);
    }
}

TEST_CASE("WallGraph Consolidation", "[graph]")
{
    Project project;
    project.floors.push_back({0, {}, {}});

    // Duplicate walls
    project.floors[0].walls.push_back({"w1", 0, 0.0, 0.0, 100.0, 0.0, 300, 20});
    project.floors[0].walls.push_back({"w1_dup", 0, 0.0, 0.0, 100.0, 0.0, 300, 20});

    // Reverse wall
    project.floors[0].walls.push_back({"w2", 0, 100.0, 0.0, 100.0, 100.0, 300, 20});
    project.floors[0].walls.push_back({"w2_rev", 0, 100.0, 100.0, 100.0, 0.0, 300, 20});

    WallGraph graph = BuildWallGraph(project, 0);

    REQUIRE(graph.edges.size() == 2);

    // Edge 0: (0,0) to (100,0)
    REQUIRE(graph.edges[0].wallIds.size() == 2);
    CHECK(graph.edges[0].wallIds[0] == "w1");
    CHECK(graph.edges[0].wallIds[1] == "w1_dup");

    // Edge 1: (100,0) to (100,100)
    REQUIRE(graph.edges[1].wallIds.size() == 2);
    CHECK(graph.edges[1].wallIds[0] == "w2");
    CHECK(graph.edges[1].wallIds[1] == "w2_rev");
}

TEST_CASE("Empty Graph", "[graph]")
{
    Project project;
    WallGraph graph = BuildWallGraph(project, 99);
    CHECK(graph.nodes.empty());
    CHECK(graph.edges.empty());
}
