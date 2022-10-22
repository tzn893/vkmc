#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "flecs::flecs" for configuration "MinSizeRel"
set_property(TARGET flecs::flecs APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(flecs::flecs PROPERTIES
  IMPORTED_IMPLIB_MINSIZEREL "${_IMPORT_PREFIX}/lib/flecs.lib"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/bin/flecs.dll"
  )

list(APPEND _cmake_import_check_targets flecs::flecs )
list(APPEND _cmake_import_check_files_for_flecs::flecs "${_IMPORT_PREFIX}/lib/flecs.lib" "${_IMPORT_PREFIX}/bin/flecs.dll" )

# Import target "flecs::flecs_static" for configuration "MinSizeRel"
set_property(TARGET flecs::flecs_static APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(flecs::flecs_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "C"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/flecs_static.lib"
  )

list(APPEND _cmake_import_check_targets flecs::flecs_static )
list(APPEND _cmake_import_check_files_for_flecs::flecs_static "${_IMPORT_PREFIX}/lib/flecs_static.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
