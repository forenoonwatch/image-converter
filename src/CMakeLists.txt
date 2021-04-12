target_sources(image_converter PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

target_include_directories(image_converter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(b64)
add_subdirectory(stb)