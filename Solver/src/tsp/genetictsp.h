#pragma once

#include "../pathsolver.h"

class GeneticTSPSolver : public PathSolver {
public:
    virtual Path solveForPath(const GeoMap &map) override;
};
