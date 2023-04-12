#pragma once

#include "../pathsolver.h"

class TspNearest : public PathSolver {
public:
    Path solveForPath(const GeoMap &map) override;

private:
    static Path computeNearestPath(const GeoMap &map);
    static void o2opt(Path &path);
    static void o3opt(Path &path);
};
