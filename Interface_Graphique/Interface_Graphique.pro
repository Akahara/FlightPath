QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Solver/src/breitling/breitlingSolver.cpp \
    Solver/src/breitling/breitlingnatural.cpp \
    Solver/src/breitling/label_setting_breitling.cpp \
    Solver/src/geoserializer.cpp \
    Solver/src/geoserializer/csvserializer.cpp \
    Solver/src/geoserializer/xlsserializer.cpp \
    Solver/src/path.cpp \
    Solver/src/tsp/genetictsp.cpp \
    Solver/src/tsp/tsp_nearest_multistart_opt.cpp \
    Solver/src/tsp/tsp_optimization.cpp \
    Solver/src/userinterface.cpp \
    Solver/vendor/OpenXLSX/external/pugixml/pugixml.cpp \
    Solver/vendor/OpenXLSX/sources/XLCell.cpp \
    Solver/vendor/OpenXLSX/sources/XLCellIterator.cpp \
    Solver/vendor/OpenXLSX/sources/XLCellRange.cpp \
    Solver/vendor/OpenXLSX/sources/XLCellReference.cpp \
    Solver/vendor/OpenXLSX/sources/XLCellValue.cpp \
    Solver/vendor/OpenXLSX/sources/XLColor.cpp \
    Solver/vendor/OpenXLSX/sources/XLColumn.cpp \
    Solver/vendor/OpenXLSX/sources/XLContentTypes.cpp \
    Solver/vendor/OpenXLSX/sources/XLDateTime.cpp \
    Solver/vendor/OpenXLSX/sources/XLDocument.cpp \
    Solver/vendor/OpenXLSX/sources/XLFormula.cpp \
    Solver/vendor/OpenXLSX/sources/XLProperties.cpp \
    Solver/vendor/OpenXLSX/sources/XLRelationships.cpp \
    Solver/vendor/OpenXLSX/sources/XLRow.cpp \
    Solver/vendor/OpenXLSX/sources/XLRowData.cpp \
    Solver/vendor/OpenXLSX/sources/XLSharedStrings.cpp \
    Solver/vendor/OpenXLSX/sources/XLSheet.cpp \
    Solver/vendor/OpenXLSX/sources/XLWorkbook.cpp \
    Solver/vendor/OpenXLSX/sources/XLXmlData.cpp \
    Solver/vendor/OpenXLSX/sources/XLXmlFile.cpp \
    Solver/vendor/OpenXLSX/sources/XLZipArchive.cpp \
    excelhelper.cpp \
    main.cpp \
    mainwindow.cpp \
    mainwindowtest.cpp

HEADERS += \
    Solver/src/breitling/breitlingSolver.h \
    Solver/src/breitling/breitlingnatural.h \
    Solver/src/breitling/label_setting_breitling.h \
    Solver/src/breitling/structures.h \
    Solver/src/geography.h \
    Solver/src/geomap.h \
    Solver/src/geometry.h \
    Solver/src/geoserializer.h \
    Solver/src/geoserializer/csvserializer.h \
    Solver/src/geoserializer/xlsserializer.h \
    Solver/src/path.h \
    Solver/src/pathsolver.h \
    Solver/src/station.h \
    Solver/src/tsp/genetictsp.h \
    Solver/src/tsp/tsp_nearest_multistart_opt.h \
    Solver/src/tsp/tsp_optimization.h \
    Solver/src/userinterface.h \
    Solver/vendor/OpenXLSX/OpenXLSX.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/args.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/cenv.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/config.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/convert.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/cstdio.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/cstdlib.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/encoding_errors.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/encoding_utf.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/filebuf.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/fstream.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/iostream.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/scoped_ptr.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/stackstring.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/system.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/utf.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/utf8_codecvt.hpp \
    Solver/vendor/OpenXLSX/external/nowide/nowide/windows.hpp \
    Solver/vendor/OpenXLSX/external/pugixml/pugiconfig.hpp \
    Solver/vendor/OpenXLSX/external/pugixml/pugixml.hpp \
    Solver/vendor/OpenXLSX/external/zippy/zippy.hpp \
    Solver/vendor/OpenXLSX/headers/OpenXLSX-Exports.hpp \
    Solver/vendor/OpenXLSX/headers/XLCell.hpp \
    Solver/vendor/OpenXLSX/headers/XLCellIterator.hpp \
    Solver/vendor/OpenXLSX/headers/XLCellRange.hpp \
    Solver/vendor/OpenXLSX/headers/XLCellReference.hpp \
    Solver/vendor/OpenXLSX/headers/XLCellValue.hpp \
    Solver/vendor/OpenXLSX/headers/XLColor.hpp \
    Solver/vendor/OpenXLSX/headers/XLColumn.hpp \
    Solver/vendor/OpenXLSX/headers/XLCommandQuery.hpp \
    Solver/vendor/OpenXLSX/headers/XLConstants.hpp \
    Solver/vendor/OpenXLSX/headers/XLContentTypes.hpp \
    Solver/vendor/OpenXLSX/headers/XLDateTime.hpp \
    Solver/vendor/OpenXLSX/headers/XLDocument.hpp \
    Solver/vendor/OpenXLSX/headers/XLException.hpp \
    Solver/vendor/OpenXLSX/headers/XLFormula.hpp \
    Solver/vendor/OpenXLSX/headers/XLIterator.hpp \
    Solver/vendor/OpenXLSX/headers/XLProperties.hpp \
    Solver/vendor/OpenXLSX/headers/XLRelationships.hpp \
    Solver/vendor/OpenXLSX/headers/XLRow.hpp \
    Solver/vendor/OpenXLSX/headers/XLRowData.hpp \
    Solver/vendor/OpenXLSX/headers/XLSharedStrings.hpp \
    Solver/vendor/OpenXLSX/headers/XLSheet.hpp \
    Solver/vendor/OpenXLSX/headers/XLWorkbook.hpp \
    Solver/vendor/OpenXLSX/headers/XLXmlData.hpp \
    Solver/vendor/OpenXLSX/headers/XLXmlFile.hpp \
    Solver/vendor/OpenXLSX/headers/XLXmlParser.hpp \
    Solver/vendor/OpenXLSX/headers/XLZipArchive.hpp \
    Solver/vendor/OpenXLSX/sources/utilities/XLUtilities.hpp \
    excelhelper.h \
    fuelmodel.h \
    mainwindow.h \
    mainwindowtest.h \
    stationmodel.h

FORMS += \
    mainwindow.ui \
    mainwindowtest.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Solver/aerodromes.csv \
    Solver/aerodromes.xlsx \
    Solver/vendor/OpenSXLSX_LICENSE.md \
    Solver/vendor/OpenXLSX/CMakeLists.txt \
    Solver/vendor/OpenXLSX/OpenXLSXConfig.cmake

INCLUDEPATH += \
    Solver/vendor/OpenXLSX \
    Solver/vendor/OpenXLSX/external/pugixml \
    Solver/vendor/OpenXLSX/external/nowide \
    Solver/vendor/OpenXLSX/external/zippy \
    Solver/vendor/OpenXLSX/headers

TARGET = FlightPath
