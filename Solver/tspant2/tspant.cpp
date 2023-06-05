#include "tspant.h"

/*
constructeur
*/
TspAnt::TspAnt(Problem& data)
{
    tmpVisitedLength = 0;
    currentArcPos = -1;
    currentDestination =NULL;
    currentOrigin = NULL;
    state = NOTHING;

    for (int i = 0; i < data.nbStations; i++)
        stationsListToVisit.push_back(data.allStations[i]);
}



/*
* @param fromStation: la station courante
* @brief Determine la prochaine station 
*/
Station TspAnt::getNearStation(Station fromStation)
{
    double pheromoneSize = 0;
    int pos;
    for (int i = 0; i < int(stationsListToVisit.size()); i++) {
        if (stationsListToVisit[i] == fromStation)
            continue;
         pos = utility::findStationPositionInTheList(getStationsListToVisit, fromStation);
        pheromoneSize += data.pheromones[pos][i];
    }

    double found = float(rand() % int(pheromoneSize * 1000)) / double(1000);
    double tmpPheromones = 0;
    int ii = 0;
    while (ii < int(stationsListToVisit.size())) {
        if (stationsListToVisit[ii] == fromStation) {
            ii++;
            continue;
        }
        pos= utility::findStationPositionInTheList(getStationsListToVisit, currentDestination)
        tmpPheromones += data.pheromones[pos][ii];

        if (tmpPheromones > found)
            break;

        ii++;
    }
    if (ii == stationsListToVisit.size()) {
        // aucune solution acceptable, détruire la fourmi courante
        antException e;
        e.a = this;
        e.state = antException::TO_DELETE;
        throw e;
    }

    return stationsListToVisit[ii];
}


void TspAnt::findNextSearchDestination()
{
    switch (state) {
        // si la fourmi vient d'être créée
    case NOTHING: {
        visitedStationsList.push_back(stationsListToVisit[0]);
        std::vector<Station>::iterator tmp = stationsListToVisit.begin();
        while (tmp != stationsListToVisit.end()) {
            if (*tmp == stationsListToVisit[0]) {
                stationsListToVisit.erase(tmp);
                break;
            }
            tmp++;
        }

        Station dest = getNearStation(stationsListToVisit[0]);
        state = SEARCHING_PATH;
        currentOrigin = stationsListToVisit[0];
        currentDestination = dest;
        currentArcPos = 0;
        currentArcSize = geometry::distance(currentOrigin.getLocation(), currentDestination.getLocation()); //data.distances[0][currentDestination];

        break;
    }
                // si la fourmi cherche son chemin dans le graphe
    case SEARCHING_PATH: {
        // on a atteint currentDestination           
        tmpVisitedLength += geometry::distance(currentOrigin.getLocation(), currentDestination.getLocation());//data.distances[currentOrigin][currentDestination];
        visitedStationsList.push_back(currentDestination);

        std::vector<Station>::iterator tmp = stationsListToVisit.begin();
        while (tmp != stationsListToVisit.end()) {
            if (*tmp == currentDestination) {
                stationsListToVisit.erase(tmp);
                break;
            }
            tmp++;
        }

        if (stationsListToVisit.size() == 0) {
            // plus rien à visiter, le chemin est complet
            // on revient vers le nid
            tmpVisitedLength +=geometry::distance(currentDestination.getLocation(), data.distances[currentDestination][0]);

            state = RETURNING;
            currentOrigin = visitedStationsList[visitedStationsList.size() - 1];
            currentDestination = visitedStationsList[visitedStationsList.size() - 2];
            currentArcSize = geometry::distance(currentOrigin.getLocation(), currentDestination.getLocation());// data.distances[visitedStationsList[currentOrigin]][visitedStationsList[currentDestination]];
            currentArcPos = currentArcSize;
            return;
        }

        Station dest = getNearStation(currentDestination);
        currentOrigin = currentDestination;
        currentDestination = dest;
        currentArcSize = geometry::distance(currentOrigin.getLocation(), currentDestination.getLocation());// data.distances[currentOrigin][currentDestination];
        currentArcPos = 0;
        break;
    }
                       // si la fourmi revient au nid
    case RETURNING: {
        int pos1, pos2;
        if (currentDestination == stationsListToVisit[0]) {
            // retourné au nid avec succès
           
            pos1 = utility::findStationPositionInTheList(visitedStationsList, currentOrigin);
            pos2 = utility::findStationPositionInTheList(visitedStationsList, currentDestination);

            
            data.setPheromones(pos1, pos2, tmpVisitedLength);

            // sauver le résultat, changer de fourmi
            antException e;
            e.a = this;
            e.state = antException::TO_REGISTER;
            throw e;
        }

        // trouver la ville précédemment visitée et la passer en destination
        // mettre des phéromones sur l'arc parcouru
        pos1 = utility::findStationPositionInTheList(visitedStationsList, currentOrigin);
        pos2 = utility::findStationPositionInTheList(visitedStationsList, currentDestination);
        data.setPheromones(pos1, pos2, tmpVisitedLength);
        currentOrigin = currentDestination;
        currentDestination = currentOrigin //- 1;
        currentArcSize = geometry::distance(currentOrigin.getLocation(), currentDestination.getLocation());// data.distances[currentOrigin.getLocation()]][currentDestination.getLocation()];
        currentArcPos = currentArcSize;

        break;
    }
    }
}

void TspAnt::frame()
{
    switch (state) {
    case SEARCHING_PATH:
        tmpVisitedLength++;
    case: RETURNING:
        currentArcPos++;
        if (currentArcPos >= currentArcSize)
            findNextSearchDestination();
        break;
    case NOTHING:
        findNextSearchDestination();
        break;
    }
}