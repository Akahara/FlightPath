#pragma once

#include <filesystem>
#include <regex>

#include "geomap.h"
#include "path.h"

#define EXCLUDE_COLUMN 1
#define OACI_COLUMN 2
#define NAME_COLUMN 3
#define LATITUDE_COLUMN 4
#define LONGITUDE_COLUMN 5
#define STATUS_COLUMN 6
#define NIGHT_VFR_COLUMN 7
#define FUEL_COLUMN 8

/**
 * @brief Abstract class for serializing/deserializing GeoMap and Path objects. \n\n
 * Classes inheriting from GeoSerializer must implement the parseMap() and writePath() methods. \n\n
 * This class also contains a static method for converting a string to a coordinate value.
 */
class GeoSerializer {
public:

    /**
    * @brief Parses a file containing GeoMap data and returns a corresponding GeoMap object.
    * @param file The path to the file containing the GeoMap data.
    * @return The parsed GeoMap object.
    */
    virtual GeoMap parseMap(const std::filesystem::path &file) const = 0;

    /**
     * @brief Writes a Path object to a file (Navigation sheet).
     * @param file The path to the file to write to.
     * @param path The Path object to write.
     */
    virtual void writePath(const std::filesystem::path &file, const Path &path) const = 0;

    // TODO : Add a method for writing a GeoMap object to a file (Map sheet)

protected:

    /**
     * @brief Converts a string to a coordinate value.
     * @param str The string to convert.
     * @return The coordinate value.
     * @throws std::invalid_argument if the string is not a valid coordinate.
     */
    static double string2coordinate(const std::string &str);
};