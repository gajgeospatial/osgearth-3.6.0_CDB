#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "osgEarth::osgEarth" for configuration "MinSizeRel"
set_property(TARGET osgEarth::osgEarth APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(osgEarth::osgEarth PROPERTIES
  IMPORTED_IMPLIB_MINSIZEREL "${_IMPORT_PREFIX}/lib/osgEarths.lib"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/bin/osgEarths.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS osgEarth::osgEarth )
list(APPEND _IMPORT_CHECK_FILES_FOR_osgEarth::osgEarth "${_IMPORT_PREFIX}/lib/osgEarths.lib" "${_IMPORT_PREFIX}/bin/osgEarths.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
