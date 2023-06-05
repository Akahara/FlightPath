#ifndef H__ant_system__
#define H__ant_system__

#include <list>
#include <vector>
#include "tspant.h"
//#include "../station.h"
#include "tspant.h"
#include "../path.h"

//class problem;
class TspAnt;

/**
 * moteur d'algorithme
 */
class antSystem {
public:
	antSystem(int  nbAnt, Problem&) :data(d);

	~antSystem();

	// on déroule l'exécution sur n itérations
	void run(int n);

	int pathCount;

private:
	// fourmis gérées
	std::list<TspAnt*> ants;

	// données du problème
	Problem& data;

	// meilleure solution trouvée
	nauticmiles_t bestLenght;
	std::vector<Station> bestSolution;//Liste des stations dans le meilleur parcours trouvé jusqu'à lors

	int curIteration;

	void notifySolution(nauticmiles_t value, std::vector<Station>& stationList);

};

#endif
