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

void interface_mock::writePathToKML(const GeoMap &geomap, const Path &path, const std::filesystem::path &filePath)
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
            "<name> Longueur du chemin : " << path.length() << " NM</name>\n"
            "<styleUrl>#line-000000-2163-nodesc</styleUrl>\n"
            "<LineString>\n"
            "<tessellate>1</tessellate>\n"
            "<coordinates>\n";

    for (const Station *station : path.getStations()) {
        file << station->getLocation().lon << "," << station->getLocation().lat << ",0\n";
    }

    file << "</coordinates>\n"
            "</LineString>\n"
            "</Placemark>\n";

    for (const Station *station : path.getStations()) {
        file << "<Placemark>\n"
                "<name>" << station->getOACI() << "</name>\n"
                "<styleUrl>";

        if (station == path.getStations().front() || station == path.getStations().back()) {
            file << flag_icon;
        } else {
            file << station_icon;
        }

        file << "</styleUrl>\n"
                "<Point>\n"
                "<coordinates>" << station->getLocation().lon << "," << station->getLocation().lat << ",0</coordinates>\n"
                "</Point>\n"
                "</Placemark>\n";
    }

    file << "</Document>\n"
            "</kml>\n" << std::endl;

}
