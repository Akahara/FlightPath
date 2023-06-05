#ifndef TSP_ANT
#define TSP_ANT

#include "../TspSolver.h"
#include <vector>
#include "problem.h"
#include "tspantexception.h"
#include "../geometry.h"

using std::vector;
class TspAnt : public PathSolver
{
private:
    vector<Station> visitedStationsList; //Liste des stations visitees
    vector<Station> stationsListToVisit;//Stations a visiter
protected:
    Problem& data;
public:
    nauticmiles_t tmpVisitedLength;//compteur de longueur du chemin parcouru
    TspAnt(Problem& data);
    ~TspAnt() {};
    virtual Path solveForPath(const GeoMap& map) override;
    vector<Station> getVisitedStationsList() { return visitedStationsList; }//getters de la liste des stations visitees
    vector<Station >getStationsListToVisit() { return stationsListToVisit; }//getters de la liste des stations a visiter
    //trouve la prochaine station a atteindre

    Station getNearStation(Station station);//Choix pond�r� d'une station


public:
    enum {
        SEARCHING_PATH, //fourmi � la recherche de son 1er noeud(station de depart connu)?
        RETURNING, //fourmi retournant vers le nid
        NOTHING, //la fourmi vient d'�tre cr��e
    };

    int state; // �tat de la fourmi, en route, en retour ..


protected:
    // donn�es de parcours locales
    nauticmiles_t currentArcSize;    // longueur de l'arc actuellement parcouru
    nauticmiles_t currentArcPos;    // position sur l'arc actuellement parcouru
    Station currentOrigin;        // premi�re extr�mit� de l'arc actuellement parcouru 
    Station currentDestination;    // seconde extr�mit� de l'arc actuellement parcouru
    void findNextSearchDestination();//d�termination de la prochaine station � atteindre

   
    void frame();//permet de faire �voluer la fourmie � chaque it�ration
};

#endif
