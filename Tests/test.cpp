#include "pch.h"

#include <random>

#include "../Solver/src/geomap.h"
#include "../Solver/src/geometry.h"
#include "../Solver/src/tsp/TspSolver.h"
#include "../Solver/src/breitling/BreitlingSolver.h"

TEST(TestLocationCase, TestConstructors)
{
    Location l1{ 1, -2 };  // lon=-2 lat=+1   lon=2�S lat=1�N
    Location l2{ 1.5, 0 }; // lon=0  lat=
    EXPECT_EQ(l1, Location::fromNECoordinates(-2, +1));
    EXPECT_EQ(l1, Location::fromNECoordinates(+2, +1, Card::SE));
    EXPECT_EQ(l1, Location::fromDegreeMinuteSecond(2,0,0, 1,0,0, Card::SE));
    EXPECT_EQ(l2, Location::fromNECoordinates(+0, +1.5));
    EXPECT_EQ(l2, Location::fromNECoordinates(-0, +1.5, Card::NE));
    EXPECT_EQ(l2, Location::fromDegreeMinuteSecond(0,0,0, 1,30,0, Card::NE));

    EXPECT_EQ((Location{ 0,0 }), (Location{ 360, 0 }));
    EXPECT_EQ((Location{ 0,0 }), (Location{ 0, 360 }));
    EXPECT_EQ((Location{ 50,0 }).lon, 50);
    EXPECT_EQ((Location{ 360+50,0 }).lon, 50);
    EXPECT_EQ((Location{ 360 + 50,181 }), (Location{ -50, 1 }));
}

TEST(TestDistancesCase, TestLocationDistances)
{
    Location london = Location::fromDegreeMinuteSecond(51,28,14, 0,27,42, Card::NE);
    Location paris  = Location::fromDegreeMinuteSecond(49,0,46,  2,32,59, Card::NW);
    nauticmiles_t distanceNM = geometry::distance(london, paris);
    km_t distanceKM = geography::nauticmiles2km(distanceNM);
    EXPECT_NEAR(distanceNM, 187.456, .5); // .5nm range
    EXPECT_NEAR(distanceKM, 347.168, .5); // .5km range
}

TEST(TestPath, TestCreation)
{
    Station s1{ Location{0,0} };
    Station s2{ Location{10,0} };
    Station s3{ Location{20,0} };
    Station s4{ Location{30,0} };
    Path p{};
    p.getStations().push_back(&s1);
    p.getStations().push_back(&s2);
    p.getStations().push_back(&s3);
    p.getStations().push_back(&s4);
    ASSERT_EQ(p.size(), 4);
    EXPECT_FLOAT_EQ(p.length(),
                    geometry::distance(s1.getLocation(), s2.getLocation()) +
                    geometry::distance(s2.getLocation(), s3.getLocation()) +
                    geometry::distance(s3.getLocation(), s4.getLocation()));
}

TEST(TestPath, TestCycle)
{
    Station s1{ Location{0,0} };
    Station s2{ Location{10,0} };
    Station s3{ Location{20,0} };
    Station s4{ Location{30,0} };
    Path p{};
    p.getStations().push_back(&s1);
    p.getStations().push_back(&s2);
    p.getStations().push_back(&s3);
    p.getStations().push_back(&s4);
    EXPECT_FALSE(geometry::isCycle(p)); // not a cycle
    p.getStations().push_back(&s1);
    EXPECT_TRUE(geometry::isCycle(p));  // valid
    p.getStations().push_back(&s2);
    p.getStations().push_back(&s1);
    EXPECT_FALSE(geometry::isCycle(p)); // passes twice by s2 and s1
}

static GeoMap genDummyMap()
{
    GeoMap map;
    map.getStations().push_back(Station{ Location{0, 0} });
    map.getStations().push_back(Station{ Location{10,0} });
    map.getStations().push_back(Station{ Location{20,0} });
    map.getStations().push_back(Station{ Location{30,0} });
    return map;
}

static GeoMap genPredictableLargeMap(size_t stationCount, double seed=0)
{
    std::mt19937 engine{ seed };
    std::uniform_int_distribution<double> random{ 0, 360 };
    std::vector<double> positions;
    std::generate(positions.begin(), positions.end(), [&random, &engine]() { return random(engine); });

    GeoMap map;
    for (size_t i = 0; i < stationCount; i++) {
        map.getStations().push_back(Station{ Location{ positions[i*2], positions[i*2+1] } });
    }
    return map;
}

