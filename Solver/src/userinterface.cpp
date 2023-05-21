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

void interface_mock::writePathToKML(const ProblemMap &geomap, const ProblemPath &path, const std::filesystem::path &filePath)
{
    constexpr char fuel_icon[] = "#icon-1581-FF5252-labelson-nodesc";
    constexpr char flag_icon[] = "#icon-1661-FF5252-labelson-nodesc";
    constexpr char station_icon[] = "#icon-1739-FF5252-labelson-nodesc";

    std::ofstream file{ filePath };

    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
            "<Document>\n"
            "<name>Path</name>\n"
            "<Style id=\"icon-1581-FF5252-labelson-nodesc\">\n"
            "<IconStyle>\n"
            "<color>ff5252ff</color>\n"
            "<scale>1</scale>\n"
            "<Icon>\n"
            "<href>https://www.gstatic.com/mapspro/images/stock/503-wht-blank_maps.png</href>\n"
            "</Icon>\n"
            "</IconStyle>\n"
            "<BalloonStyle>\n"
            "<text><![CDATA[<h3>$[name]</h3>]]></text>\n"
            "</BalloonStyle>\n"
            "</Style>\n"
            "<Style id=\"icon-1661-FF5252-labelson-nodesc\">\n"
            "<IconStyle>\n"
            "<color>ff5252ff</color>\n"
            "<scale>1</scale>\n"
            "<Icon>\n"
            "<href>https://www.gstatic.com/mapspro/images/stock/503-wht-blank_maps.png</href>\n"
            "</Icon>\n"
            "</IconStyle>\n"
            "<BalloonStyle>\n"
            "<text><![CDATA[<h3>$[name]</h3>]]></text>\n"
            "</BalloonStyle>\n"
            "</Style>\n"
            "<Style id=\"icon-1739-FF5252-labelson-nodesc\">\n"
            "<IconStyle>\n"
            "<color>ff5252ff</color>\n"
            "<scale>1</scale>\n"
            "<Icon>\n"
            "<href>https://www.gstatic.com/mapspro/images/stock/503-wht-blank_maps.png</href>\n"
            "</Icon>\n"
            "</IconStyle>\n"
            "<BalloonStyle>\n"
            "<text><![CDATA[<h3>$[name]</h3>]]></text>\n"
            "</BalloonStyle>\n"
            "</Style>\n"
            "<Style id=\"line-000000-2163-nodesc-normal\">\n"
            "<LineStyle>\n"
            "<color>ff000000</color>\n"
            "<width>2.163</width>\n"
            "</LineStyle>\n"
            "<BalloonStyle>\n"
            "<text><![CDATA[<h3>$[name]</h3>]]></text>\n"
            "</BalloonStyle>\n"
            "</Style>\n"
            "<Style id=\"line-000000-2163-nodesc-highlight\">\n"
            "<LineStyle>\n"
            "<color>ff000000</color>\n"
            "<width>3.2445</width>\n"
            "</LineStyle>\n"
            "<BalloonStyle>\n"
            "<text><![CDATA[<h3>$[name]</h3>]]></text>\n"
            "</BalloonStyle>\n"
            "</Style>\n"
            "<StyleMap id=\"line-000000-2163-nodesc\">\n"
            "<Pair>\n"
            "<key>normal</key>\n"
            "<styleUrl>#line-000000-2163-nodesc-normal</styleUrl>\n"
            "</Pair>\n"
            "<Pair>\n"
            "<key>highlight</key>\n"
            "<styleUrl>#line-000000-2163-nodesc-highlight</styleUrl>\n"
            "</Pair>\n"
            "</StyleMap>\n"
            "<Placemark>\n"
            "<name> Longueur du chemin : " << getLength(path) << " NM</name>\n"
            "<styleUrl>#line-000000-2163-nodesc</styleUrl>\n"
            "<LineString>\n"
            "<tessellate>1</tessellate>\n"
            "<coordinates>\n";

    for (const ProblemStation &station : path) {
        file << station.getLocation().lon << "," << station.getLocation().lat << ",0\n";
    }

    file << "</coordinates>\n"
            "</LineString>\n"
            "</Placemark>\n";

    for (const ProblemStation &station : path) {
        file << "<Placemark>\n"
                "<name>" << station.getOriginalStation()->getOACI() << "</name>\n"
                "<styleUrl>";

        if (station == path.front() || station == path.back()) {
            file << flag_icon;
        } else {
            file << station_icon;
        }

        file << "</styleUrl>\n"
                "<Point>\n"
                "<coordinates>" << station.getLocation().lon << "," << station.getLocation().lat << ",0</coordinates>\n"
                "</Point>\n"
                "</Placemark>\n";
    }

    file << "</Document>\n"
            "</kml>\n" << std::endl;

}
