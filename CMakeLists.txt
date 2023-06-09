cmake_minimum_required(VERSION 3.24)
project(ProjetS8)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

add_subdirectory(Interface_Graphique/Solver/vendor/OpenXLSX)

add_executable(ProjetS8 Interface_Graphique/Solver/src/geoserializer.cpp Interface_Graphique/Solver/src/geoserializer/xlsserializer.cpp Interface_Graphique/Solver/src/geoserializer/csvserializer.cpp Interface_Graphique/Solver/src/tsp/tsp_nearest_multistart_opt.h Interface_Graphique/Solver/src/tsp/tsp_nearest_multistart_opt.cpp Interface_Graphique/Solver/src/userinterface.cpp Interface_Graphique/Solver/src/path.cpp Interface_Graphique/Solver/src/tsp/tsp_optimization.cpp Interface_Graphique/Solver/src/tsp/tsp_nearest_multistart_opt.h Interface_Graphique/Solver/src/tsp/tsp_nearest_multistart_opt.cpp Interface_Graphique/Solver/src/tsp/tsp_optimization.h)
#add_executable(ProjetS8 Solver/src/main.cpp Solver/src/geoserializer.cpp Solver/src/geoserializer/xlsserializer.cpp Solver/src/geoserializer/csvserializer.cpp Solver/src/tsp/tsp_nearest_multistart_opt.h Solver/src/tsp/tsp_nearest_multistart_opt.cpp Solver/src/userinterface.cpp Solver/src/path.cpp Solver/src/tsp/tsp_optimization.cpp Solver/src/tsp/tsp_nearest_multistart_opt.h Solver/src/tsp/tsp_nearest_multistart_opt.cpp Solver/src/tsp/tsp_optimization.h Solver/src/breitling/breitlingSolver.cpp Solver/src/breitling/breitlingnatural.cpp Solver/src/breitling/label_setting_breitling.cpp)
target_link_libraries(ProjetS8 OpenXLSX::OpenXLSX)
