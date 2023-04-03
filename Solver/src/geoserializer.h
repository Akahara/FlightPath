#pragma once

#include <filesystem>
#include <regex>

#include "geomap.h"
#include "path.h"
#include <map>

#define EXCLUDE_COLUMN 1
#define OACI_COLUMN 2
#define NAME_COLUMN 3
#define LATITUDE_COLUMN 4
#define LONGITUDE_COLUMN 5
#define STATUS_COLUMN 6
#define NIGHT_VFR_COLUMN 7
#define FUEL_COLUMN 8


static std::map<std::string, Station::NightVFR> NightVFR_fromString
{
    {"NON", Station::NightVFR::NO},
    {"OUI", Station::NightVFR::YES},
    {"LIMITE", Station::NightVFR::LIMITED},
    {"PCL", Station::NightVFR::PCL}
};

static std::map<std::string, Station::Fuel> Fuel_fromString
{
    {"NON", Station::Fuel::NO},
    {"OUI", Station::Fuel::YES},
    {"AUTOMAT", Station::Fuel::AUTOMAT}
};

static std::map<Station::NightVFR, std::string> VFR_toString
{
    {Station::NightVFR::NO,"NON"},
    {Station::NightVFR::YES,"OUI"},
    {Station::NightVFR::LIMITED,"LIMITE"},
    {Station::NightVFR::PCL,"PCL"}
};

static std::map<Station::Fuel, std::string> Fuel_toString
{
    {Station::Fuel::NO,"NON"},
    {Station::Fuel::YES,"OUI"},
    {Station::Fuel::AUTOMAT,"AUTOMAT"}
};

class GeoSerializer {
public:
    virtual GeoMap parseMap(const std::filesystem::path &file) const = 0;
    virtual void writePath(const std::filesystem::path &file, const Path &path) const = 0;

protected:
    static double string2coordinate(const std::string &str);
};