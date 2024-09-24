#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "osgEarth::osgEarth" for configuration "Debug"
set_property(TARGET osgEarth::osgEarth APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(osgEarth::osgEarth PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/osgEarthd.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/osgEarthd.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS osgEarth::osgEarth )
list(APPEND _IMPORT_CHECK_FILES_FOR_osgEarth::osgEarth "${_IMPORT_PREFIX}/lib/osgEarthd.lib" "${_IMPORT_PREFIX}/bin/osgEarthd.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
