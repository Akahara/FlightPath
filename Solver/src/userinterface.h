#pragma once

#include <filesystem>

#include "geomap.h"
#include "path.h"

class UserInterface {
private:
    // TODO design UserInterface
};

namespace interface_mock {

    void writePathToFile(const GeoMap &geomap, const Path &path, const std::filesystem::path &file);
    void writePathToKML(const GeoMap &geomap, const Path &path, const std::filesystem::path &file);
}