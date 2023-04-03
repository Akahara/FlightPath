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

    //On vide enti�rement les vectors
    row.clear();
    content.clear();

    //Sauter la premi�re ligne : cette ligne contient seulement l'ent�te des donn�es 
    getline(csvFile, infoLine);

    //Recuperation des donn�es sous forme de vectors:  
    while (std::getline(csvFile, infoLine))
    {
        std::stringstream sLine(infoLine);
        row.clear();
        //On s�pare chaque ligne gr�ce au s�parateur ";" 



        while (std::getline(sLine, word, ';'))
        {
            row.push_back(word);

        }
        //On ajoute le vector row au vector content
        content.push_back(row);
    }

    
    //Cr�ation des stations a partir des locations  
    for (int i = 0; i < content.size(); i++)
    {
        if (content[i][EXCLUDE_COLUMN-1].empty())
        {
            Location loc{ string2coordinate(content[i][LONGITUDE_COLUMN - 1]),string2coordinate(content[i][LATITUDE_COLUMN - 1]) };
            Station s(loc, content[i][NAME_COLUMN - 1], content[i][OACI_COLUMN - 1], content[i][STATUS_COLUMN - 1], vfr[content[i][NIGHT_VFR_COLUMN - 1]], fuel[content[i][FUEL_COLUMN - 1]]);
            //On ajoute ses stations dans le GeoMap r�sultat
            resultat.getStations().emplace_back(s);
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
        csvFile << reverse_VFR.at(i->getNightVFR()) << ";";
        csvFile << reverse_FUEL.at(i->getFuel()) << ";\n";
        
            
    }

    csvFile.close();

}
