project(media-controls)

set(media-controls_HEADERS
    media-controls.hpp
    media-control.hpp
    media-slider.hpp)

set(media-controls_SOURCES
    media-slider.cpp
    media-controls.cpp
    media-control.cpp)

if(WIN32)
	list(APPEND media-controls_SOURCES
		media-controls.rc)
endif()


qt5_wrap_ui(media-controls_UI_HEADERS MediaControls.ui)
qt5_add_resources(media-controls_QRC_SOURCES media-controls.qrc)

add_library(media-controls MODULE
	${media-controls_HEADERS}
	${media-controls_SOURCES}
    ${media-controls_UI_HEADERS}
    ${media-controls_QRC_SOURCES})
target_link_libraries(media-controls
	obs-frontend-api
	Qt5::Widgets
	libobs)

set_target_properties(media-controls PROPERTIES FOLDER "plugins/exeldro")
install_obs_plugin_with_data(media-controls data)