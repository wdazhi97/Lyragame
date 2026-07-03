# Adapter exposing Unreal Engine's bundled RE2 library as a CMake package.

if(NOT TARGET re2::re2)
	add_library(re2::re2 STATIC IMPORTED)
	set_target_properties(re2::re2 PROPERTIES
		IMPORTED_LOCATION "${UE_RE2_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${UE_RE2_INCLUDE_DIR}")
endif()

set(re2_FOUND TRUE)
