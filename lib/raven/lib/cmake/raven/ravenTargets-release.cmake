#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "raven::raven" for configuration "Release"
set_property(TARGET raven::raven APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(raven::raven PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libraven.a"
  )

list(APPEND _cmake_import_check_targets raven::raven )
list(APPEND _cmake_import_check_files_for_raven::raven "${_IMPORT_PREFIX}/lib/libraven.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
