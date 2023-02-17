#pragma once

#include "../pathsolver.h"

// day time, normally in range 0..24 (ie. 6.5 is 6:30am)
// Unless otherwise specified, methods should respond well to values in range 0..inf
typedef float daytime_t;

// to be renamed
// All daytime fields in this struct should be in the 0..24 range
struct BreitlingData {
    nauticmiles_t planeSpeed;    // nautic miles per hour
    daytime_t nauticalDaytime;   // sunrise time (<nauticalNighttime)
    daytime_t nauticalNighttime; // sunset time (>nauticalDaytime)
    daytime_t departureTime;     // time at which the plane starts flying
    double planeFuelCapacity;    // the fuel capacity of the plane, unit must match planeFuelUsage
    double planeFuelUsage;       // the speed at which fuel is consumed, per hour, fuel unit must match planeFuelCapacity
    const Station *departureStation;
    const Station *targetStation;
};

namespace breitling_constraints {

// the plane must do the whole flyght in less than 24 hours
constexpr daytime_t MAXIMUM_FLYGHT_DURATION = 24;
// the plane must go through 100 stations minimum
constexpr size_t MINIMUM_STATION_COUNT = 100;

// the path must go through 4 stations, one for each cardinal direction
bool satisfiesCardinalsConstraints(const Path &path);
// the path must go through a set number of stations
bool satisfiesStationCountConstraints(const Path &path);
// the path must start and end at set cities
bool satisfiesPathConstraints(const BreitlingData &dataset, const Path &path);
// the path must allow the plane to not go out of fuel, it is assumed that the plane fuels up at *every* station when possible
bool satisfiesFuelConstraints(const BreitlingData &dataset, const Path &path);
// the whole path must not take too long
bool satisfiesTimeConstraints(const BreitlingData &dataset, const Path &path);

// check all satisfiesXXConstraints, users may want to check time constraints separately because
// slow planes simply cannot go through the set number of stations in due time
bool isPathValid(const BreitlingData &dataset, const Path &path)
{
    return satisfiesCardinalsConstraints(path) &&
        satisfiesStationCountConstraints(path) &&
        satisfiesPathConstraints(dataset, path) &&
        satisfiesFuelConstraints(dataset, path) &&
        satisfiesTimeConstraints(dataset, path);
}

// a plane can refuel at a station at daytime or during nighttime if the station allows it
bool canBeUsedToFuel(const BreitlingData &dataset, const Station &station, daytime_t daytime);

}

class BreitlingSolver : public PathSolver {
private:
    BreitlingData m_dataset;
public:
    BreitlingSolver(const BreitlingData &dataset)
        : m_dataset(dataset)
    {
    }

    virtual Path solveForPath(const GeoMap &map) override;
};
