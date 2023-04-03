#include <sstream>
#include "csvserializer.h"

GeoMap CSVSerializer::parseMap(const std::filesystem::path &file) const
{
    GeoMap resultat{};

    std::vector<std::vector<std::string>> content;
    std::vector<std::string> row;
    std::string infoLine, word;

    //Ouverture du fichier csv
    std::ifstream csvFile;
    csvFile.open(file, std::ios_base::in);
    if (!csvFile.is_open())
    {
        std::cout << "error";
        return GeoMap();
    }

    //On vide entièrement les vectors
    row.clear();
    content.clear();

    //Sauter la première ligne : cette ligne contient seulement l'entête des données
    getline(csvFile, infoLine);

    //Recuperation des donn�es sous forme de vectors:  
    while (std::getline(csvFile, infoLine))
    {
        std::stringstream sLine(infoLine);
        row.clear();

        //On sépare chaque ligne grace au séparateur ";"
        while (std::getline(sLine, word, ';'))
        {
            row.push_back(word);

        }
        //On ajoute le vector row au vector content
        content.push_back(row);
    }
    
    //Création des stations a partir des locations
    for (int i = 0; i < content.size(); i++)
    {
        if (content[i][EXCLUDE_COLUMN-1].empty())
        {
            std::string OACI, name, lat, lon, status, nightVFR, fuel;

            OACI = content[i][OACI_COLUMN - 1];
            name = content[i][NAME_COLUMN - 1];
            lat = content[i][LATITUDE_COLUMN - 1];
            lon = content[i][LONGITUDE_COLUMN - 1];
            status = content[i][STATUS_COLUMN - 1];
            nightVFR = content[i][NIGHT_VFR_COLUMN - 1];
            fuel = content[i][FUEL_COLUMN - 1];

            if (fuel.back() == '\r') {
                fuel.pop_back(); // remove the last character (\r)
            }

            Location location {string2coordinate(lon), string2coordinate(lat)};
            Station station (location, name, OACI, status, nightVFR, fuel);

            //On ajoute ses stations dans le GeoMap résultat
            resultat.getStations().emplace_back(station);
        }
    }

    return resultat;
}

void CSVSerializer::writePath(const std::filesystem::path &file, const Path &path) const
{
    std::ofstream csvFile;
    csvFile.open(file, std::ios::app);

    if (!csvFile.is_open())
    {
        std::cout << "error";
        throw std::runtime_error("Erreur ouverture");
    }

    for (Station *i : path.getStations())
    {
        csvFile << i->getOACI() << ";";
        csvFile << i->getName() << ";";
        csvFile << i->getLocation().lat << ";";
        csvFile << i->getLocation().lon << ";";
        csvFile << i->getStatus() << ";";
        csvFile << i->getNightVFR() << ";";
        csvFile << i->getFuel() << ";\n";
    }

    csvFile.close();
}
