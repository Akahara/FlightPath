#include "xlsserializer.h"

GeoMap XLSSerializer::parseMap(const std::filesystem::path& file) const
{
    //GeoMap map{};

    //OpenXLSX::XLDocument doc;
    //doc.open(file.string());

    //auto sheetName = doc.workbook().worksheetNames()[0];
    //auto worksheet = doc.workbook().worksheet(sheetName);

    //for (auto row = 2; row <= worksheet.rowCount(); ++row) { // skip the header

    //    // skip excluded stations
    //    std::string exclude = worksheet.cell(row, EXCLUDE_COLUMN).value().get<std::string>();
    //    if (exclude == "x" || exclude == "X")
    //        continue;

    //    // get all the fields
    //    std::string OACI = worksheet.cell(row, OACI_COLUMN).value().get<std::string>();
    //    std::string name = worksheet.cell(row, NAME_COLUMN).value().get<std::string>();
    //    std::string lat = worksheet.cell(row, LATITUDE_COLUMN).value().get<std::string>();
    //    std::string lon = worksheet.cell(row, LONGITUDE_COLUMN).value().get<std::string>();
    //    std::string status = worksheet.cell(row, STATUS_COLUMN).value().get<std::string>();
    //    std::string nightVFR = worksheet.cell(row, NIGHT_VFR_COLUMN).value().get<std::string>();
    //    std::string fuel = worksheet.cell(row, FUEL_COLUMN).value().get<std::string>();

    //    // create a Location from the coordinates
    //    Location location = Location::fromNECoordinates(string2coordinate(lat), string2coordinate(lon));

    //    // create a Station
    //    Station station(location, name, OACI);

    //    // TODO add other fields

    //    // add the station to the map
    //    map.getStations().push_back(station);
    //}

    return{};
}

void XLSSerializer::writePath(const std::filesystem::path& file, const Path& path) const
{
    // TODO implement XLSSerializer::writePath
}