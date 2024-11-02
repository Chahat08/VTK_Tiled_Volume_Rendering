# CMakeList.txt : CMake project for VTK_Tiled_Volume_Rendering, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (VTK_Tiled_Volume_Rendering "VTK_Tiled_Volume_Rendering.cpp" "VTK_Tiled_Volume_Rendering.h")

target_link_libraries(VTK_Tiled_Volume_Rendering PRIVATE ${VTK_LIBRARIES})

vtk_module_autoinit(
	TARGETS VTK_Tiled_Volume_Rendering
	MODULES ${VTK_LIBRARIES}
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET VTK_Tiled_Volume_Rendering PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
