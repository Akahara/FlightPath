#pragma once

#include <map>

#include "../pathsolver.h"
#include "../breitling/label_setting_breitling.h"
#include "../breitling/BreitlingSolver.h"
#include "../geometry.h"

typedef double timedistance_t;

namespace tools
{

	bool ProblemStationInPath(std::vector<const ProblemStation*> path, ProblemStation& station);
	bool ProblemStationInProblemStationVector(std::vector<ProblemStation*>stationVector, ProblemStation& station);
	bool EndProblemStationInReachableProblemStations(std::vector<ProblemStation*> stations,  ProblemStation& endProblemStation);
	std::vector<timedistance_t*> SortTimeDistance(std::vector<timedistance_t*> distanceList);
  Location findHalfWayCoordinates(Location start, Location end);

}

//A structure to track necessary information during the flight
struct travel_variables
{
	nauticmiles_t flightDistance =0;
	nauticmiles_t distanceSinceLastRefuel =0;
	daytime_t currentTime = 0;

	void updateTime(BreitlingData dataset)
	{
		currentTime = dataset.departureTime + flightDistance / dataset.planeSpeed;
	}
	
};


class OptimisationSolver : public PathSolver 
{
	BreitlingData m_dataset;
	travel_variables* travel = new travel_variables();
	std::vector<const ProblemStation*> chemin;

public:
	OptimisationSolver(const BreitlingData& dataset)
		: m_dataset(dataset)
	{
	}

  ProblemPath solveForPath(const ProblemMap& map, SolverRuntime *runtime) override;

private:
	void TestPath(std::vector<const ProblemStation*>& chemin);
	std::vector<ProblemMap*> seperateRegion(const ProblemMap& map);
  void initializePath(const std::vector<ProblemMap*> regions,ProblemStation* startingProblemStation, ProblemStation* endProblemStation);
	std::vector<ProblemStation> RefuelableStation(const ProblemMap& map, const ProblemStation& centerProblemStation, travel_variables travel, bool refuelable);
	ProblemStation* stationSelectionInReach(const ProblemMap& map, const ProblemStation& startProblemStation,const ProblemStation& destinationProblemStation, std::vector<ProblemStation*> skipList, travel_variables* travel, bool refuelable);
	size_t findProblematicProblemStation(std::vector<const ProblemStation*> path, travel_variables* travel);
  std::vector<ProblemStation*> connectProblemStationsTogether(const ProblemMap& map,const ProblemStation& startProblemStation, const ProblemStation& endProblemStation, travel_variables* travel, bool refuelable, bool *stopFlag);
	void updateTravelVariable(travel_variables* travel, const ProblemStation* point);
	void resetTestVariable();
  void findIntermidiateRefillableStation(const ProblemMap& map, std::vector<const ProblemStation*> path, travel_variables* travel, bool *stopFlag);
	Path transformToPath(std::vector<const ProblemStation*> path);
	void addRefuelStationIfFirstStationUnreachable(const ProblemMap& map, std::vector<const ProblemStation*> path);
};