TEST(TestTSP, TestDummy)
{
    GeoMap map = genDummyMap();

    TSPSolver solver;
    Path path = solver.solveForPath(map);
    ASSERT_EQ(map.getStations().size(), path.size());
    ASSERT_TRUE(geometry::isCycle(path));

    // on the dummy example the expected path is known
    ASSERT_EQ(path[0], map.getStations()[0]);
    ASSERT_EQ(path[1], map.getStations()[1]);
    ASSERT_EQ(path[2], map.getStations()[2]);
    ASSERT_EQ(path[3], map.getStations()[3]);
}

TEST(TestTSP, TestLarge)
{
    GeoMap map = genPredictableLargeMap(100); // 100 stations

    TSPSolver solver;
    Path path = solver.solveForPath(map);
    ASSERT_EQ(map.getStations().size(), path.size());
    ASSERT_TRUE(geometry::isCycle(path));

    // local optimization: swapping two stations should be longer
    nauticmiles_t shortestLength = path.length();
    for (size_t i = 0; i < path.size(); i++) {
        for (size_t j = 0; j < path.size(); j++) {
            std::swap(path.getStations()[i], path.getStations()[j]);
            ASSERT_GE(path.length(), shortestLength);
            std::swap(path.getStations()[i], path.getStations()[j]);
        }
    }
}

TEST(TestBreitling, TestPredictable)
{
    GeoMap map = genPredictableLargeMap(200, 0);
    BreitlingData dataset{};
    dataset.nauticalDaytime   = 8;   // day starts at 8am
    dataset.nauticalNighttime = 20;  // night falls at 8pm
    dataset.departureTime     = 8;   // the course starts at 8am
    dataset.planeFuelCapacity = 100; // arbitrary capacity, 100 is nice because it can be interpreted as a percentage
    dataset.planeFuelUsage    = 10;  // 10% per hour
    dataset.planeSpeed        = 100; // 100nm/h
    dataset.departureStation  = &map.getStations()[0];
    dataset.targetStation     = &map.getStations()[1];
    
    BreitlingSolver solver{ dataset };
    Path path = solver.solveForPath(map);

    EXPECT_TRUE(breitling_constraints::isPathValid(dataset, path)); // check all constraints at once

    EXPECT_TRUE(breitling_constraints::satisfiesCardinalsConstraints(path));
    EXPECT_TRUE(breitling_constraints::satisfiesFuelConstraints(dataset, path));
    EXPECT_TRUE(breitling_constraints::satisfiesPathConstraints(dataset, path));
    EXPECT_TRUE(breitling_constraints::satisfiesStationCountConstraints(path));
    EXPECT_TRUE(breitling_constraints::satisfiesTimeConstraints(dataset, path));
}

TEST(TestBreitling, TestLarge)
{
    BreitlingData dataset{};
    dataset.nauticalDaytime = 8;   // day starts at 8am
    dataset.nauticalNighttime = 20;  // night falls at 8pm
    dataset.departureTime = 8;   // the course starts at 8am
    dataset.planeFuelCapacity = 100; // arbitrary capacity, 100 is nice because it can be interpreted as a percentage
    dataset.planeFuelUsage = 10;  // 10% per hour
    dataset.planeSpeed = 100; // 100nm/h

    for (size_t i = 0; i < 20; i++) {
        double seed = rand();
        GeoMap map = genPredictableLargeMap(200, seed);
        dataset.departureStation = &map.getStations()[0];
        dataset.targetStation = &map.getStations()[1];

        BreitlingSolver solver{ dataset };
        Path path = solver.solveForPath(map);

        EXPECT_TRUE(breitling_constraints::isPathValid(dataset, path)) << "with seed " << seed; // check all constraints at once

        EXPECT_TRUE(breitling_constraints::satisfiesCardinalsConstraints(path));
        EXPECT_TRUE(breitling_constraints::satisfiesFuelConstraints(dataset, path));
        EXPECT_TRUE(breitling_constraints::satisfiesPathConstraints(dataset, path));
        EXPECT_TRUE(breitling_constraints::satisfiesStationCountConstraints(path));
        EXPECT_TRUE(breitling_constraints::satisfiesTimeConstraints(dataset, path));
    }
}