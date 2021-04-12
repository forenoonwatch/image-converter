target_sources(image_converter PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/stb_image.cpp"
#	"${CMAKE_CURRENT_SOURCE_DIR}/stb_image_write.cpp"
)

target_include_directories(image_converter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")