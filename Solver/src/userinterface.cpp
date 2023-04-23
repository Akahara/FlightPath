#include "userinterface.h"

#include "breitling/breitlingsolver.h"

#include <fstream>

void interface_mock::writePathToFile(const ProblemMap &geomap, const Path &path, const std::filesystem::path &filePath)
{
  std::ofstream file{ filePath };

  double
    minLon = std::numeric_limits<double>::max(),
    minLat = std::numeric_limits<double>::max(),
    maxLon = std::numeric_limits<double>::min(),
    maxLat = std::numeric_limits<double>::min();
  for (const ProblemStation &station : geomap) {
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

  for (const ProblemStation &station : geomap) {
    // black - all good
    // pink - no fuel  no night
    // blue - no night
    // red - no fuel
    float red = station.canBeUsedToFuel() ? 0 : 1;
    float green = .1f;
    float blue = station.isAccessibleAtNight() ? 0 : 1;
    file
      << "<circle cx=\"" << station.getLocation().lon << "\" cy=\"" << station.getLocation().lat << "\" r=\".1\" "
      << "fill=\"rgb(" << red*255 << "," << green*255 << "," << blue*255 << ")\"/>\n";

    if (breitling_constraints::isStationInMandatoryRegion(*station.getOriginalStation(), 0))
      file
      << "<rect x=\"" << station.getLocation().lon - .1 << "\" y=\"" << station.getLocation().lat - .1 << "\" "
      << "stroke-width=\".05\" stroke=\"blue\" width = \".2\" height=\".2\" fill=\"transparent\" />";
    if (breitling_constraints::isStationInMandatoryRegion(*station.getOriginalStation(), 1))
      file
      << "<rect x=\"" << station.getLocation().lon - .1 << "\" y=\"" << station.getLocation().lat - .1 << "\" "
      << "stroke-width=\".05\" stroke=\"green\" width = \".2\" height=\".2\" fill=\"transparent\" />";
    if (breitling_constraints::isStationInMandatoryRegion(*station.getOriginalStation(), 2))
      file
      << "<rect x=\"" << station.getLocation().lon - .1 << "\" y=\"" << station.getLocation().lat - .1 << "\" "
      << "stroke-width=\".05\" stroke=\"red\" width = \".2\" height=\".2\" fill=\"transparent\" />";
    if (breitling_constraints::isStationInMandatoryRegion(*station.getOriginalStation(), 3))
      file
      << "<rect x=\"" << station.getLocation().lon - .1 << "\" y=\"" << station.getLocation().lat - .1 << "\" "
      << "stroke-width=\".05\" stroke=\"purple\" width = \".2\" height=\".2\" fill=\"transparent\" />";
  }

  file << "<path d=\"M";
  for (const Station *station : path.getStations())
    file << station->getLocation().lon << " " << station->getLocation().lat << " ";
  file << "\" stroke-width=\".05\" stroke=\"black\" fill=\"none\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n";

  file << "</svg>" << std::endl;
}
