#include "optimisationSolver.h"

#define NUMBER_REGION 4
#define FUEL_SECURITY_PERCENTAGE 0.25

#include <map>
#include <iostream>
#include <list>
#include <algorithm>    
#include <limits>
#include "../userinterface.h"


/**
* Seperate the map into 4 distinct regions : 
*	1 - Northwest region (index : 0)
*	2 - Southwest region (index : 1)
*	3 - Southeast region (index : 2)
*	4 - Northeast region (index : 3)
* This function return a vector of 4 pointers on ProblemMap.
*/
std::vector<ProblemMap*> OptimisationSolver::seperateRegion(const ProblemMap& map)
{
	//Initialization 
	std::vector<ProblemMap*> regions;
	ProblemMap* regionNorthEast = new ProblemMap();
	ProblemMap* regionNorthOuest = new ProblemMap();
	ProblemMap* regionSouthEast = new ProblemMap();
	ProblemMap* regionSouthOuest = new ProblemMap();

	//Create a map that associates the ProblemMap with it's associated number region
	std::map<int, ProblemMap*> regionCorrespondance
	{
		{0, regionNorthOuest },
		{1, regionSouthOuest},
		{2, regionSouthEast},
		{3, regionNorthEast}
	};
	

	//Check if the station is in any of the imposed limits
	for (ProblemStation currentProblemStation : map)
	{
		for (int regionIndice = 0; regionIndice < NUMBER_REGION; regionIndice++)
		{
			if (breitling_constraints::isStationInMandatoryRegion(*currentProblemStation.getOriginalStation(), regionIndice))
			{
				regionCorrespondance[regionIndice]->emplace_back(currentProblemStation);
			}
		}
	}
	
	//We aadd the ProblemMap into the vector in the correct order
	regions.push_back(regionNorthOuest);
	regions.push_back(regionSouthOuest);
	regions.push_back(regionSouthEast);
	regions.push_back(regionNorthEast);
	

	return regions;
}


