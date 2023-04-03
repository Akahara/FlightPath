#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include "../geoserializer.h"
#include <map>


class CSVSerializer : public GeoSerializer {
public:
    /**
     * @brief Parse Map : Lire les informations contenus dans un fichier
     * @param file : Le fichier à traiter
     * @return Un objet GeoMap qui correspond à un vector contenant toutes les stations
    */
    GeoMap parseMap(const std::filesystem::path &file) const override;
    /**
     * @brief Write path : Ecrire un chemin dans un fichier csv
     * @param file : Le fichier 
     * @param path : Le chemin à écrire
    */
    void writePath(const std::filesystem::path &file, const Path &path) const override;
};