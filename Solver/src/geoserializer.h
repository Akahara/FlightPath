#pragma once

#include <filesystem>

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



static std::map<std::string, Station::NightVFR> vfr{
    {"NO", Station::NightVFR::NO},
    {"YES", Station::NightVFR::YES},
    {"LIMITED", Station::NightVFR::LIMITED},
    {"PCL", Station::NightVFR::PCL},
    {"UNKNOWN", Station::NightVFR::UNKNOWN},
};

static std::map<std::string, Station::Fuel> fuel{
    {"NO", Station::Fuel::NO},
    {"YES", Station::Fuel::YES },
    {"AUTOMAT", Station::Fuel::AUTOMAT},
    {"UNKNOWN", Station::Fuel::UNKNOWN},
};

static std::map<Station::NightVFR, std::string> reverse_VFR
{
    {Station::NightVFR::NO,"NO", },
    { Station::NightVFR::YES,"YES",},
    { Station::NightVFR::LIMITED,"LIMITED",},
    {Station::NightVFR::PCL,"PCL" },
    { Station::NightVFR::UNKNOWN,"UNKNOWN"},
};

static std::map<Station::Fuel, std::string> reverse_FUEL{
    {Station::Fuel::NO,"NO" },
    {Station::Fuel::YES,"YES" },
    {Station::Fuel::AUTOMAT,"AUTOMAT"},
    {Station::Fuel::UNKNOWN,"UNKNOWN"},
};


class GeoSerializer {
public:
    virtual GeoMap parseMap(const std::filesystem::path& file) const = 0;
    virtual void writePath(const std::filesystem::path& file, const Path& path) const = 0;

protected:
    const double string2coordinate(const std::string& str) const;
};