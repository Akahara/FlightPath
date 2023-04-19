#pragma once

#include "breitlingsolver.h"

class NaturalBreitlingSolver : public PathSolver {
private:
  BreitlingData m_dataset;
public:
  NaturalBreitlingSolver(const BreitlingData &dataset)
    : m_dataset(dataset)
  {
  }

  virtual Path solveForPath(const ProblemMap &map) override;
};