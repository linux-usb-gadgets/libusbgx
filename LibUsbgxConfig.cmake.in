get_filename_component(LibUsbgx_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

# Compute the installation prefix relative to this file.
get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
if(_IMPORT_PREFIX STREQUAL "/")
  set(_IMPORT_PREFIX "")
endif()

set(LibUsbgx_INCLUDE_DIRS "${_IMPORT_PREFIX}/include")

if(NOT TARGET LibUsbgx::LibUsbgx)
	add_library(LibUsbgx::LibUsbgx SHARED IMPORTED)
	set_property(TARGET LibUsbgx::LibUsbgx APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

	set_target_properties(LibUsbgx::LibUsbgx PROPERTIES 
		IMPORTED_LOCATION "${_IMPORT_PREFIX}/lib/libusbgx.so"
		INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include")
endif()

set(LibUsbgx_LIBRARIES LibUsbgx::LibUsbgx)