/**
*Initialize the path accordingly to the Dataset : 
* The starting station and the end staion is incuded in the dataset.
* We select a random station in each region to fulfill the regionds requierments. 
*/
void OptimisationSolver::initializePath(const std::vector<ProblemMap*> regions, ProblemStation* startingProblemStation, ProblemStation* endProblemStation)
{
	chemin.clear();
	srand((unsigned)time(NULL));
	chemin.emplace_back(startingProblemStation);



	int regionOfStartStation = 0;
	int regionOfEndStation = 0;

	ProblemStation* northEastProblemStation;
	ProblemStation* northWestProblemStation;
	ProblemStation* southEastProblemStation;
	ProblemStation* southWestProblemStation;
	
	for (int i = 0; i < NUMBER_REGION; i++)
	{
		for (ProblemStation u : *regions[i])
		{
			if (u.getOriginalStation()->getName() == endProblemStation->getOriginalStation()->getName())
			{
				regionOfEndStation = i;
			}
		}
	}

	std::cout << "Region of Start Station : " << regionOfStartStation << "Region of end station: " << regionOfEndStation << std::endl;

	//We select a random station from each region
	
	northEastProblemStation = new ProblemStation(regions[0]->at(rand() % regions[0]->size()));
	northWestProblemStation = new ProblemStation((regions[1]->at(rand() % regions[1]->size())));
	southEastProblemStation = new ProblemStation((regions[2]->at(rand() % regions[2]->size())));
	southWestProblemStation = new ProblemStation((regions[3]->at(rand() % regions[3]->size())));

	//Creation of a distance variable for each station
	timedistance_t * neDistance = new timedistance_t( geometry::distance(startingProblemStation->getLocation(), northEastProblemStation->getLocation()));
	timedistance_t * swDistance = new timedistance_t( geometry::distance(startingProblemStation->getLocation(), southWestProblemStation->getLocation()));
	timedistance_t* seDistance = new timedistance_t( geometry::distance(startingProblemStation->getLocation(), southEastProblemStation->getLocation()));
	timedistance_t* nwDistance = new timedistance_t( geometry::distance(startingProblemStation->getLocation(), northWestProblemStation->getLocation()));

	
	//A map with the distance as key so that we can get the station associated
	std::map<timedistance_t*, ProblemStation*> distanceProblemStation
	{
		{neDistance, northEastProblemStation},
		{swDistance, southWestProblemStation},
		{seDistance, southEastProblemStation},
		{nwDistance, northWestProblemStation},
	};

	//A map that connects the distance with the correct region
	std::map<int ,timedistance_t*> orderStation
	{
		{0,nwDistance},
		{1,swDistance},
		{2,seDistance},
		{3,neDistance},
	};

	
	//We compare the distance to find the closest point to the starting point
	
	//This vector contains a list of the calculated distances 
	std::vector<timedistance_t*> distanceVector;
	distanceVector.emplace_back(neDistance);
	distanceVector.emplace_back(swDistance);
	distanceVector.emplace_back(seDistance);
	distanceVector.emplace_back(nwDistance);

	//If the end problem station is already in a region, we do not have to add a random station in that specific region 
	if (regionOfEndStation > 0)
	{
		auto it = std::find(distanceVector.begin(), distanceVector.end(), orderStation[regionOfEndStation]);
		distanceVector.erase(it);
	}
	
	////We find the closest station to the last station added to the path
	while (!distanceVector.empty())
	{	
		
		std::list<timedistance_t> closestStation;
		for (timedistance_t* i : distanceVector)
		{
			closestStation.emplace_back(*i);
		}

		closestStation.sort();
		distanceVector = tools::SortTimeDistance(distanceVector);
	
		ProblemStation* closestProblemStation = distanceProblemStation[distanceVector.front()];

		//If the end station is a part of the region, we do not add it during the sorting but at the end
		if (endProblemStation->getOriginalStation()->getName() != distanceProblemStation[distanceVector.front()]->getOriginalStation()->getName())
		{
			chemin.emplace_back(distanceProblemStation[distanceVector.front()]);
		}
		
		/*std::cout << "The name of the addded station" << closestProblemStation->getName() << std::endl;*/

		//We calculate the distance of every station compared to the last added station
		*neDistance =  geometry::distance(closestProblemStation->getLocation(), northEastProblemStation->getLocation());
		*swDistance =  geometry::distance(closestProblemStation->getLocation(), southWestProblemStation->getLocation());
		*seDistance =  geometry::distance(closestProblemStation->getLocation(), southEastProblemStation->getLocation());
		*nwDistance =  geometry::distance(closestProblemStation->getLocation(), northWestProblemStation->getLocation());
		distanceVector.erase(distanceVector.begin());

	}
	
	chemin.emplace_back(endProblemStation);

}

static ProblemPath adaptProblemPath(const std::vector<const ProblemStation *> &path) {
    ProblemPath adapted;
    adapted.reserve(path.size());
    for(const ProblemStation *s : path)
        adapted.push_back(*s);
    return adapted;
}

