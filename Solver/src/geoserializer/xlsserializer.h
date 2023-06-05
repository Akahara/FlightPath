#pragma once

#include "../geoserializer.h"
#include "../geometry.h"

class XLSSerializer : public GeoSerializer {
public:
    GeoMap parseMap(const std::filesystem::path &file) const override;
    void writePath(const std::filesystem::path &file, const Path &path) const override;
};