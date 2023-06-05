#include "problem.h"
#include <cstdlib>

Problem::Problem(int nb_stations, float borne1, float borne2, float coeff)
	: nbStations(nb_stations), borneMax(borne1), borneMin(borne2), evaporation(coeff),
	distances(nbStations, std::vector<int>(nbStations, 0)),
	pheromones(nbStations, std::vector<double>(nbStations, borneMin))
{


	/**for (int i = 0; i < nbStations; i++) {
		distances[i][i] = 0;
		for (int j = i + 1; j < nbStations; j++)
			distances[i][j] = distances[j][i] = rand() % 100;
	}**/

	// solution optimale
	for (int i = 0; i < nbStations; i++)
		distances[i][(i + 1) % nbStations] = distances[(i + 1) % nbStations][i] = 1;

	//optimalLength = nbStations;
}

Problem::Problem(const std::filesystem::path& file,float bornMin, float bornMax, float coeff)
{
	pheromones(nbStations, std::vector<double>(nbStations, bornMin);
	evaporation = coeff;
	borneMin = bornMin;
	borneMax = bornMax;
	allStations = CSVSerializer::parseMap(file);
	nbStations = allStations.getStations().size();
}

void Problem::setPheromones(int i, int j, int wayLength) {
	double ph = 100.f * optimalLength / (wayLength + 1 - optimalLength);

	pheromones[i][j] += ph;

	if (pheromones[i][j] < borneMin) pheromones[i][j] = borneMin;
	if (pheromones[i][j] > borneMax) pheromones[i][j] = borneMax;

	pheromones[j][i] = pheromones[i][j];
}

void Problem::evaporate() {
	for (int i = 0; i < nbStations; i++)
		for (int j = 0; j < i; j++) {
			pheromones[i][j] = pheromones[i][j] * (100 - evaporation) / 100;
			if (pheromones[i][j] < borneMin)
				pheromones[i][j] = borneMin;

			pheromones[j][i] = pheromones[i][j];
		}
}

// int nbStations;
// int borneMax = 500, borneMin = 2;
// evaporation = 1%

// optimalLength