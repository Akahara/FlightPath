#pragma once

#include "../pathsolver.h"

class GeneticTSPSolver : public PathSolver {
public:
    virtual ProblemPath solveForPath(const ProblemMap &map, bool *stopFlag=nullptr) override;
};
