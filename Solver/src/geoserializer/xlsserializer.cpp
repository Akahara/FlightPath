#include "xlsserializer.h"

#include <OpenXLSX.hpp>

GeoMap XLSSerializer::parseMap(const std::filesystem::path& file) const
{
    GeoMap map{};

    OpenXLSX::XLDocument doc;
    doc.open(file.string());

    std::string sheetName = doc.workbook().worksheetNames()[0];
    OpenXLSX::XLWorksheet worksheet = doc.workbook().worksheet(sheetName);

    for (auto row = 2; row <= worksheet.rowCount(); row++) { // skip the header
        try {
            // Skip excluded stations
            auto exclude = worksheet.cell(row, EXCLUDE_COLUMN).value().get<std::string>();
            if (exclude == "x" || exclude == "X") {
                continue;
            }

            // Get all the fields
            std::string OACI, name, lat, lon, status, nightVFR, fuel;

            OACI = worksheet.cell(row, OACI_COLUMN).value().get<std::string>();
            name = worksheet.cell(row, NAME_COLUMN).value().get<std::string>();
            lat = worksheet.cell(row, LATITUDE_COLUMN).value().get<std::string>();
            lon = worksheet.cell(row, LONGITUDE_COLUMN).value().get<std::string>();
            status = worksheet.cell(row, STATUS_COLUMN).value().get<std::string>();
            nightVFR = worksheet.cell(row, NIGHT_VFR_COLUMN).value().get<std::string>();
            fuel = worksheet.cell(row, FUEL_COLUMN).value().get<std::string>();

            // Create a Location from the coordinates
            Location location = Location::fromNECoordinates(string2coordinate(lat), string2coordinate(lon));

            // Create a Station
            Station station(location, name, OACI, status, nightVFR, fuel);

            // Add the station to the map
            map.getStations().push_back(station);
        } catch (std::exception &e) {
            throw std::runtime_error("Error while parsing the file \""
                                     + file.string()
                                     + "\" at row "
                                     + std::to_string(row)
                                     + ": " + e.what());
        }
    }

    return map;
}

void XLSSerializer::writePath(const std::filesystem::path& file, const Path& path) const
{
    // TODO : Create a specific directory for the model file

    // Delete the file if it already exists
    if (std::filesystem::exists("FicheDeNavigation.xlsx"))
    {
        std::filesystem::remove("FicheDeNavigation.xlsx");
    }

    // Copy the template file
    std::filesystem::copy("FicheDeNavigationModel.xlsx", "FicheDeNavigation.xlsx");

    // Open the file
    OpenXLSX::XLDocument doc;
    doc.open("FicheDeNavigation.xlsx");
    auto wks = doc.workbook().worksheet("Sheet1");

    std::vector<const Station *> stations = path.getStations();

    // Fill with stations
    int line;
    for (int idx = 0; idx < stations.size(); idx++)
    {
        line = 2*idx + 2; // 2, 4, 6, ...
        wks.cell("A" + std::to_string(line)).value() = stations[idx]->getOACI();
        wks.cell("B" + std::to_string(line)).value() = stations[idx]->getName();
    }

    // Fill with distances and caps
    for (int idx = 0; idx < stations.size() - 1; idx++)
    {
        line = 2 * idx + 3; // 3, 5, 7, ...

        nauticmiles_t distance = geometry::distance(stations[idx]->getLocation(), stations[idx + 1]->getLocation());
        distance = std::round(distance);

        double cap = geometry::cap(stations[idx]->getLocation(), stations[idx + 1]->getLocation());
        cap = std::round(cap);

        wks.cell("D" + std::to_string(line)).value() = cap;
        wks.cell("E" + std::to_string(line)).value() = distance;
    }

    doc.save();
}
