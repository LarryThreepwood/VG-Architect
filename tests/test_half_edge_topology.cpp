#include "catch.hpp"
#include "vg_architect/core/model.hpp"
#include "vg_architect/core/wall_graph.hpp"
#include "vg_architect/core/half_edge_topology.hpp"

#include <unordered_set>

using namespace vg_architect::core;

void VerifyInvariants(const HalfEdgeTopology& topology)
{
    for (int i = 0; i < static_cast<int>(topology.halfEdges.size()); ++i)
    {
        const auto& he = topology.halfEdges[i];

        // Twin relationship
        CHECK(topology.halfEdges[he.twin].twin == i);

        // Next/prev consistency
        CHECK(topology.halfEdges[he.next].prev == i);
        CHECK(topology.halfEdges[he.prev].next == i);

        // Origin node consistency
        // (This would require access to the graph to be perfect, but we can check internal consistency)
        // twin(he).next.origin == he.origin? No.
        // next(he).origin == twin(he).origin? Yes, should be.
        int destNode = topology.halfEdges[he.twin].originNode;
        CHECK(topology.halfEdges[he.next].originNode == destNode);
    }
}

void VerifyCycles(const HalfEdgeTopology& topology)
{
    std::unordered_set<int> visited;
    for (int i = 0; i < static_cast<int>(topology.halfEdges.size()); ++i)
    {
        if (visited.count(i)) continue;

        int curr = i;
        int count = 0;
        int maxCount = static_cast<int>(topology.halfEdges.size());

        do {
            visited.insert(curr);
            curr = topology.halfEdges[curr].next;
            count++;
            REQUIRE(count <= maxCount); // Prevent infinite loops
        } while (curr != i);

        CHECK(count >= 3); // Minimal planar cycle
    }
}

TEST_CASE("Topology Invariants and Cycles", "[topology]")
{
    SECTION("Simple Square")
    {
        Project p;
        p.floors.push_back({0, {}, {}});
        p.floors[0].walls.push_back({"w1", 0, 0, 0, 100, 0, 300, 20});
        p.floors[0].walls.push_back({"w2", 0, 100, 0, 100, 100, 300, 20});
        p.floors[0].walls.push_back({"w3", 0, 100, 100, 0, 100, 300, 20});
        p.floors[0].walls.push_back({"w4", 0, 0, 100, 0, 0, 300, 20});

        WallGraph g = BuildWallGraph(p, 0);
        HalfEdgeTopology t = BuildHalfEdgeTopology(g);

        VerifyInvariants(t);
        VerifyCycles(t);
    }

    SECTION("Rectangle split by a wall (Shared wall)")
    {
        Project p;
        p.floors.push_back({0, {}, {}});
        // Left room
        p.floors[0].walls.push_back({"w1", 0, 0, 0, 0, 100, 300, 20});
        p.floors[0].walls.push_back({"w2", 0, 0, 100, 100, 100, 300, 20});
        p.floors[0].walls.push_back({"wS", 0, 100, 100, 100, 0, 300, 20}); // Shared
        p.floors[0].walls.push_back({"w4", 0, 100, 0, 0, 0, 300, 20});
        // Right room extension
        p.floors[0].walls.push_back({"w5", 0, 100, 100, 200, 100, 300, 20});
        p.floors[0].walls.push_back({"w6", 0, 200, 100, 200, 0, 300, 20});
        p.floors[0].walls.push_back({"w7", 0, 200, 0, 100, 0, 300, 20});

        WallGraph g = BuildWallGraph(p, 0);
        HalfEdgeTopology t = BuildHalfEdgeTopology(g);

        VerifyInvariants(t);
        VerifyCycles(t);
    }

    SECTION("T-junction")
    {
        Project p;
        p.floors.push_back({0, {}, {}});
        p.floors[0].walls.push_back({"w1", 0, 0, 0, 100, 0, 300, 20});
        p.floors[0].walls.push_back({"w2", 0, 100, 0, 200, 0, 300, 20});
        p.floors[0].walls.push_back({"w3", 0, 100, 0, 100, 100, 300, 20});

        WallGraph g = BuildWallGraph(p, 0);
        HalfEdgeTopology t = BuildHalfEdgeTopology(g);

        VerifyInvariants(t);
        VerifyCycles(t);
    }

    SECTION("Cross intersection")
    {
        Project p;
        p.floors.push_back({0, {}, {}});
        p.floors[0].walls.push_back({"w1", 0, 0, 0, 100, 0, 300, 20});
        p.floors[0].walls.push_back({"w2", 0, 100, 0, 200, 0, 300, 20});
        p.floors[0].walls.push_back({"w3", 0, 100, 0, 100, 100, 300, 20});
        p.floors[0].walls.push_back({"w4", 0, 100, 0, 100, -100, 300, 20});

        WallGraph g = BuildWallGraph(p, 0);
        HalfEdgeTopology t = BuildHalfEdgeTopology(g);

        VerifyInvariants(t);
        VerifyCycles(t);
    }
}

TEST_CASE("Topology Determinism", "[topology]")
{
    Project p;
    p.floors.push_back({0, {}, {}});
    p.floors[0].walls.push_back({"w1", 0, 0, 0, 100, 0, 300, 20});
    p.floors[0].walls.push_back({"w2", 0, 100, 0, 100, 100, 300, 20});
    p.floors[0].walls.push_back({"w3", 0, 100, 100, 0, 100, 300, 20});
    p.floors[0].walls.push_back({"w4", 0, 0, 100, 0, 0, 300, 20});

    WallGraph g1 = BuildWallGraph(p, 0);
    HalfEdgeTopology t1 = BuildHalfEdgeTopology(g1);

    WallGraph g2 = BuildWallGraph(p, 0);
    HalfEdgeTopology t2 = BuildHalfEdgeTopology(g2);

    REQUIRE(t1.halfEdges.size() == t2.halfEdges.size());
    for (size_t i = 0; i < t1.halfEdges.size(); ++i) {
        CHECK(t1.halfEdges[i].originNode == t2.halfEdges[i].originNode);
        CHECK(t1.halfEdges[i].twin == t2.halfEdges[i].twin);
        CHECK(t1.halfEdges[i].next == t2.halfEdges[i].next);
        CHECK(t1.halfEdges[i].prev == t2.halfEdges[i].prev);
    }
}
