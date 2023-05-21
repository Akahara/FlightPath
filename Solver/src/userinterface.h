#pragma once

#include <filesystem>

#include "geomap.h"
#include "path.h"
#include "pathsolver.h"

class UserInterface {
private:
    // TODO design UserInterface
};

namespace interface_mock {

    void writePathToFile(const ProblemMap &geomap, const ProblemPath &path, const std::filesystem::path &file);
    void writePathToKML(const ProblemMap &geomap, const ProblemPath &path, const std::filesystem::path &file);
}