ProblemPath OptimisationSolver::solveForPath(const ProblemMap& map, SolverRuntime *runtime)
{
	srand((unsigned)time(NULL));
	bool doablePath = false;
	bool pathLongEnough = false;
	std::vector<ProblemStation*> skipList{};
	ProblemStation startingProblemStation = map[m_dataset.departureStation];
	ProblemStation endProblemStation = map[m_dataset.targetStation];
	
	initializePath(seperateRegion(map), &startingProblemStation, &endProblemStation );
	addRefuelStationIfFirstStationUnreachable(map, chemin);

  //std::cout << std::endl << "Does if statisfy fuel usage = " << breitling_constraints::satisfiesFuelConstraints(m_dataset,chemin) << std::endl;
  while (!doablePath || !pathLongEnough && !runtime->userInterupted)
	{
      runtime->currentProgress = chemin.size() / 100.f;
			try
			{
				resetTestVariable();
				//interface_mock::writePathToFile(map, transformToPath(chemin), "out.svg");
				for (std::vector<const ProblemStation*>::iterator iteratorStation = chemin.begin(); iteratorStation != chemin.end() - 1 && chemin.size() < breitling_constraints::MINIMUM_STATION_COUNT /* && chemin.size() < 100 */; ++iteratorStation)
				{
					
            std::vector <ProblemStation*> selectionGroup = connectProblemStationsTogether(map, *iteratorStation[0], *iteratorStation[1], travel, false, &runtime->userInterupted);
						if (selectionGroup.empty())
						{
              findIntermidiateRefillableStation(map, chemin, travel, &runtime->userInterupted);
						}
						for (ProblemStation* groupProblemStation : selectionGroup)
						{
							iteratorStation = chemin.insert(iteratorStation + 1, groupProblemStation);
						}
						if (chemin.size() >= breitling_constraints::MINIMUM_STATION_COUNT)
						{
							pathLongEnough = true;
						}
				}
					
				if (findProblematicProblemStation(chemin, travel) != chemin.size())
				{
          findIntermidiateRefillableStation(map, chemin, travel, &runtime->userInterupted);
				}
					
			}
			catch (std::runtime_error error)
			{
				std::cout << error.what() << std::endl;
				chemin.clear();
				initializePath(seperateRegion(map), &startingProblemStation, &endProblemStation);
				addRefuelStationIfFirstStationUnreachable(map, chemin);
			}
			///
      if (breitling_constraints::satisfiesFuelConstraints(m_dataset,adaptProblemPath(chemin)) == true)
			{
				doablePath = true;
			}
			else
			{	
        findIntermidiateRefillableStation(map, chemin, travel, &runtime->userInterupted);

				if (chemin.size() >= breitling_constraints::MINIMUM_STATION_COUNT)
				{
				
					initializePath(seperateRegion(map), &startingProblemStation, &endProblemStation);
				}
				
			}
	}

	//fillProblemStationToLimit(map, chemin);
	
	
	//TestPath(chemin);
	
	//chemin.clear();
	//initializePath(seperateRegion(map), &startingProblemStation, &endProblemStation);
  return adaptProblemPath(chemin);
}





/**
* Searches for stations that can be reached with the current disponible fuel
* If refuable is equal to true, then a condition is placed on the search : We only select station that can be use to refuel the plane
*/

std::vector<ProblemStation> OptimisationSolver::RefuelableStation(const ProblemMap& map, const ProblemStation& centerProblemStation, travel_variables travel, bool refuable)
{
	
	std::vector<ProblemStation> reachablePoint;
	double quantityOfFuelLeft = m_dataset.planeFuelCapacity - travel.distanceSinceLastRefuel / m_dataset.planeSpeed * m_dataset.planeFuelUsage;
	//Max distance of with the current remaining fuel
	double maxDistanceOfTravel = quantityOfFuelLeft * m_dataset.planeFuelUsage / m_dataset.planeSpeed;

	//It is not necessary to look for reachble stations if the max distance that we can go is negatif 
	if (maxDistanceOfTravel > 0)
	{
    for (const ProblemStation &point : map)
		{

			timedistance_t distanceBetweenPoints = (geometry::distance(centerProblemStation.getLocation(), point.getLocation())) / m_dataset.planeSpeed;
			//If the distance between the two point is smaller than the maximum travel distance
			if (distanceBetweenPoints < maxDistanceOfTravel) 
			{
				daytime_t timeAtArrival = geometry::distance(centerProblemStation.getLocation(), point.getLocation()) / m_dataset.planeSpeed;
				//If there is a need for the station to be a station that can be used to refuel the plane
				if (refuable)
				{
					//We check that the station is a refuel point
					if (point.canBeUsedToFuel())
					{
						reachablePoint.push_back(point);
					}
				}
				else
				{
					//We add it simply to the list
					reachablePoint.push_back(point);
				}
			}

		}
	}


	return reachablePoint;
}


/*
* We select a station from all the reachble station from the a starting point and we give the each station a score. 
* The goal is to select the station that has the loest score 
*/

