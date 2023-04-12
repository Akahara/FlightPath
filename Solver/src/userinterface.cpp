#include "userinterface.h"

#include <fstream>

void interface_mock::writePathToFile(const GeoMap &geomap, const Path &path, const std::filesystem::path &filePath)
{
  std::ofstream file{ filePath };

  double
    minLon = std::numeric_limits<double>::max(),
    minLat = std::numeric_limits<double>::max(),
    maxLon = std::numeric_limits<double>::min(),
    maxLat = std::numeric_limits<double>::min();
  for (const Station &station : geomap.getStations()) {
    double lon = station.getLocation().lon;
    double lat = station.getLocation().lat;
    minLon = std::min(minLon, lon);
    minLat = std::min(minLat, lat);
    maxLon = std::max(maxLon, lon);
    maxLat = std::max(maxLat, lat);
  }

  constexpr double padding = 1;
  file << "<svg viewBox=\"" 
    << (minLon - padding) << " "
    << (minLat - padding) << " "
    << (maxLon - minLon + 2 * padding) << " "
    << (maxLat - minLat + 2 * padding)
    << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";

  file << "<g transform=\"scale(.83, -1) translate(0, -92.5) \">\n";

  for (const Station &station : geomap.getStations()) {
    const char *color = "red"; // TODO base color on 
    file << "<circle cx=\"" << station.getLocation().lon << "\" cy=\"" << station.getLocation().lat << "\" r=\".12\" fill=\"" << color << "\"/>\n";
  }

  file << "<path d=\"M";
  for (const Station *station : path.getStations())
    file << station->getLocation().lon << " " << station->getLocation().lat << " ";
  file << "\" stroke-width=\".08\" stroke=\"blue\" fill=\"none\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n";

  file << "</g>\n";

  file << "</svg>" << std::endl;
}
