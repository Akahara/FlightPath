#pragma once

#include <iostream>
#include <fstream>

#include "../geoserializer.h"

class CSVSerializer : public GeoSerializer {
public:
    GeoMap parseMap(const std::filesystem::path &file) const override;

    /**
     * @deprecated This method does not create a navigation sheet, use XLSXSerializer instead
     */
    //TODO : rename to writeMap when it will be implemented
    [[deprecated]]
    void writePath(const std::filesystem::path &file, const Path &path) const override;
};