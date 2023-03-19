#include "xlsserializer.h"

GeoMap XLSSerializer::parseMap(const std::filesystem::path &file) const
{
    GeoMap map{};

    OpenXLSX::XLDocument doc;
    doc.open(file.string());

    std::string sheetName = doc.workbook().worksheetNames()[0];
    OpenXLSX::XLWorksheet worksheet = doc.workbook().worksheet(sheetName);

    for (auto row = 2; row <= worksheet.rowCount(); ++row) { // skip the header
        try {
            // skip excluded stations
            auto exclude = worksheet.cell(row, EXCLUDE_COLUMN).value().get<std::string>();
            if (exclude == "x" || exclude == "X")
                continue;

            // get all the fields
            std::string OACI, name, lat, lon, status, nightVFR_string, fuel_string;
            OACI = worksheet.cell(row, OACI_COLUMN).value().get<std::string>();
            name = worksheet.cell(row, NAME_COLUMN).value().get<std::string>();
            lat = worksheet.cell(row, LATITUDE_COLUMN).value().get<std::string>();
            lon = worksheet.cell(row, LONGITUDE_COLUMN).value().get<std::string>();
            status = worksheet.cell(row, STATUS_COLUMN).value().get<std::string>();
            nightVFR_string = worksheet.cell(row, NIGHT_VFR_COLUMN).value().get<std::string>();
            fuel_string = worksheet.cell(row, FUEL_COLUMN).value().get<std::string>();
            std::transform(nightVFR_string.begin(), nightVFR_string.end(), nightVFR_string.begin(), ::toupper);
            std::transform(fuel_string.begin(), fuel_string.end(), fuel_string.begin(), ::toupper);

            // remove brand names after AUTOMAT
            if (fuel_string.contains("AUTOMAT")) {
                fuel_string = "AUTOMAT";
            }

            // create a Location from the coordinates
            Location location = Location::fromNECoordinates(string2coordinate(lat), string2coordinate(lon));

            Station::NightVFR nightVFR;
            Station::Fuel fuel;

            // check if the values of NightVFR and Fuel are valid
            try {
                nightVFR = NightVFR_fromString.at(nightVFR_string);
            } catch (std::out_of_range &e) {
                throw std::runtime_error("Invalid NightVFR value: " + nightVFR_string);
            }
            try {
                fuel = Fuel_fromString.at(fuel_string);
            } catch (std::out_of_range &e) {
                throw std::runtime_error("Invalid Fuel value: " + fuel_string);
            }

            // create a Station
            Station station(location, name, OACI, status, nightVFR, fuel);

            // add the station to the map
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

void XLSSerializer::writePath(const std::filesystem::path &file, const Path &path) const
{
    // TODO implement XLSSerializer::writePath

}
