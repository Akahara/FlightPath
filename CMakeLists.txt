cmake_minimum_required(VERSION 3.24)
project(ProjetS8)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

add_subdirectory(Solver/vendor/OpenXLSX)

add_executable(ProjetS8 Solver/src/main.cpp Solver/src/geoserializer/xlsserializer.cpp Solver/src/geoserializer/csvserializer.cpp Solver/src/geoserializer.cpp Solver/src/breitling/BreitlingSolver.cpp Solver/src/breitling/label_setting_breitling.cpp Solver/src/path.cpp Solver/src/userinterface.cpp)
target_link_libraries(ProjetS8 OpenXLSX::OpenXLSX)