#include <iostream>

#include "../vendor/OpenXLSX/OpenXLSX.hpp"

#include "geomap.h"
#include "geometry.h"
#include "tsp/genetictsp.h"
#include "userinterface.h"
#include "../src/station.h"

void testDummy()
{
    // TODO move all this in test classes, or scrap completely

    size_t T = 10;
    for (size_t x = 0; x < T; x++)
        for (size_t y = 0; y < T; y++)
            map.getStations().push_back(Station{ Location{(double)x,(double)y} });

    //Location london{ -0.1276, 51.5072 }; // note! 51N .12W the order is swapped
    //Location paris{ 2.3522, 48.8566 };
    Location london = Location::fromNECoordinates(51.5072, -0.1276);
    Location paris = Location::fromNECoordinates(48.8566, 2.3522);
    std::cout << "distance from london to paris=" << geometry::distance(london, paris) << "nmi" << std::endl;

    std::cin.get();
}

int main()
{
    using namespace OpenXLSX;

    XLDocument doc;
    doc.create("Spreadsheet.xlsx");
    auto wks = doc.workbook().worksheet("Sheet1");

    wks.cell("A1").value() = "Hello, OpenXLSX!";

    doc.save();
    doc.close();

    XLDocument d2;
    d2.open("Spreadsheet.xlsx");
    for (const auto &name : d2.workbook().worksheetNames())
        std::cout << name << std::endl;
    std::string name = d2.workbook().worksheetNames()[0];
    std::cout << "opening " << name << std::endl;
    auto wk2 = d2.workbook().worksheet(name);
    std::cout << wk2.cell("A1").value().get<std::string>() << std::endl;

    std::cin.get();
    return 0;
}