ProblemStation* OptimisationSolver::stationSelectionInReach(const ProblemMap& map, const ProblemStation& startProblemStation, const ProblemStation& destinationProblemStation, std::vector<ProblemStation*> skipList, travel_variables* travel, bool refuelable)
{
	//Iterate for each station in the reachable station to find witch station has the best closest to destinationProblemStation/farthest to startProblemStation ration. 
	ProblemStation* SelectedProblemStation{};
	bool ElementInPath = false;
    timedistance_t ratio = std::numeric_limits<timedistance_t>::max();
	std::vector<ProblemStation> reachableProblemStations{};
	Station* destinationStation = (Station*)destinationProblemStation.getOriginalStation();
	timedistance_t calculatedRatio;
	Location halfWayPoint = tools::findHalfWayCoordinates(startProblemStation.getLocation(), destinationProblemStation.getLocation());

	
	reachableProblemStations = RefuelableStation(map, startProblemStation, *travel,refuelable);
	
	//If there is no reachable station, there is no selection possible
	if (reachableProblemStations.empty())
	{
		return nullptr;
	}

	for (ProblemStation i : reachableProblemStations)
	{
		const Station* currentStation = i.getOriginalStation();
		timedistance_t calculatedRatio, midwayPoint;
		midwayPoint = (geometry::distance(destinationProblemStation.getLocation(), startProblemStation.getLocation()) / 2);
	
		calculatedRatio = geometry::distance(halfWayPoint, i.getLocation());
		
		if (geometry::distance(i.getLocation(), startProblemStation.getLocation()) + travel->distanceSinceLastRefuel / m_dataset.planeSpeed * m_dataset.planeFuelUsage > 0)
		{
			travel_variables trajet = *travel;
			trajet.distanceSinceLastRefuel = +geometry::distance(i.getLocation(), startProblemStation.getLocation());
			float remainingFuel = m_dataset.planeFuelCapacity - trajet.distanceSinceLastRefuel / m_dataset.planeSpeed * m_dataset.planeFuelUsage;
			if (remainingFuel > 0)
			{
				if (tools::ProblemStationInProblemStationVector(skipList, i) == false && currentStation->getName() == destinationStation->getName())
				{
					SelectedProblemStation = new ProblemStation(i);

					return SelectedProblemStation;
				} 

				if (calculatedRatio < ratio && tools::ProblemStationInPath(chemin, i) == false && tools::ProblemStationInProblemStationVector(skipList, i) == false)
				{
					SelectedProblemStation = new ProblemStation(i);
					ratio = calculatedRatio;
					//std::cout << SelectedProblemStation->getName() << std::endl;
				}
				ElementInPath = false;
			}
			
		}

	

	}

	//std::cout << SelectedProblemStation->getName() << std::endl;
	return SelectedProblemStation;
}

/*
Reset the travel variable
*/
void OptimisationSolver::resetTestVariable()
{
	travel->flightDistance = 0;
	travel->distanceSinceLastRefuel = 0;
	travel->currentTime = m_dataset.departureTime;
}

/*
Finds the station which cannot be reached because the remaining fuel isn't sufficient and add a station that can be used to refuel the plane before the problematic station
*/
void OptimisationSolver::findIntermidiateRefillableStation(const ProblemMap& map, const std::vector<const ProblemStation*> path, travel_variables * travel, bool *stopFlag)
{
	bool noMoreProblem = false;
	travel_variables* voyage = travel;
	do
	{
		//Pointe the iterator to the problematic station
		std::vector< const ProblemStation*>::iterator stationIterator = chemin.begin() + findProblematicProblemStation(chemin, voyage);
		//If the iterator points to the last station in the path, then there is no problem detected so there is no need to correct the station
		if (stationIterator != chemin.end())
		{



			do
			{
				//We add a station before the problematic station : 
				//if there is no station that the plane can fly to with the current disponible fuel, than we need to go back again
				stationIterator = stationIterator - 1;
				updateTravelVariable(voyage, *stationIterator);
			

			} while (RefuelableStation(map, *stationIterator[0], *voyage,true).empty());


      std::vector<ProblemStation* > groupOfProblemStation = connectProblemStationsTogether(map, *stationIterator[0], *stationIterator[1], voyage, true, stopFlag);
			for (ProblemStation* emergencyProblemStation : groupOfProblemStation)
			{
				stationIterator = chemin.insert(stationIterator + 1, emergencyProblemStation);

			}
			TestPath(chemin);
		}
		else
		{
			noMoreProblem = true;
		}



  } while (!noMoreProblem && !*stopFlag);

}

