# Install script for directory: N:/Development/Dev_Base/osgearth-3.6/src/applications

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/OSGEARTH")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
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

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_viewer/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_imgui/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_tfs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_boundarygen/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_version/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_atlas/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_bakefeaturetiles/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_conv/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_3pv/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_clamp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_simple/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_features/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_featurefilter/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_los/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_terrainprofile/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_map/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_annotation/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_occlusionculling/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_minimap/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_mrt/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_pick/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_skyview/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_infinitescroll/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_video/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("N:/Development/Dev_Base/osgearth-3.6/msvc/src/applications/osgearth_heatmap/cmake_install.cmake")
endif()

