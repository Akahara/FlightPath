#pragma once

#include <filesystem>

#include "geomap.h"
#include "path.h"
#include "pathsolver.h"

namespace svg_export {

void writePathToFile(const ProblemMap &geomap, const ProblemPath &path, const std::filesystem::path &file);

}

namespace kml_export {

void writeHeader(std::ostream &out);
void writeFooter(std::ostream &out);
void writeAllStationsLayer(std::ostream &out, const ProblemMap &map);
void writeProblemStationsLayer(std::ostream &out, const ProblemMap &map);
void writePathLayer(std::ostream &out, const ProblemPath &path, const char *layerName);

}
