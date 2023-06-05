#include "tspantsystem.h"
#include "ant_system.h"
#include "problem.h"


#include <algorithm>
#include <iostream>


antSystem::antSystem(int nbAnt, problem& d) :data(d) {
	for (int i = 0; i < nbAnt; i++)
		ants.push_back(new TspAnt(data));

	bestLenght =0;
	pathCount = 0;
	curIteration = 0;
}

antSystem::~antSystem() {
	for (std::list<TspAnt*>::iterator i = ants.begin(); i != ants.end(); i++)
		delete* i;
}


void antSystem::run(int n) {
	for (curIteration = 0; curIteration < n; curIteration++) {
		// traitement de chaque fourmi
		std::list<TspAnt*>::iterator it = ants.begin();
		while (it != ants.end()) {
			try {
				(*it)->frame();
			}
			catch (antException& e) {
				if (e.state == antException::TO_REGISTER)
					notifySolution(e.a->tmpVisitedLength, e.a->visitedStationsList);

				if (bestLength <= data.optimalLength)
					return;

				*it = new ant(data);
				delete e.a;
			}
			it++;
		}

		// evaporation des pheromones
		if (curIteration % 20 == 0)
			data.evaporate();

		if (curIteration % 50 == 0)
			std::cout << bestLength << std::endl;
	}
}


void antSystem::notifySolution(nauticmiles_t value, std::vector<Station>& stationList) {
	if (bestLength == -1) {
		bestLength = value;
		bestSolution = stationList;
	}
	else {
		pathCount++;

		if (value < bestLength) {
			bestLength = value;
			bestSolution = stationList;

			/**
			*
			/
			//std::cout << curIteration << " " << bestLength << " ";
			/*for (int i=0; i<int(stationList.size()); i++)
				std::cout << stationList[i] << ",";
*/
//std::cout << std::endl;
		}
	}
}