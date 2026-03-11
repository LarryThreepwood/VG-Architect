#include "catch.hpp"
#include "vg_architect/core/model.hpp"
#include "vg_architect/core/wall_graph.hpp"
#include "vg_architect/core/half_edge_topology.hpp"
#include "vg_architect/core/face_extraction.hpp"

using namespace vg_architect::core;

TEST_CASE("Planar Face Extraction", "[face]")
{
    SECTION("Simple Square")
    {
        Project p;
        p.floors.push_back({0, {}, {}});
        // (0,0) -> (100,0) -> (100,100) -> (0,100)
        p.floors[0].walls.push_back({"w1", 0, 0, 0, 100, 0, 300, 20});
        p.floors[0].walls.push_back({"w2", 0, 100, 0, 100, 100, 300, 20});
        p.floors[0].walls.push_back({"w3", 0, 100, 100, 0, 100, 300, 20});
        p.floors[0].walls.push_back({"w4", 0, 0, 100, 0, 0, 300, 20});

        WallGraph g = BuildWallGraph(p, 0);
        HalfEdgeTopology t = BuildHalfEdgeTopology(g);
        std::vector<FaceCycle> faces = ExtractFaces(g, t);

        // Expect 2 faces: 1 interior (positive area), 1 outer (negative area)
        REQUIRE(faces.size() == 2);

        int positiveCount = 0;
        int negativeCount = 0;
        for (const auto& f : faces) {
            if (f.signedArea > 0) positiveCount++;
            if (f.signedArea < 0) negativeCount++;
        }
        CHECK(positiveCount == 1);
        CHECK(negativeCount == 1);
    }

    SECTION("Rectangle split by wall (Two rooms)")
    {
        Project p;
        p.floors.push_back({0, {}, {}});
        p.floors[0].walls.push_back({"w1", 0, 0, 0, 0, 100, 300, 20});
        p.floors[0].walls.push_back({"w2", 0, 0, 100, 100, 100, 300, 20});
        p.floors[0].walls.push_back({"wS", 0, 100, 100, 100, 0, 300, 20}); // Splitter
        p.floors[0].walls.push_back({"w4", 0, 100, 0, 0, 0, 300, 20});
        p.floors[0].walls.push_back({"w5", 0, 100, 100, 200, 100, 300, 20});
        p.floors[0].walls.push_back({"w6", 0, 200, 100, 200, 0, 300, 20});
        p.floors[0].walls.push_back({"w7", 0, 200, 0, 100, 0, 300, 20});

        WallGraph g = BuildWallGraph(p, 0);
        HalfEdgeTopology t = BuildHalfEdgeTopology(g);
        std::vector<FaceCycle> faces = ExtractFaces(g, t);

        // Expect 3 faces: 2 interior rooms, 1 outer void
        REQUIRE(faces.size() == 3);
        int interiorCount = 0;
        int outerCount = 0;
        for (const auto& f : faces) {
            if (f.signedArea > 0) interiorCount++;
            else if (f.signedArea < 0) outerCount++;
        }
        CHECK(interiorCount == 2);
        CHECK(outerCount == 1);
    }

    SECTION("T-junction (No closed rooms)")
    {
        Project p;
        p.floors.push_back({0, {}, {}});
        p.floors[0].walls.push_back({"w1", 0, 0, 0, 100, 0, 300, 20});
        p.floors[0].walls.push_back({"w2", 0, 100, 0, 200, 0, 300, 20});
        p.floors[0].walls.push_back({"w3", 0, 100, 0, 100, 100, 300, 20});

        WallGraph g = BuildWallGraph(p, 0);
        HalfEdgeTopology t = BuildHalfEdgeTopology(g);
        std::vector<FaceCycle> faces = ExtractFaces(g, t);

        // For a tree structure (no cycles), all half-edges belong to the same face.
        // The outer face of a tree has zero area because it traverses every edge twice in opposite directions.
        REQUIRE(faces.size() == 1);
        CHECK(std::abs(faces[0].signedArea) < 1e-9);
    }
}
