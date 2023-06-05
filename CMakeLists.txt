HEAD
cmake_minimum_required(VERSION 3.20)
project(Projet_S8)
cmake_minimum_required(VERSION 3.20)
project(ProjetS8)
22e33170fa47f8671add0e2349b1dd095adb7966

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

add_subdirectory(Solver/vendor/OpenXLSX)

add_executable(ProjetS8 Solver/src/main.cpp Solver/src/geoserializer.cpp Solver/src/geoserializer/xlsserializer.cpp Solver/src/geoserializer/csvserializer.cpp Solver/src/tsp/tsp_nearest_multistart_opt.h Solver/src/tsp/tsp_nearest_multistart_opt.cpp Solver/src/userinterface.cpp Solver/src/path.cpp Solver/src/tsp/tsp_optimization.cpp Solver/src/tsp/tsp_nearest_multistart_opt.h Solver/src/tsp/tsp_nearest_multistart_opt.cpp Solver/src/tsp/tsp_optimization.h)
#add_executable(ProjetS8 Solver/src/main.cpp Solver/src/geoserializer.cpp Solver/src/geoserializer/xlsserializer.cpp Solver/src/geoserializer/csvserializer.cpp Solver/src/tsp/tsp_nearest_multistart_opt.h Solver/src/tsp/tsp_nearest_multistart_opt.cpp Solver/src/userinterface.cpp Solver/src/path.cpp Solver/src/tsp/tsp_optimization.cpp Solver/src/tsp/tsp_nearest_multistart_opt.h Solver/src/tsp/tsp_nearest_multistart_opt.cpp Solver/src/tsp/tsp_optimization.h Solver/src/breitling/breitlingSolver.cpp Solver/src/breitling/breitlingnatural.cpp Solver/src/breitling/label_setting_breitling.cpp)
target_link_libraries(ProjetS8 OpenXLSX::OpenXLSX)
