#pragma once

#include "../pathsolver.h"

class TspNearest : public PathSolver {
public:
    Path solveForPath(const GeoMap &map) override;
};
