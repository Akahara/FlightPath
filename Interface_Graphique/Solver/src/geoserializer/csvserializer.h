#pragma once

#include <iostream>
#include <fstream>

#include "../geoserializer.h"

class CSVSerializer : public GeoSerializer {
public:
    GeoMap parseMap(const std::filesystem::path &file) const override;

    void writePath(const std::filesystem::path &file, const Path &path) const override;

    void writeMap(const GeoMap &map, const std::filesystem::path &file) const override;
};
