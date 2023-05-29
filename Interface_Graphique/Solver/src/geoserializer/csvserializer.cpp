#include <sstream>
#include "csvserializer.h"

GeoMap CSVSerializer::parseMap(const std::filesystem::path &file) const
{
    GeoMap resultat{};

    std::vector<std::vector<std::string>> content;
    std::vector<std::string> row;
    std::string infoLine, word;

    // Open the file
    std::ifstream csvFile;
    csvFile.open(file, std::ios_base::in);
    if (!csvFile.is_open())
    {
        throw std::runtime_error("Error while opening the file \"" + file.string() + "\"");
    }

    // Clear the vectors
    row.clear();
    content.clear();

    // Skip the header
    getline(csvFile, infoLine);

    // Fill the vectors with the content of the file
    while (std::getline(csvFile, infoLine))
    {
        std::stringstream sLine(infoLine);
        row.clear();

        // Split the line with the delimiter ';'
        while (std::getline(sLine, word, ';'))
        {
            row.push_back(word);

        }
        // Add the row vector to the content vector
        content.push_back(row);
    }
    
    // Create the stations
    for (int i = 0; i < content.size(); i++)
    {
        std::string OACI, name, lat, lon, status, nightVFR, fuel;

        OACI = content[i][OACI_COLUMN - 1];
        name = content[i][NAME_COLUMN - 1];
        lat = content[i][LATITUDE_COLUMN - 1];
        lon = content[i][LONGITUDE_COLUMN - 1];
        status = content[i][STATUS_COLUMN - 1];
        nightVFR = content[i][NIGHT_VFR_COLUMN - 1];
        fuel = content[i][FUEL_COLUMN - 1];

        bool excluded = false;
        if (!content[i][EXCLUDE_COLUMN-1].empty()) {
            excluded = true;
        }

        if (fuel.back() == '\r') {
            fuel.pop_back(); // remove the last character (\r)
        }

        double lat_value, lon_value;

        try {
            lat_value = string2coordinate(lat);
            lon_value = string2coordinate(lon);
        } catch (std::exception &e) {
            throw std::runtime_error("Error while parsing the file \""
                                     + file.string()
                                     + "\" at row "
                                     + std::to_string(i + 2)
                                     + ": " + e.what());
        }

        Location location{ lon_value, lat_value };
        Station station (excluded, location, name, OACI, status, nightVFR, fuel);

        // Add the station to the map
        resultat.getStations().emplace_back(station);
    }

    return resultat;
}

void CSVSerializer::writePath(const std::filesystem::path &file, const Path &path) const
{
    std::ofstream csvFile;
    csvFile.open(file, std::ios::app);

    if (!csvFile.is_open())
    {
        throw std::runtime_error("Error while opening the file \"" + file.string() + "\"");
    }

    for (const Station *i : path.getStations())
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
