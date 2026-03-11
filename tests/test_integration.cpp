#include "catch.hpp"
#include "vg_architect/core/model.hpp"
#include "vg_architect/core/wall_ops.hpp"

#include <algorithm>
#include <random>

using namespace vg_architect::core;

TEST_CASE("RecomputeRooms Integration", "[integration]")
{
    SECTION("Deterministic regardless of input order")
    {
        Project p1, p2;
        p1.floors.push_back({0, {}, {}});
        p2.floors.push_back({0, {}, {}});

        std::vector<Wall> walls = {
            {"w1", 0, 0, 0, 100, 0, 300, 20},
            {"w2", 0, 100, 0, 100, 100, 300, 20},
            {"w3", 0, 100, 100, 0, 100, 300, 20},
            {"w4", 0, 0, 100, 0, 0, 300, 20}
        };

        for (const auto& w : walls) p1.floors[0].walls.push_back(w);

        std::vector<Wall> shuffled = walls;
        std::shuffle(shuffled.begin(), shuffled.end(), std::mt19937(42));
        for (const auto& w : shuffled) p2.floors[0].walls.push_back(w);

        RecomputeRooms(p1, 0);
        RecomputeRooms(p2, 0);

        REQUIRE(p1.floors[0].rooms.size() == 1);
        REQUIRE(p2.floors[0].rooms.size() == 1);
        CHECK(p1.floors[0].rooms[0].id == p2.floors[0].rooms[0].id);
        CHECK(p1.floors[0].rooms[0].boundaryWallIds.size() == p2.floors[0].rooms[0].boundaryWallIds.size());
    }

    SECTION("Shared walls produce separate rooms")
    {
        Project p;
        p.floors.push_back({0, {}, {}});
        // Room 1
        p.floors[0].walls.push_back({"w1", 0, 0, 0, 0, 100, 300, 20});
        p.floors[0].walls.push_back({"w2", 0, 0, 100, 100, 100, 300, 20});
        p.floors[0].walls.push_back({"wS", 0, 100, 100, 100, 0, 300, 20}); // Shared
        p.floors[0].walls.push_back({"w4", 0, 100, 0, 0, 0, 300, 20});
        // Room 2
        p.floors[0].walls.push_back({"w5", 0, 100, 100, 200, 100, 300, 20});
        p.floors[0].walls.push_back({"w6", 0, 200, 100, 200, 0, 300, 20});
        p.floors[0].walls.push_back({"w7", 0, 200, 0, 100, 0, 300, 20});

        RecomputeRooms(p, 0);

        REQUIRE(p.floors[0].rooms.size() == 2);
    }

    SECTION("T-junction layouts produce zero rooms")
    {
        Project p;
        p.floors.push_back({0, {}, {}});
        p.floors[0].walls.push_back({"w1", 0, 0, 0, 100, 0, 300, 20});
        p.floors[0].walls.push_back({"w2", 0, 100, 0, 200, 0, 300, 20});
        p.floors[0].walls.push_back({"w3", 0, 100, 0, 100, 100, 300, 20});

        RecomputeRooms(p, 0);

        CHECK(p.floors[0].rooms.empty());
    }
}
