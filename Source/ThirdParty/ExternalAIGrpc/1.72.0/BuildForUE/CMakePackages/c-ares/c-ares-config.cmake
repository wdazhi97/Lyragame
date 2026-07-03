# Adapter exposing Unreal Engine's bundled c-ares library as a CMake package.

if(NOT TARGET c-ares::cares)
	add_library(c-ares::cares STATIC IMPORTED)
	set_target_properties(c-ares::cares PROPERTIES
		IMPORTED_LOCATION "${UE_CARES_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${UE_CARES_INCLUDE_DIR}")
endif()

set("c-ares_FOUND" TRUE)