Path OptimisationSolver::transformToPath(std::vector<const ProblemStation*> path)
{
	Path trajet{};

	for (const ProblemStation *iterator : path)
	{
		trajet.getStations().emplace_back(iterator->getOriginalStation());
	}
	return trajet;
}

//A special problem can be face when we add a random station to the path: 
//The distance between the start station and the first station may not be reachable by the plan so we add a station that can be used as a refuel station to connect the starting station and the first station
void OptimisationSolver::addRefuelStationIfFirstStationUnreachable(const ProblemMap& map, std::vector<const ProblemStation*> path)
{
	travel_variables voyage;
	double quantityOfFuelLeft =0;
	//Max travel time with the current remaining fuel
	double maxTimeTravel =0;
	std::vector<ProblemStation*> skip{};

	do
	{
		updateTravelVariable(&voyage, path[1]);
		quantityOfFuelLeft = m_dataset.planeFuelCapacity - voyage.distanceSinceLastRefuel / m_dataset.planeSpeed * m_dataset.planeFuelUsage;
		maxTimeTravel = quantityOfFuelLeft * m_dataset.planeFuelUsage / m_dataset.planeSpeed;
		if (maxTimeTravel < 0)
		{
			path.insert( path.begin()+1, stationSelectionInReach(map, *path[0], *path[1], skip, &voyage, true) );
		}

	}while(maxTimeTravel < 0);
	
}





//Update the travel variable up to the selected point in the 
void OptimisationSolver::updateTravelVariable(travel_variables* travel, const ProblemStation* point)
{
	bool reachedSelectedProblemStation = false;
	travel->flightDistance = 0;
	for (std::vector<const ProblemStation*>::iterator iteratorProblemStation = chemin.begin() + 1; iteratorProblemStation != chemin.end() && !reachedSelectedProblemStation; ++iteratorProblemStation)
	{
		travel->flightDistance = geometry::distance(iteratorProblemStation[-1]->getLocation(), iteratorProblemStation[0]->getLocation());
		travel->currentTime = m_dataset.departureTime + travel->flightDistance / m_dataset.planeSpeed;
		travel->distanceSinceLastRefuel =+geometry::distance(iteratorProblemStation[-1]->getLocation(), iteratorProblemStation[0]->getLocation());
		if (iteratorProblemStation[0]->canBeUsedToFuel())
		{
			travel->distanceSinceLastRefuel = 0;
		}
		if (point->getOriginalStation()->getName() == iteratorProblemStation[0]->getOriginalStation()->getName())
		{
			reachedSelectedProblemStation = true;
		}

	}
	travel->currentTime = m_dataset.departureTime + travel->flightDistance / m_dataset.planeSpeed;
}

