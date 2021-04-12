target_sources(image_converter PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/cencode.cpp"
)

target_include_directories(image_converter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")