# Install script for directory: N:/Development/Dev_Base/osgearth-3.6/src/osgEarthDrivers/detail

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

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/osgPlugins-3.7.0" TYPE MODULE FILES "N:/Development/Dev_Base/osgearth-3.6/msvc/src/osgEarthDrivers/detail/Debug/osgdb_osgearth_detaild.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/osgPlugins-3.7.0" TYPE MODULE FILES "N:/Development/Dev_Base/osgearth-3.6/msvc/src/osgEarthDrivers/detail/Release/osgdb_osgearth_detail.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/osgPlugins-3.7.0" TYPE MODULE FILES "N:/Development/Dev_Base/osgearth-3.6/msvc/src/osgEarthDrivers/detail/MinSizeRel/osgdb_osgearth_details.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/osgPlugins-3.7.0" TYPE MODULE FILES "N:/Development/Dev_Base/osgearth-3.6/msvc/src/osgEarthDrivers/detail/RelWithDebInfo/osgdb_osgearth_detail.dll")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/osgEarthDrivers/detail" TYPE FILE FILES
    "N:/Development/Dev_Base/osgearth-3.6/src/osgEarthDrivers/detail/DetailExtension"
    "N:/Development/Dev_Base/osgearth-3.6/src/osgEarthDrivers/detail/DetailOptions"
    "N:/Development/Dev_Base/osgearth-3.6/src/osgEarthDrivers/detail/DetailTerrainEffect"
    )
endif()

