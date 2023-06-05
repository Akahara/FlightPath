#ifndef H__problem__
#define H__problem__

#include <vector>
#include "../geomap.h"
#include "../csvserializer.h"

/**
 * données du problème : distances + phéromones
 */

namespace utility {
	int findStationPositionInTheList(const std::vector<Station>& stations, const Station& station) {
		for (int i = 0; i < stations.size(); i++) {
			if (stations[i].getLocation() == station.getLocation()) {
				return i;
			}
		}
		return -1; // Si la station n'est pas trouvée
	}

}
class Problem {
public:
	Problem(int, float, float, float);
	Problem(const std::filesystem::path& file, float, float, float);

	void setPheromones(int, int, int);

	void evaporate();

	int nbStations;
	float borneMax, borneMin;
	float evaporation;

	nauticmiles_t optimalLength;

	// arcs
	// pheromones
	GeoMap allStations;
	//std::vector<std::vector<nauticmiles_t> > distances;//Distances entre les stations
	std::vector<std::vector<double> > pheromones;
};

#endif
