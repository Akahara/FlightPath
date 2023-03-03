#include "geoserializer.h"

const double GeoSerializer::string2coordinate(const std::string &str) const
{
    // split string into degrees, minutes and direction  (example: 52:30.12N)
    std::string degrees = str.substr(0, str.find(":"));   // degrees = 52
    std::string minutes = str.substr(str.find(":") + 1);     // minutes = 30.12N
    std::string direction = minutes.substr(minutes.size() - 1); // direction = N
    minutes.pop_back();                                              // minutes = 30.12

    // convert to decimal degrees
    double result = std::stod(degrees) + std::stod(minutes) / 60;

    // change sign to keep a consistent coordinate system
    if (direction == "W" | direction == "O" | direction == "S") {
        result = -result;
    }

    return result;
}