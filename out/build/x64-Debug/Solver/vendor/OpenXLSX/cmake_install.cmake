# Install script for directory: C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/out/install/x64-Debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OpenXLSX/headers" TYPE FILE FILES "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/out/build/x64-Debug/Solver/vendor/OpenXLSX/OpenXLSX-Exports.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OpenXLSX/headers" TYPE FILE FILES
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/OpenXLSX-Exports.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLCell.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLCellIterator.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLCellRange.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLCellReference.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLCellValue.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLColor.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLColumn.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLCommandQuery.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLConstants.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLContentTypes.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLDateTime.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLDocument.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLException.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLFormula.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLIterator.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLProperties.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLRelationships.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLRow.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLRowData.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLSharedStrings.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLSheet.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLWorkbook.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLXmlData.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLXmlFile.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLXmlParser.hpp"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/headers/XLZipArchive.hpp"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OpenXLSX" TYPE FILE FILES "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/OpenXLSX.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "lib" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/out/build/x64-Debug/output/OpenXLSXd.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/OpenXLSX" TYPE FILE FILES
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/Solver/vendor/OpenXLSX/OpenXLSXConfig.cmake"
    "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/out/build/x64-Debug/Solver/vendor/OpenXLSX/OpenXLSX/OpenXLSXConfigVersion.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/OpenXLSX/OpenXLSXTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/OpenXLSX/OpenXLSXTargets.cmake"
         "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/out/build/x64-Debug/Solver/vendor/OpenXLSX/CMakeFiles/Export/c72cc94553a1a0c9b05f75dae42fb1d7/OpenXLSXTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/OpenXLSX/OpenXLSXTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/OpenXLSX/OpenXLSXTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/OpenXLSX" TYPE FILE FILES "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/out/build/x64-Debug/Solver/vendor/OpenXLSX/CMakeFiles/Export/c72cc94553a1a0c9b05f75dae42fb1d7/OpenXLSXTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/OpenXLSX" TYPE FILE FILES "C:/Users/jesie/Documents/Cours/Polytech Tours/S8/Aero/ProjetS8/out/build/x64-Debug/Solver/vendor/OpenXLSX/CMakeFiles/Export/c72cc94553a1a0c9b05f75dae42fb1d7/OpenXLSXTargets-debug.cmake")
  endif()
endif()

