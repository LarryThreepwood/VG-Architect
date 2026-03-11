#include "catch.hpp"
#include "vg_architect/core/model.hpp"
#include "vg_architect/core/wall_ops.hpp"
#include "vg_architect/core/export_geometry.hpp"

using namespace vg_architect::core;

TEST_CASE("Empty Project Export", "[export]")
{
    Project project;
    ExportedProject exported = ExportProject(project);
    CHECK(exported.floors.empty());
}

TEST_CASE("Empty Floor Export", "[export]")
{
    Project project;
    project.floors.push_back({0, {}, {}});
    ExportedProject exported = ExportProject(project);
    REQUIRE(exported.floors.size() == 1);
    CHECK(exported.floors[0].floorIndex == 0);
    CHECK(exported.floors[0].walls.empty());
    CHECK(exported.floors[0].rooms.empty());
}

TEST_CASE("Deterministic Wall Ordering", "[export]")
{
    Project project;
    Floor floor;
    floor.floorIndex = 0;

    // Walls out of order
    floor.walls.push_back({"w3", 0, 100.0, 100.0, 0.0, 100.0, 300.0, 20.0});
    floor.walls.push_back({"w1", 0, 0.0, 0.0, 100.0, 0.0, 300.0, 20.0});
    floor.walls.push_back({"w4", 0, 0.0, 100.0, 0.0, 0.0, 300.0, 20.0});
    floor.walls.push_back({"w2", 0, 100.0, 0.0, 100.0, 100.0, 300.0, 20.0});

    project.floors.push_back(floor);

    ExportedProject exported = ExportProject(project);
    REQUIRE(exported.floors.size() == 1);
    REQUIRE(exported.floors[0].walls.size() == 4);

    // Sorted by startX, startZ, endX, endZ, id
    // w1: (0,0) to (100,0)
    // w4: (0,100) to (0,0)
    // w2: (100,0) to (100,100)
    // w3: (100,100) to (0,100)

    CHECK(exported.floors[0].walls[0].id == "w1");
    CHECK(exported.floors[0].walls[1].id == "w4");
    CHECK(exported.floors[0].walls[2].id == "w2");
    CHECK(exported.floors[0].walls[3].id == "w3");
}

TEST_CASE("Identical Coordinate Walls Ordering", "[export]")
{
    Project project;
    Floor floor;
    floor.floorIndex = 0;

    floor.walls.push_back({"wb", 0, 0.0, 0.0, 100.0, 0.0, 300.0, 20.0});
    floor.walls.push_back({"wa", 0, 0.0, 0.0, 100.0, 0.0, 300.0, 20.0});

    project.floors.push_back(floor);

    ExportedProject exported = ExportProject(project);
    REQUIRE(exported.floors[0].walls.size() == 2);
    CHECK(exported.floors[0].walls[0].id == "wa");
    CHECK(exported.floors[0].walls[1].id == "wb");
}

TEST_CASE("Deterministic Floor Ordering", "[export]")
{
    Project project;
    project.floors.push_back({1, {}, {}});
    project.floors.push_back({0, {}, {}});
    project.floors.push_back({2, {}, {}});

    ExportedProject exported = ExportProject(project);
    REQUIRE(exported.floors.size() == 3);
    CHECK(exported.floors[0].floorIndex == 0);
    CHECK(exported.floors[1].floorIndex == 1);
    CHECK(exported.floors[2].floorIndex == 2);
}

TEST_CASE("Stable Room Export and ID Generation", "[export]")
{
    Project project;
    Floor floor;
    floor.floorIndex = 0;
    floor.walls.push_back({"w1", 0, 0.0, 0.0, 100.0, 0.0, 300.0, 20.0});
    floor.walls.push_back({"w2", 0, 100.0, 0.0, 100.0, 100.0, 300.0, 20.0});
    floor.walls.push_back({"w3", 0, 100.0, 100.0, 0.0, 100.0, 300.0, 20.0});
    floor.walls.push_back({"w4", 0, 0.0, 100.0, 0.0, 0.0, 300.0, 20.0});

    Room room;
    room.floorIndex = 0;
    room.boundaryWallIds = {"w1", "w2", "w3", "w4"};
    floor.rooms.push_back(room);

    project.floors.push_back(floor);

    SECTION("Room ID and vertices are stable")
    {
        ExportedProject exported1 = ExportProject(project);
        ExportedProject exported2 = ExportProject(project);

        REQUIRE(exported1.floors[0].rooms.size() == 1);
        REQUIRE(exported2.floors[0].rooms.size() == 1);

        CHECK(exported1.floors[0].rooms[0].id == "room_w1_w2_w3_w4");
        CHECK(exported1.floors[0].rooms[0].id == exported2.floors[0].rooms[0].id);

        REQUIRE(exported1.floors[0].rooms[0].boundaryVertices.size() == 4);
        for (size_t i = 0; i < 4; ++i) {
            CHECK(exported1.floors[0].rooms[0].boundaryVertices[i].x == exported2.floors[0].rooms[0].boundaryVertices[i].x);
            CHECK(exported1.floors[0].rooms[0].boundaryVertices[i].z == exported2.floors[0].rooms[0].boundaryVertices[i].z);
        }
    }
}

TEST_CASE("Repeated Export Identical Results", "[export]")
{
    Project project;
    Floor floor;
    floor.floorIndex = 0;
    floor.walls.push_back({"w1", 0, 0.0, 0.0, 100.0, 0.0, 300.0, 20.0});
    project.floors.push_back(floor);

    ExportedProject exp1 = ExportProject(project);
    ExportedProject exp2 = ExportProject(project);

    REQUIRE(exp1.floors.size() == exp2.floors.size());
    REQUIRE(exp1.floors[0].walls.size() == exp2.floors[0].walls.size());
    CHECK(exp1.floors[0].walls[0].id == exp2.floors[0].walls[0].id);
}