std::vector<ProblemStation*> OptimisationSolver::connectProblemStationsTogether(const ProblemMap& map, const ProblemStation& startProblemStation, const ProblemStation& endProblemStation, travel_variables* travel, bool refuelable, bool *stopFlag)
{
	
	std::vector<ProblemStation*> resultProblemStation;
	std::vector<ProblemStation*> skipList;
	ProblemStation *SelectedProblemStation{};
	ProblemStation *currentProblemStation = (ProblemStation*)&startProblemStation;
	const Station* endStation = endProblemStation.getOriginalStation();
	bool elementAdded = false;
	bool firstElementAdded = false;
	
	
	// At everypassage, we add a new station even if the next station is in reach
	//This operation make it so that at least, one station is added at each passage 
	skipList.emplace_back(new ProblemStation(endProblemStation));
	
	do
	{
		//Security mesure : If the remaining fuel is lower than FUEL_SECURITY_PERCENTAGE, the next station need to be a refuelable station
		if (m_dataset.planeFuelCapacity - travel->distanceSinceLastRefuel / m_dataset.planeSpeed * m_dataset.planeFuelUsage < m_dataset.planeFuelCapacity * FUEL_SECURITY_PERCENTAGE)
		{
			refuelable = true;
		}
		//If there is no station selected, than this route isn't vailable so we return an empty vector
		SelectedProblemStation = stationSelectionInReach(map, *currentProblemStation, endProblemStation, skipList, travel, refuelable);
		if (nullptr == SelectedProblemStation)
		{
			return std::vector<ProblemStation*>();
		}
		
		nauticmiles_t currentDistance = geometry::distance(currentProblemStation->getLocation(), SelectedProblemStation->getLocation());
		travel->flightDistance = travel->flightDistance  + currentDistance;
		travel->currentTime = m_dataset.departureTime + travel->flightDistance / m_dataset.planeSpeed;
		if (!SelectedProblemStation->canBeUsedToFuel())
		{
			travel->distanceSinceLastRefuel = travel->distanceSinceLastRefuel + currentDistance;
		}
		else
		{

			travel->distanceSinceLastRefuel = 0;
		}
		float remainingFuel = m_dataset.planeFuelCapacity - travel->distanceSinceLastRefuel / m_dataset.planeSpeed * m_dataset.planeFuelUsage;
		//If the remaining fuel is superior to 0, that the selected station can be reached so we add the station to the connection vector between the 2 points in the path
		if (remainingFuel > 0)
		{
		
			elementAdded = true;
			firstElementAdded = true;
			//Station added to the connexion station
			resultProblemStation.emplace_back(SelectedProblemStation);
			skipList.emplace_back(SelectedProblemStation);
			
		

		}
		else
		{
			//If the fuel left in the plane isn't enough to cover the distance, we search for an other station that will permit us to do the route. We reset our travel variable
			travel->flightDistance = travel->flightDistance - currentDistance;
			travel->currentTime = m_dataset.departureTime + travel->flightDistance / m_dataset.planeSpeed;
			if (!SelectedProblemStation->canBeUsedToFuel())
			{
				travel->distanceSinceLastRefuel = travel->distanceSinceLastRefuel - currentDistance;
			}
			skipList.emplace_back(SelectedProblemStation);
			elementAdded = false;
		}
		
		//If the element is valid, we can add it to the connexion vecor
		if (elementAdded == true && firstElementAdded)
		{
			refuelable = false;
			skipList.clear();
			currentProblemStation = SelectedProblemStation;
			elementAdded = false;
			refuelable = false;
			
		}
	
	
		

	} while (endStation->getName() != currentProblemStation->getOriginalStation()->getName()); // This loop is to be done until the selected station is the destination station
	
	//The last station added is always the destination station so  we need to remove it because it is already in the path
	resultProblemStation.pop_back();
	
	
	return resultProblemStation;
}

//Find the station in a path where the remaining fuel is < 0. In this situation, it means that this station cannot be reached. If there is no problematic station, the method return le number associated to the last station with corrrespond to the size of the path.
//If there is a problematic station, this method return its index and update the travel variable.
size_t  OptimisationSolver::findProblematicProblemStation(std::vector<const ProblemStation*> path, travel_variables* travel)
{

	nauticmiles_t currentDistance = 0;
	nauticmiles_t distanceSinceLastRefuel = 0;
	for (size_t i = 1; i < chemin.size(); i++) {
		const ProblemStation* station = chemin[i];
		nauticmiles_t flightDistance = geometry::distance(chemin[i - 1]->getLocation(), station->getLocation());
		currentDistance += flightDistance;
		distanceSinceLastRefuel += flightDistance;
		daytime_t currentTime = m_dataset.departureTime + currentDistance / m_dataset.planeSpeed;
		//std::cout << std::endl << "ProblemStation number " << i << "  : " << distanceSinceLastRefuel;
		if (station->canBeUsedToFuel())
		{
			distanceSinceLastRefuel = 0;
		}else
		{
			float remainingFuel = m_dataset.planeFuelCapacity - distanceSinceLastRefuel / m_dataset.planeSpeed * m_dataset.planeFuelUsage;
			if (remainingFuel < 0)
			{
				travel->distanceSinceLastRefuel = distanceSinceLastRefuel;
				travel->flightDistance = currentDistance;
				travel->currentTime = currentTime;
				return i;
			}
			//std::cout << std::endl << "    Fuel left  " << remainingFuel << std::endl;
		}
		
		
	}

	return chemin.size();
}


//Check if a station is in the vector
bool tools::ProblemStationInPath(std::vector<const ProblemStation*> path, ProblemStation& station)
{
	for (const ProblemStation* i : path)
	{
		if (i->getOriginalStation()->getName() == station.getOriginalStation()->getName())
		{
			return true;
		}
	}
	return false;
}

