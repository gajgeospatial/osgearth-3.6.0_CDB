#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "osgEarth::osgEarth" for configuration "RelWithDebInfo"
set_property(TARGET osgEarth::osgEarth APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(osgEarth::osgEarth PROPERTIES
  IMPORTED_IMPLIB_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/osgEarth.lib"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/osgEarth.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS osgEarth::osgEarth )
list(APPEND _IMPORT_CHECK_FILES_FOR_osgEarth::osgEarth "${_IMPORT_PREFIX}/lib/osgEarth.lib" "${_IMPORT_PREFIX}/bin/osgEarth.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
