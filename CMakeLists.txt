cmake_minimum_required(VERSION 3.24)
project(Projet_S8)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

add_subdirectory(Solver/vendor/OpenXLSX)

add_executable(Projet_S8 Solver/src/main.cpp Solver/src/geoserializer.cpp Solver/src/geoserializer/xlsserializer.cpp Solver/src/geoserializer/csvserializer.cpp Solver/src/tsp/tspnearest.h Solver/src/tsp/tspnearest.cpp Solver/src/breitling/BreitlingSolver.cpp Solver/src/breitling/label_setting_breitling.h Solver/src/userinterface.cpp Solver/src/tsp/genetictsp.cpp Solver/src/tsp/TspSolver.cpp Solver/src/path.cpp)
target_link_libraries(Projet_S8 OpenXLSX::OpenXLSX)