//Check if a station is in the vector
bool tools::ProblemStationInProblemStationVector(std::vector<ProblemStation*>stationVector, ProblemStation& station)
{
	if (stationVector.empty())
	{
		return false;
	}
	for (const ProblemStation* i : stationVector)
	{
		if (i->getOriginalStation()->getName() == station.getOriginalStation()->getName())
		{
			return true;
		}
	}
	return false;
}

//Check if the end station is in the vector
bool tools::EndProblemStationInReachableProblemStations(std::vector<ProblemStation*> stations, ProblemStation& endProblemStation)
{
	
	return ProblemStationInProblemStationVector(stations,endProblemStation);
}

//Sort in the natural order a timedistance_t* vector: 
//This function is specificly designed for a map with 4 regions or less
std::vector<timedistance_t*> tools::SortTimeDistance(std::vector<timedistance_t*> distanceList)
{
	std::vector<timedistance_t> rankDistance;
	std::vector<timedistance_t*> resultVector;
	for (timedistance_t* i : distanceList)
	{
		rankDistance.emplace_back(*i);
	}

	std::sort(rankDistance.begin(), rankDistance.end());


	if (distanceList.size() == 4)
	{
		std::map<timedistance_t, timedistance_t*> closestDistance
		{
			{*distanceList[0],distanceList[0]},
			{*distanceList[1],distanceList[1]},
			{*distanceList[2],distanceList[2]},
			{*distanceList[3],distanceList[3]},
		};
		for (timedistance_t currentDistance : rankDistance)
		{
			resultVector.emplace_back(closestDistance[currentDistance]);
		}
	}
	else if(distanceList.size() == 3)
	{
		std::map<timedistance_t, timedistance_t*> closestDistance
		{
			{*distanceList[0],distanceList[0]},
			{*distanceList[1],distanceList[1]},
			{*distanceList[2],distanceList[2]},

		};
		for (timedistance_t currentDistance : rankDistance)
		{
			resultVector.emplace_back(closestDistance[currentDistance]);
		}
	}
	else if (distanceList.size() == 2)
	{
		std::map<timedistance_t, timedistance_t*> closestDistance
		{
			{*distanceList[0],distanceList[0]},
			{*distanceList[1],distanceList[1]},

		};
		for (timedistance_t currentDistance : rankDistance)
		{
			resultVector.emplace_back(closestDistance[currentDistance]);
		}
	}
	else if (distanceList.size() == 1)
	{
		std::map<timedistance_t, timedistance_t*> closestDistance
		{
			{*distanceList[0],distanceList[0]},
		};
		for (timedistance_t currentDistance : rankDistance)
		{
			resultVector.emplace_back(closestDistance[currentDistance]);
		}
	}
	
	return resultVector;
}

Location tools::findHalfWayCoordinates(Location start, Location end)
{
	return Location((start.longitude + end.longitude)/2,(start.latitude + end.latitude)/2);
}

//Test a path to seen the remaining fuel at each station on the console
void OptimisationSolver::TestPath(std::vector<const ProblemStation*>& chemin)
{
	std::cout << std::endl;
	nauticmiles_t currentDistance = 0;
	nauticmiles_t distanceSinceLastRefuel = 0;
	for (size_t i = 1; i < chemin.size(); i++) {
		const ProblemStation* station = chemin[i];
		nauticmiles_t flightDistance = geometry::distance(chemin[i - 1]->getLocation(), station->getLocation());
		currentDistance += flightDistance;
		distanceSinceLastRefuel += flightDistance;
		daytime_t currentTime = m_dataset.departureTime + currentDistance / m_dataset.planeSpeed;
		std::cout << "ProblemStation number (" << station->getOriginalStation()->getName() << ") " << i << "  : " << distanceSinceLastRefuel;
		if (station->canBeUsedToFuel()) {
			distanceSinceLastRefuel = 0;
		}
		float remainingFuel = m_dataset.planeFuelCapacity - distanceSinceLastRefuel / m_dataset.planeSpeed * m_dataset.planeFuelUsage;
		std::cout << std::endl << "    Fuel left  " << remainingFuel << std::endl;
	}

	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;
}

 
