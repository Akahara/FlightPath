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

void writePathToFile(const ProblemMap &geomap, const Path &path, const std::filesystem::path &file);

}

namespace kml_export {

void writeHeader(std::ostream &out);
void writeFooter(std::ostream &out);
void writeAllStationsLayer(std::ostream &out, const GeoMap &map);
void writeProblemStationsLayer(std::ostream &out, const ProblemMap &map);
void writePathLayer(std::ostream &out, const Path &path, const char *layerName);

}