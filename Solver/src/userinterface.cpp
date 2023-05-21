#include "userinterface.h"

#include "breitling/breitlingSolver.h"

#include <fstream>

void interface_mock::writePathToFile(const ProblemMap &geomap, const ProblemPath &path, const std::filesystem::path &filePath)
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

  file << "<g transform=\"scale(.83, -1) translate(0, -92.5) \">\n";

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
  for (const ProblemStation station : path)
    file << station.getLocation().lon << " " << station.getLocation().lat << " ";
  file << "\" stroke-width=\".05\" stroke=\"black\" fill=\"none\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n";

  file << "</g>\n";

  file << "</svg>" << std::endl;
}

namespace kml_export {

void writeHeader(std::ostream &out)
{
  out << 
    "<?xml version = \"1.0\" encoding = \"UTF-8\" ?>"
    "\n<kml xmlns=\"http://www.opengis.net/kml/2.2\">"
    "\n  <Document>"
    "\n    <name>Carte</name>"
    "\n    <description/>";
}

void writeFooter(std::ostream &out)
{
  out <<
    "\n  </Document>"
    "\n</kml>";
}

void writeStyle(std::ostream &out, const char *styleId, const char *color, const char *iconHref, bool small=false)
{
  out <<
    "\n<Style id=\"" << styleId << "-normal\">"
    "\n  <IconStyle>"
    "\n    <color>" << color << "</color>"
    "\n    <scale>1</scale>"
    "\n    <Icon>"
    "\n      <href>" << iconHref << "</href>"
    "\n    </Icon>"
    << (small ? "\n    <hotSpot x=\"32\" xunits=\"pixels\" y=\"64\" yunits=\"insetPixels\"/>" : "") <<
    "\n  </IconStyle>"
    "\n  <LabelStyle>"
    "\n    <scale>0</scale>"
    "\n  </LabelStyle>"
    "\n  <BalloonStyle>"
    "\n    <text><h3>$[name]</h3></text>"
    "\n  </BalloonStyle>"
    "\n</Style>";
  out <<
    "\n<Style id=\"" << styleId << "-highlight\">"
    "\n  <IconStyle>"
    "\n    <color>" << color << "</color>"
    "\n    <scale>1</scale>"
    "\n    <Icon>"
    "\n      <href>" << iconHref << "</href>"
    "\n    </Icon>"
    << (small ? "\n    <hotSpot x=\"32\" xunits=\"pixels\" y=\"64\" yunits=\"insetPixels\"/>" : "") <<
    "\n  </IconStyle>"
    "\n  <LabelStyle>"
    "\n    <scale>1</scale>"
    "\n  </LabelStyle>"
    "\n  <BalloonStyle>"
    "\n    <text><h3>$[name]</h3></text>"
    "\n  </BalloonStyle>"
    "\n</Style>";
  out <<
    "\n<StyleMap id=\"" << styleId << "\">"
    "\n  <Pair>"
    "\n    <key>normal</key>"
    "\n    <styleUrl>#" << styleId << "-normal</styleUrl>"
    "\n  </Pair>"
    "\n  <Pair>"
    "\n    <key>highlight</key>"
    "\n    <styleUrl>#" << styleId << "-highlight</styleUrl>"
    "\n  </Pair>"
    "\n</StyleMap>";
}

void writeStation(std::ostream &out, const Station &station, const char *style)
{
  out <<
    "\n<Placemark>"
    "\n  <name>" << station.getName() << "</name>"
    "\n  <styleUrl>#" << style << "</styleUrl>"
    "\n  <Point>"
    "\n    <coordinates>"
    "\n      " << station.getLocation().lon << "," << station.getLocation().lat << ",0"
    "\n    </coordinates>"
    "\n  </Point>"
    "\n</Placemark>";
}

void writeAllStationsLayer(std::ostream &out, const GeoMap &map)
{
  const char *styleId = "station-icon";

  writeStyle(out, "ff5252ff", styleId, "https://www.gstatic.com/mapspro/images/stock/503-wht-blank_maps.png");
  out <<
    "\n<Folder>"
    "\n  <name>A�rodromes</name>";
  for (const Station &station : map.getStations()) {
    writeStation(out, station, styleId);
  }
  out <<
    "\n</Folder>";
}

void writeProblemStationsLayer(std::ostream &out, const ProblemMap &map)
{
  const char *fuelAvailableStyle = "station-wfuel-icon";
  const char *nightAvailableStyle = "station-wnight-icon";
  writeStyle(out, fuelAvailableStyle, "2a9df0", "https://www.gstatic.com/mapspro/images/stock/1027-biz-gas.png");
  writeStyle(out, nightAvailableStyle, "f0ac2a", "https://www.gstatic.com/mapspro/images/stock/1005-biz-convenience.png");

  out <<
    "\n<Folder>"
    "\n  <name>A�rodromes disponibles de nuit</name>";
  for (const ProblemStation &station : map) {
    if (!station.isAccessibleAtNight())
      continue;
    writeStation(out, *station.getOriginalStation(), nightAvailableStyle);
  }
  out <<
    "\n</Folder>";

  out <<
    "\n<Folder>"
    "\n  <name>A�rodromes avec rechargement possible</name>";
  for (const ProblemStation &station : map) {
    if (!station.canBeUsedToFuel())
      continue;
    writeStation(out, *station.getOriginalStation(), fuelAvailableStyle);
  }
  out <<
    "\n</Folder>";
}

void writePathLayer(std::ostream &out, const Path &path, const char *layerName)
{
  const char *pathStyle = "path-path";
  const char *pathIconStyle = "path-icon";
  const char *pathFlagStyle = "path-flag";
  writeStyle(out, pathStyle, "ff5252ff", "https://www.gstatic.com/mapspro/images/stock/503-wht-blank_maps.png", true);
  writeStyle(out, pathIconStyle, "ff5252ff", "https://www.gstatic.com/mapspro/images/stock/503-wht-blank_maps.png", true);
  writeStyle(out, pathFlagStyle, "ff5252ff", "https://www.gstatic.com/mapspro/images/stock/1195-fac-flag.png");

  out <<
    "\n<Folder>"
    "\n  <name>" << layerName << "</name>";

  out <<
    "\n<Placemark>"
    "\n  <name> Longueur du chemin : " << path.length() << " NM</name>"
    "\n  <styleUrl>#" << pathStyle << "</styleUrl>"
    "\n  <LineString>"
    "\n    <tessellate>1</tessellate>"
    "\n    <coordinates>";

  for (const Station *station : path.getStations()) {
    out << station->getLocation().lon << "," << station->getLocation().lat << ",0\n";
  }

  out << 
    "\n    </coordinates>"
    "\n  </LineString>"
    "\n</Placemark>";

  for (const Station *station : path.getStations()) {
    bool flagIcon = station == path.getStations().front() || station == path.getStations().back();
    writeStation(out, *station, flagIcon ? pathFlagStyle : pathIconStyle);
  }

  out <<
    "\n</Folder>";
}

}
