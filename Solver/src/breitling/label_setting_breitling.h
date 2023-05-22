#pragma once

#include "breitlingSolver.h"

class LabelSettingBreitlingSolver : public PathSolver {
private:
  BreitlingData m_dataset;
public:
  LabelSettingBreitlingSolver(const BreitlingData &dataset)
    : m_dataset(dataset)
  {
  }

  virtual ProblemPath solveForPath(const ProblemMap &map) override;
};