project(media-controls VERSION 0.3.1)
set(PROJECT_FULL_NAME "Media Controls")

set(media-controls_HEADERS
    media-controls.hpp
    media-control.hpp
    media-slider.hpp
	../../slider-absoluteset-style.hpp)

set(media-controls_SOURCES
    media-slider.cpp
    media-controls.cpp
    media-control.cpp
	../../slider-absoluteset-style.cpp)

if(WIN32)
	get_filename_component(ISS_FILES_DIR "${CMAKE_BINARY_DIR}\\..\\package" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_FILES_DIR}" ISS_FILES_DIR)
	get_filename_component(ISS_PACKAGE_DIR "${CMAKE_PACKAGE_PREFIX}\\.." ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_PACKAGE_DIR}" ISS_PACKAGE_DIR)
	get_filename_component(ISS_SOURCE_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_SOURCE_DIR}" ISS_SOURCE_DIR)
	configure_file("installer.iss.in"
			"${PROJECT_BINARY_DIR}/installer.iss"
		)
	configure_file(resource.rc.in media-controls.rc)
	list(APPEND media-controls_SOURCES
		media-controls.rc)
endif()


qt5_wrap_ui(media-controls_UI_HEADERS MediaControls.ui)

add_library(media-controls MODULE
	${media-controls_HEADERS}
	${media-controls_SOURCES}
	${media-controls_UI_HEADERS})

target_link_libraries(media-controls
	obs-frontend-api
	Qt5::Widgets
	libobs)

set_target_properties(media-controls PROPERTIES FOLDER "plugins/exeldro")
install_obs_plugin_with_data(media-controls data)
