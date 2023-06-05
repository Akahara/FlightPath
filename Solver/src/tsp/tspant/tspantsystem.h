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

	// on d�roule l'ex�cution sur n it�rations
	void run(int n);

	int pathCount;

private:
	// fourmis g�r�es
	std::list<TspAnt*> ants;

	// donn�es du probl�me
	Problem& data;

	// meilleure solution trouv�e
	nauticmiles_t bestLenght;
	std::vector<Station> bestSolution;//Liste des stations dans le meilleur parcours trouv� jusqu'� lors

	int curIteration;

	void notifySolution(nauticmiles_t value, std::vector<Station>& stationList);

};

#endif
